/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          July 2000
 RCS:           $Id: flatauxdataeditor.cc,v 1.17 2007-10-02 14:14:25 cvskris Exp $
________________________________________________________________________

-*/

#include "flatauxdataeditor.h"

#include "mouseevent.h"
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
    , selptidx_( -1 )
    , polygonsellst_( LineStyle::Solid, 1, Color( 255, 0, 0 ) )
    , polygonselrect_( true )
    , movementlimit_( 0 )
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


int AuxDataEditor::getSelPtIdx() const
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
    RCol2Coord polytrans;
    polytrans.set3Pts( curview_.topLeft(), curview_.topRight(),
	    curview_.bottomLeft(), 
	    RowCol( mousearea_.topLeft().x, mousearea_.topLeft().y ),
	    RowCol( mousearea_.topRight().x, mousearea_.topRight().y ),
	    mousearea_.bottomLeft().y );

    for ( int idx=0; idx<polygonsel_.size(); idx++ )
    {
	if ( polygonsel_[idx]->poly_.size()<3 )
	    continue;

	TypeSet<Geom::Point2D<int> > displayselpoly;
	for ( int idy=0; idy<polygonsel_[idx]->poly_.size(); idy++ )
	{
	    const RowCol& rc =
		polytrans.transformBack(polygonsel_[idx]->poly_[idy]);
	    displayselpoly += Geom::Point2D<int>( rc.row, rc.col );
	}

	//Polygon polygon( displayselpoly );

	for ( int idy=0; idy<auxdata_.size(); idx++ )
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

	//	if ( !polygon.isInside( Geom::Point2D<int>( rc.row, rc.col ) ) )
	//	    continue;

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


void AuxDataEditor::mousePressCB( CallBacker* cb )
{
    if ( mousehandler_.isHandled() ) 
	return; 

    const MouseEvent& ev = mousehandler_.event(); 
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

	selptcoord_ = selptidx_!=-1 ? auxdata_[seldatasetidx_]->poly_[selptidx_]
				    : trans.transform(
					RowCol(ev.pos().x,ev.pos().y ) );
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
    if ( !mousedown_ )
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
	 allowremove_[seldatasetidx_] && selptidx_!=-1 )
    {
	removeSelected.trigger();

	if ( doedit_[seldatasetidx_] )
	{
	    auxdata_[seldatasetidx_]->poly_.remove( selptidx_ );
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

	if ( selptidx_!=-1 )
	    viewer_.appearance().annot_.auxdata_ += auxdata_[seldatasetidx_];

	viewer_.handleChange( Viewer::Annot );
    }

    if ( seldatasetidx_!=-1 && selptidx_!=-1 && hasmoved_ )
    {
	mousehandler_.setHandled( true );
	movementFinished.trigger();
    }

    mousedown_ = false;
}


void AuxDataEditor::mouseMoveCB( CallBacker* cb )
{
    if ( !mousedown_ || mousehandler_.isHandled() ) 
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

	if ( doedit_[seldatasetidx_]  && selptidx_!=-1 )
	    auxdata_[seldatasetidx_]->poly_[selptidx_] = selptcoord_;
	else if ( !feedback_ )
	{
	    feedback_ = new Annotation::AuxData( *auxdata_[seldatasetidx_] );
	    viewer_.appearance().annot_.auxdata_ += feedback_;
	    if ( selptidx_==-1 )
	    {
		feedback_->poly_.erase();
		feedback_->poly_ += selptcoord_;
	    }
	    else
		viewer_.appearance().annot_.auxdata_ -=
		    		     auxdata_[seldatasetidx_];
	}
	else if ( selptidx_==-1 )
	    feedback_->poly_[0] = selptcoord_;
	else 
	    feedback_->poly_[selptidx_] = selptcoord_;

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

	if ( !hasmoved_ && !ev.shiftStatus() || !polygonsel_.size() )
	{
	    Annotation::AuxData* polysel = new Annotation::AuxData( 0 );
	    polysel->markerstyle_.color_.setTransparency( 255 );
	    polysel->linestyle_ = polygonsellst_;
	    polysel->fillcolor_.setTransparency( 255 );
	    polysel->poly_ += trans.transform( RowCol(prevpt_.x,prevpt_.y) );
	    polygonsel_ += polysel;
	    viewer_.appearance().annot_.auxdata_ += polysel;
	}

	const int polyidx = polygonsel_.size()-1;

	const Point pt = trans.transform( RowCol(ev.pos().x,ev.pos().y) );
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

	prevpt_ = ev.pos();
	mousehandler_.setHandled( true );
    }

    hasmoved_ = true;
}


bool AuxDataEditor::updateSelection( const Geom::Point2D<int>& pt )
{
    seldatasetidx_ = -1;
    selptidx_ = -1;

    int minsqdist;
    for ( int idx=0; idx<auxdata_.size(); idx++ )
    {
	if ( !auxdata_[idx] )
	    continue;

	const int rng = auxdata_[idx]->markerstyle_.size_;
	const Geom::PixRectangle<int> markerrect( pt.x-rng, pt.y-rng,
						  pt.x+rng, pt.y+rng );

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
	    if ( !markerrect.isInside( displaypos ) )
		continue;

	    const int sqdist = displaypos.sqDistTo( pt );
	    if ( seldatasetidx_==-1 || sqdist<minsqdist )
	    {
		seldatasetidx_ = idx;
		selptidx_ = idy;
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

    if ( auxdata_[idx]->x1rg_ )
	res.setLeftRight( *auxdata_[idx]->x1rg_ );

    if ( auxdata_[idx]->x2rg_ )
	res.setTopBottom( *auxdata_[idx]->x2rg_ );

    return res;
}


void AuxDataEditor::limitMovement( const Rect* r )
{
    delete movementlimit_;
    movementlimit_ = r ? new Rect(*r) : 0;
}


};
