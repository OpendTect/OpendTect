/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          July 2000
________________________________________________________________________

-*/
static const char* rcsID = "$Id: flatauxdataeditor.cc,v 1.28 2009-09-10 11:11:49 cvssatyaki Exp $";

#include "flatauxdataeditor.h"

#include "mouseevent.h"
#include "menuhandler.h"
#include "polygon.h"
#include "rcol2coord.h"


namespace FlatView
{

AuxDataEditor::AuxDataEditor( Viewer& v, MouseEventHandler& meh )
    : viewer_( v )
    , mousehandler_( meh )
    , feedback_( 0 )
    , mousedown_( false )
    , hasmoved_( false )
    , addauxdataid_( 0 )
    , removeSelected( this )
    , movementStarted( this )
    , movementFinished( this )
    , seldatasetidx_( -1 )
    , polygonsellst_( LineStyle::Solid, 1, Color( 255, 0, 0 ) )
    , polygonselrect_( true )
    , isselactive_( true )
    , movementlimit_( 0 )
    , menuhandler_( 0 )
{
    meh.buttonPressed.notify( mCB(this,AuxDataEditor,mousePressCB) );
    meh.buttonReleased.notify( mCB(this,AuxDataEditor,mouseReleaseCB) );
    meh.movement.notify( mCB(this,AuxDataEditor,mouseMoveCB) );
}


AuxDataEditor::~AuxDataEditor()
{
    mousehandler_.buttonPressed.remove( mCB(this,AuxDataEditor,mousePressCB) );
    mousehandler_.buttonReleased.remove(mCB(this,AuxDataEditor,mouseReleaseCB));
    mousehandler_.movement.remove( mCB(this,AuxDataEditor,mouseMoveCB) );

    if ( feedback_ )
    {
	viewer_.appearance().annot_.auxdata_ -= feedback_;
	delete feedback_;
    }

    removeSelectionPolygon();
    viewer_.handleChange( Viewer::Annot );
    limitMovement( 0 );

    setMenuHandler( 0 );
}


bool AuxDataEditor::removeSelectionPolygon()
{
    if ( !polygonsel_.size() )
	return false;

    for ( int idx=0; idx<polygonsel_.size(); idx++ )
	viewer_.appearance().annot_.auxdata_ -= polygonsel_[idx];

    deepErase( polygonsel_ );

    return true;
}


int AuxDataEditor::addAuxData( FlatView::Annotation::AuxData* nd, bool doedit )
{
    if ( viewer_.appearance().annot_.auxdata_.indexOf( nd )==-1 )
    {
	pErrMsg("Auxdata not present in viewer");
	return -1;
    }

    int res = 0;
    while ( ids_.indexOf( res )!=-1 )
	res++;

    ids_ += res;
    auxdata_ += nd;
    allowadd_ += true;
    allowmove_ += true;
    allowremove_ += true;
    doedit_ += doedit;

    return res;
}


void AuxDataEditor::removeAuxData( int id )
{
    const int idx = ids_.indexOf( id );
    if ( idx<0 )
	return;

    ids_.remove( idx );
    auxdata_.remove( idx );
    allowadd_.remove( idx );
    allowmove_.remove( idx );
    allowremove_.remove( idx );
    doedit_.remove( idx );
}


void AuxDataEditor::enableEdit( int id, bool allowadd, bool allowmove,
			        bool allowdelete )
{
    const int idx = ids_.indexOf( id );

    allowadd_[idx] = allowadd;
    allowmove_[idx] = allowmove;
    allowremove_[idx] = allowdelete;
}


void AuxDataEditor::setAddAuxData( int id )
{
    addauxdataid_ = id;
    if ( removeSelectionPolygon() ) viewer_.handleChange( Viewer::Annot );
}


int AuxDataEditor::getAddAuxData() const
{
    return addauxdataid_;
}

    
void AuxDataEditor::setView( const Rect& wv,
			     const Geom::Rectangle<int>& mouserect )
{
    curview_ = wv;
    mousearea_ = mouserect;
}


int AuxDataEditor::getSelPtDataID() const
{ return seldatasetidx_!=-1 ? ids_[seldatasetidx_] : -1; }


const TypeSet<int>& AuxDataEditor::getSelPtIdx() const
{ return selptidx_; }


const Point& AuxDataEditor::getSelPtPos() const
{ return selptcoord_; }


void AuxDataEditor::setSelectionPolygonRectangle( bool yn )
{ polygonselrect_ = yn; }


bool AuxDataEditor::getSelectionPolygonRectangle() const
{ return polygonselrect_; }


void AuxDataEditor::setSelectionPolygonLineStyle( const LineStyle& lst )
{
    polygonsellst_ = lst;

    for ( int idx=0; idx<polygonsel_.size(); idx++ )
	polygonsel_[idx]->linestyle_ = lst;

    if ( polygonsel_.size() )
	viewer_.handleChange( Viewer::Annot );
}


const LineStyle& AuxDataEditor::getSelectionPolygonLineStyle() const
{
    return polygonsellst_;
}


void AuxDataEditor::getPointSelections( TypeSet<int>& ids,
	                                TypeSet<int>& idxs) const
{
    getPointSelections( polygonsel_, ids, idxs );
}



void AuxDataEditor::getPointSelections(
	const ObjectSet<Annotation::AuxData>& polygonsel,
	TypeSet<int>& ids, TypeSet<int>& idxs) const
{
    ids.erase();
    idxs.erase();
    RCol2Coord polytrans;
    polytrans.set3Pts( curview_.topLeft(), curview_.topRight(),
	    curview_.bottomLeft(), 
	    RowCol( mousearea_.topLeft().x, mousearea_.topLeft().y ),
	    RowCol( mousearea_.topRight().x, mousearea_.topRight().y ),
	    mousearea_.bottomLeft().y );

    for ( int idx=0; idx<polygonsel.size(); idx++ )
    {
	if ( polygonsel[idx]->poly_.size()<3 )
	    continue;

	TypeSet<Geom::Point2D<int> > displayselpoly;
	for ( int idy=0; idy<polygonsel[idx]->poly_.size(); idy++ )
	{
	    const RowCol& rc =
		polytrans.transformBack(polygonsel[idx]->poly_[idy]);
	    displayselpoly += Geom::Point2D<int>( rc.row, rc.col );
	}

	ODPolygon<int> polygon( displayselpoly );

	for ( int idy=0; idy<auxdata_.size(); idy++ )
	{
	    const int auxdataid = ids_[idy];
	    const Rect wr = getWorldRect( auxdataid );
	    RCol2Coord trans;
	    trans.set3Pts( wr.topLeft(), wr.topRight(),wr.bottomLeft(), 
		       RowCol( mousearea_.topLeft().x, mousearea_.topLeft().y ),
		       RowCol( mousearea_.topRight().x,mousearea_.topRight().y),
		       mousearea_.bottomLeft().y );

	    for ( int idz=0; idz<auxdata_[idy]->poly_.size(); idz++ )
	    {
		const RowCol& rc =
		    trans.transformBack(auxdata_[idy]->poly_[idz]);
		const Geom::Point2D<int> testpos( rc.row, rc.col );

		if ( !polygon.isInside( testpos, true, 1 ) )
		    continue;

		ids += auxdataid;
		idxs += idz;
	    }
	}
    }
}


const TypeSet<int>& AuxDataEditor::getIds() const
{ return ids_; }


const ObjectSet<Annotation::AuxData>&
AuxDataEditor::getAuxData() const
{ return auxdata_; }


void AuxDataEditor::removePolygonSelected( int dataid )
{
    TypeSet<int> ids;
    TypeSet<int> idxs;


    ObjectSet<Annotation::AuxData> polygonsel;
    deepCopy( polygonsel, polygonsel_ );

    getPointSelections( polygonsel, ids, idxs );

    while ( ids.size() )
    {
	const int curdataid = ids[0];
	if ( dataid==-1 || curdataid==dataid )
	{
	    selptidx_.erase();
	    for ( int idy=ids.size()-1; idy>=0; idy-- )
	    {
		if ( ids[idy]==curdataid )
		{
		    selptidx_ += idxs[idy];
		    ids.remove( idy );
		    idxs.remove( idy );
		}
	    }

	    seldatasetidx_ = ids_.indexOf( curdataid );
	    removeSelected.trigger( !ids.size(), this );

	    if ( !ids.size() )
		break;

	    getPointSelections( polygonsel, ids, idxs );
	    //Update, since ids may have changed.
	}
	else
	{
	    ids.remove( 0 );
	    idxs.remove( 0 );
	}
    }
}


void AuxDataEditor::setMenuHandler( MenuHandler* mh )
{
    if ( menuhandler_ )
	menuhandler_->unRef();

    menuhandler_ = mh;

    if ( menuhandler_ )
	menuhandler_->ref();
}


MenuHandler* AuxDataEditor::getMenuHandler()
{ return menuhandler_; }


void AuxDataEditor::mousePressCB( CallBacker* cb )
{
    if ( mousehandler_.isHandled() || !viewer_.appearance().annot_.editable_ ) 
	return; 

    const MouseEvent& ev = mousehandler_.event(); 
    if ( !(ev.buttonState() & OD::LeftButton ) &&
	  !(ev.buttonState() & OD::MidButton ) &&
	  (ev.buttonState() & OD::RightButton ) )
    {
	if ( !menuhandler_ )
	    return;

	mousehandler_.setHandled( true );
	menuhandler_->executeMenu();
	return;
    }

    if ( !(ev.buttonState() & OD::LeftButton ) ||
	  (ev.buttonState() & OD::MidButton ) ||
	  (ev.buttonState() & OD::RightButton ) )
	return;

    if ( mousedown_ )
    {
	mouseReleaseCB( cb );
	return;
    }

    prevpt_ = ev.pos();

    updateSelection( ev.pos() );

    if ( seldatasetidx_!=-1 )
    {
	const Rect wr = getWorldRect( ids_[seldatasetidx_] );
	RCol2Coord trans;
	trans.set3Pts( wr.topLeft(), wr.topRight(),wr.bottomLeft(), 
		   RowCol( mousearea_.topLeft().x, mousearea_.topLeft().y ),
		   RowCol( mousearea_.topRight().x, mousearea_.topRight().y ),
		   mousearea_.bottomLeft().y );

	selptcoord_ = selptidx_.size()
	    ? (FlatView::Point) auxdata_[seldatasetidx_]->poly_[selptidx_[0]]
	    : (FlatView::Point) trans.transform(RowCol(ev.pos().x,ev.pos().y) );
    }

    hasmoved_ = false;
    mousedown_ = true;
    
    if ( seldatasetidx_!=-1 || !(ev.ctrlStatus() || ev.shiftStatus()) ||
	 ev.altStatus() )
    {
	if ( removeSelectionPolygon() ) viewer_.handleChange( Viewer::Annot );
    }

    if ( seldatasetidx_!=-1 || addauxdataid_!=-1 )
    {
	limitMovement( 0 );
	movementStarted.trigger();
	mousehandler_.setHandled( true );
    }
}


void AuxDataEditor::mouseReleaseCB( CallBacker* cb )
{
    if ( !mousedown_ || !viewer_.appearance().annot_.editable_ )
	return;

    if ( mousehandler_.isHandled() ) 
	return; 

    const MouseEvent& ev = mousehandler_.event(); 
    if ( !(ev.buttonState() & OD::LeftButton ) ||
	  (ev.buttonState() & OD::MidButton ) ||
	  (ev.buttonState() & OD::RightButton ) )
	return;

    //Add
    if ( !hasmoved_ && !ev.ctrlStatus() && !ev.shiftStatus() &&
	 !ev.altStatus() && seldatasetidx_==-1 )
    {
	seldatasetidx_ = ids_.indexOf( addauxdataid_ );
	if ( seldatasetidx_!=-1 && allowadd_[seldatasetidx_] )
	{
	    const Rect wr = getWorldRect(ids_[seldatasetidx_]);
	    RCol2Coord trans;
	    trans.set3Pts( wr.topLeft(), wr.topRight(), wr.bottomLeft(), 
		   RowCol( mousearea_.topLeft().x, mousearea_.topLeft().y ),
		   RowCol( mousearea_.topRight().x, mousearea_.topRight().y ),
		   mousearea_.bottomLeft().y );

	    selptcoord_ = trans.transform( RowCol(ev.pos().x,ev.pos().y) );
	    movementFinished.trigger();
	    mousehandler_.setHandled( true );
	}

	mousedown_ = false;
	return;
    }


    //Remove
    if ( !hasmoved_ && ev.ctrlStatus() && !ev.shiftStatus() &&
	 !ev.altStatus() && seldatasetidx_!=-1 &&
	 allowremove_[seldatasetidx_] && selptidx_.size() )
    {
	removeSelected.trigger( true, this );

	if ( doedit_[seldatasetidx_] )
	{
	    auxdata_[seldatasetidx_]->poly_.remove( selptidx_[0] );
	    viewer_.handleChange( Viewer::Annot );
	}

	mousehandler_.setHandled( true );
	mousedown_ = false;
	return;
    }

    if ( feedback_ )
    {
	viewer_.appearance().annot_.auxdata_ -= feedback_;
	delete feedback_;
	feedback_ = 0;

	if ( selptidx_.size() )
	    viewer_.appearance().annot_.auxdata_ += auxdata_[seldatasetidx_];

	viewer_.handleChange( Viewer::Annot );
    }
    
    //Movement of existing position
    if ( seldatasetidx_!=-1 && selptidx_.size() && hasmoved_ )
    {
	mousehandler_.setHandled( true );
	movementFinished.trigger();
    }


    //Selection polygon movement
    if ( seldatasetidx_==-1 && !selptidx_.size() && hasmoved_ )
    {
	mousehandler_.setHandled( true );
	movementFinished.trigger();
    }

    mousedown_ = false;
}


void AuxDataEditor::mouseMoveCB( CallBacker* cb )
{
    if ( !mousedown_ || mousehandler_.isHandled() ||
         !viewer_.appearance().annot_.editable_ ) 
	return; 

    if ( seldatasetidx_!=-1 && !allowmove_[seldatasetidx_] )
	return;

    const MouseEvent& ev = mousehandler_.event(); 
    if ( !(ev.buttonState() & OD::LeftButton ) ||
	  (ev.buttonState() & OD::MidButton ) ||
	  (ev.buttonState() & OD::RightButton ) )
	return;

    if ( seldatasetidx_!=-1 )
    {
	if ( ev.ctrlStatus() || ev.shiftStatus() || ev.altStatus() )
	    return;

	const Rect wr = getWorldRect(ids_[seldatasetidx_]);
	RCol2Coord trans;
	trans.set3Pts( wr.topLeft(), wr.topRight(), wr.bottomLeft(), 
	       RowCol( mousearea_.topLeft().x, mousearea_.topLeft().y ),
	       RowCol( mousearea_.topRight().x, mousearea_.topRight().y ),
	       mousearea_.bottomLeft().y );

	const Geom::Point2D<int> mousedisplaypos =
	    mousearea_.moveInside(ev.pos());

	selptcoord_ = trans.transform(
		RowCol(mousedisplaypos.x,mousedisplaypos.y ) );

	if ( movementlimit_ )
	    selptcoord_ = movementlimit_->moveInside( selptcoord_ );

	if ( doedit_[seldatasetidx_]  && selptidx_.size() )
	    auxdata_[seldatasetidx_]->poly_[selptidx_[0]] = selptcoord_;
	else if ( !feedback_ )
	{
	    feedback_ = new Annotation::AuxData( *auxdata_[seldatasetidx_] );
	    viewer_.appearance().annot_.auxdata_ += feedback_;
	    if ( !selptidx_.size() )
	    {
		feedback_->poly_.erase();
		feedback_->poly_ += selptcoord_;
	    }
	    else
		viewer_.appearance().annot_.auxdata_ -=
		    		     auxdata_[seldatasetidx_];
	}
	else if ( !selptidx_.size() )
	    feedback_->poly_[0] = selptcoord_;
	else 
	    feedback_->poly_[selptidx_[0]] = selptcoord_;

	viewer_.handleChange( Viewer::Annot );
	mousehandler_.setHandled( true );
    }
    else if ( addauxdataid_!=-1 )
    {
	RCol2Coord trans;
	trans.set3Pts( curview_.topLeft(), curview_.topRight(),
		curview_.bottomLeft(), 
		RowCol( mousearea_.topLeft().x, mousearea_.topLeft().y ),
		RowCol( mousearea_.topRight().x, mousearea_.topRight().y ),
		mousearea_.bottomLeft().y );

	if ( (!hasmoved_ && !ev.shiftStatus()) || !polygonsel_.size() )
	{
	    Annotation::AuxData* polysel = new Annotation::AuxData( 0 );
	    polysel->linestyle_ = polygonsellst_;
	    polysel->fillcolor_.setTransparency( 255 );
	    polysel->poly_ += trans.transform( RowCol(prevpt_.x,prevpt_.y) );
	    polygonsel_ += polysel;
	    viewer_.appearance().annot_.auxdata_ += polysel;
	}

	const int polyidx = polygonsel_.size()-1;

	const Point pt = trans.transform( RowCol(ev.pos().x,ev.pos().y) );
	if ( isselactive_ )
	{
	    if ( polygonselrect_ )
	    {
		if ( polygonsel_[polyidx]->poly_.size()>1 )
		{
		    polygonsel_[polyidx]->poly_.remove( 1,
			    polygonsel_[polyidx]->poly_.size()-1 );
		}

		const Point& startpt = polygonsel_[polyidx]->poly_[0];

		polygonsel_[polyidx]->poly_ += Point(pt.x,startpt.y);
		polygonsel_[polyidx]->poly_ += pt;
		polygonsel_[polyidx]->poly_ += Point(startpt.x,pt.y);
		polygonsel_[polyidx]->close_ = true;
		viewer_.handleChange( Viewer::Annot );
	    }
	    else
	    {
		polygonsel_[polyidx]->poly_ += pt;
		if ( polygonsel_[polyidx]->poly_.size()==3 )
		    polygonsel_[polyidx]->close_ = true;
		viewer_.handleChange( Viewer::Annot );
	    }
	}

	    prevpt_ = ev.pos();
	mousehandler_.setHandled( true );
    }

    hasmoved_ = true;
}


bool AuxDataEditor::updateSelection( const Geom::Point2D<int>& pt )
{
    seldatasetidx_ = -1;
    selptidx_.erase();

    int minsqdist;
    for ( int idx=0; idx<auxdata_.size(); idx++ )
    {
	if ( !auxdata_[idx] )
	    continue;

	const int nrmarkerstyles = auxdata_[idx]->markerstyles_.size();
	if ( !nrmarkerstyles )
	    continue;

	const Rect wr = getWorldRect( ids_[idx] );
	RCol2Coord transform;
	transform.set3Pts( wr.topLeft(), wr.topRight(), wr.bottomLeft(), 
	       RowCol( mousearea_.topLeft().x, mousearea_.topLeft().y ),
	       RowCol( mousearea_.topRight().x, mousearea_.topRight().y ),
	       mousearea_.bottomLeft().y );

	const TypeSet<Point>& dataset = auxdata_[idx]->poly_;

	for ( int idy=0; idy<dataset.size(); idy++ )
	{
	    const RowCol rc = transform.transformBack( dataset[idy] );
	    const Geom::Point2D<int> displaypos( rc.row, rc.col );

	    const int markeridx = mMIN(idy,nrmarkerstyles-1);

	    const int rng = auxdata_[idx]->markerstyles_[markeridx].size_;
	    const Geom::PixRectangle<int> markerrect( pt.x-rng, pt.y-rng,
						  pt.x+rng, pt.y+rng );
	    if ( !markerrect.isInside( displaypos ) )
		continue;

	    const int sqdist = displaypos.sqDistTo( pt );
	    if ( seldatasetidx_==-1 || sqdist<minsqdist )
	    {
		seldatasetidx_ = idx;
		selptidx_ += idy;
		minsqdist = sqdist;
	    }
	}
    }

    return seldatasetidx_!=-1;

}


Rect AuxDataEditor::getWorldRect( int id ) const
{
    Rect res( curview_ );
    const int idx = ids_.indexOf( id );

    if ( idx>=0 )
    {
	if ( auxdata_[idx]->x1rg_ )
	    res.setLeftRight( *auxdata_[idx]->x1rg_ );

	if ( auxdata_[idx]->x2rg_ )
	    res.setTopBottom( *auxdata_[idx]->x2rg_ );
    }

    return res;
}


void AuxDataEditor::limitMovement( const Rect* r )
{
    delete movementlimit_;
    movementlimit_ = r ? new Rect(*r) : 0;
}


};
