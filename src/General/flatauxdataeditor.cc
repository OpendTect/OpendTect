/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          July 2000
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "flatauxdataeditor.h"

#include "bendpointfinder.h"
#include "mousecursor.h"
#include "mouseevent.h"
#include "menuhandler.h"
#include "settings.h"
#include "polygon.h"
#include "timefun.h"


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
    , addAuxDataChange( this )
    , seldatasetidx_( -1 )
    , polygonsellst_( LineStyle::Solid, 1, Color( 255, 0, 0 ) )
    , polygonselrect_( true )
    , isselactive_( true )
    , movementlimit_( 0 )
    , menuhandler_( 0 )
    , sower_( new Sower(*this,meh) )
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
	delete viewer_.removeAuxData( feedback_ );
	feedback_ = 0;
    }

    delete sower_;
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
	viewer_.removeAuxData( polygonsel_[idx] );

    deepErase( polygonsel_ );

    return true;
}


int AuxDataEditor::addAuxData( FlatView::AuxData* nd, bool doedit )
{
    bool found = false;
    for ( int idx=viewer_.nrAuxData()-1; idx>=0; idx-- )
    {
	if ( viewer_.getAuxData(idx)==nd )
	{
	    found = true;
	    break;
	}
    }

    if ( !found ) 
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
    allowpolysel_ +=true;

    return res;
}


void AuxDataEditor::removeAuxData( int id )
{
    const int idx = ids_.indexOf( id );
    if ( idx<0 )
	return;

    ids_.remove( idx );
    auxdata_.remove( idx );
    if ( auxdata_.size() == 0 )
	seldatasetidx_ = -1;
    allowadd_.remove( idx );
    allowmove_.remove( idx );
    allowremove_.remove( idx );
    doedit_.remove( idx );
    allowpolysel_.remove( idx );
}


void AuxDataEditor::enableEdit( int id, bool allowadd, bool allowmove,
			        bool allowdelete )
{
    const int idx = ids_.indexOf( id );

    allowadd_[idx] = allowadd;
    allowmove_[idx] = allowmove;
    allowremove_[idx] = allowdelete;
}


void AuxDataEditor::enablePolySel( int id, bool allowsel )
{
    const int idx = ids_.indexOf( id );

    allowpolysel_[idx] = allowsel;
}


void AuxDataEditor::setAddAuxData( int id )
{
    bool change = addauxdataid_!=id;
    
    addauxdataid_ = id;
    if ( removeSelectionPolygon() ) viewer_.handleChange( Viewer::Annot );

    if ( change )
	addAuxDataChange.trigger();
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

    sower_->setView( wv, mouserect );
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


#define mGetRCol2CoordTransform( trans, view, mousearea ) \
    RCol2Coord trans; \
    trans.set3Pts( view.topLeft(), view.topRight(), view.bottomLeft(), \
		   RowCol( mousearea.topLeft().x, mousearea.topLeft().y ), \
		   RowCol( mousearea.topRight().x, mousearea.topRight().y ), \
		   mousearea.bottomLeft().y );


void AuxDataEditor::getPointSelections(
	const ObjectSet<AuxData>& polygonsel,
	TypeSet<int>& ids, TypeSet<int>& idxs) const
{
    ids.erase();
    idxs.erase();
    mGetRCol2CoordTransform( polytrans, curview_, mousearea_ );

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
	    if ( !allowpolysel_[idy] ) continue;

	    const int auxdataid = ids_[idy];
	    const Rect wr = getWorldRect( auxdataid );
	    mGetRCol2CoordTransform( trans, wr, mousearea_ );

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


const ObjectSet<AuxData>& AuxDataEditor::getAuxData() const
{ return auxdata_; }


void AuxDataEditor::removePolygonSelected( int dataid )
{
    TypeSet<int> ids;
    TypeSet<int> idxs;


    ObjectSet<AuxData> polygonsel;
    deepCopyClone( polygonsel, polygonsel_ );

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
	mGetRCol2CoordTransform( trans, wr, mousearea_ );

	selptcoord_ = selptidx_.size() && seldatasetidx_<auxdata_.size() &&
		      selptidx_[0]<auxdata_[seldatasetidx_]->poly_.size()
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

    if ( !mousehandler_.hasEvent() || mousehandler_.isHandled() ) 
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
	    mGetRCol2CoordTransform( trans, wr, mousearea_ );

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

	if ( seldatasetidx_<doedit_.size() && doedit_[seldatasetidx_] )
	{
	    const int selidx = selptidx_[0];
	    auxdata_[seldatasetidx_]->poly_.remove( selidx ); 
	    auxdata_[seldatasetidx_]->markerstyles_.remove( selidx );
	    viewer_.handleChange( Viewer::Annot );
	}

	mousehandler_.setHandled( true );
	mousedown_ = false;
	return;
    }

    if ( feedback_ )
    {
	delete viewer_.removeAuxData( feedback_ );
	feedback_ = 0;

	if ( selptidx_.size() )
	    viewer_.addAuxData( auxdata_[seldatasetidx_] );

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

    prevpt_ = ev.pos();

    updateSelection( ev.pos() );

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
	mGetRCol2CoordTransform( trans, wr, mousearea_ );

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
	    feedback_ = auxdata_[seldatasetidx_]->clone();
	    viewer_.addAuxData( feedback_ );
	    if ( !selptidx_.size() )
	    {
		feedback_->poly_.erase();
		feedback_->poly_ += selptcoord_;
	    }
	    else
		viewer_.removeAuxData( auxdata_[seldatasetidx_] );
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
	mGetRCol2CoordTransform( trans, curview_, mousearea_ );

	if ( (!hasmoved_ && !ev.shiftStatus()) || !polygonsel_.size() )
	{
	    AuxData* polysel = viewer_.createAuxData( 0 );
	    polysel->linestyle_ = polygonsellst_;
	    polysel->fillcolor_.setTransparency( 255 );
	    //polysel->poly_ += trans.transform( RowCol(prevpt_.x,prevpt_.y) );
	    polygonsel_ += polysel;
	    viewer_.addAuxData( polysel );
	}

	const int polyidx = polygonsel_.size()-1;

	const Point pt = trans.transform( RowCol(ev.pos().x,ev.pos().y) );
	if ( isselactive_ )
	{

	    if ( polygonselrect_ )
	    {
		polygonsel_[polyidx]->poly_ += pt;
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


void AuxDataEditor::findSelection( const Geom::Point2D<int>& pt,
				   int& seldatasetidx,
				   TypeSet<int>* selptidxlist ) const
{
    seldatasetidx = -1;
    if ( selptidxlist )
	selptidxlist->erase();

    int minsqdist;
    for ( int idx=0; idx<auxdata_.size(); idx++ )
    {
	if ( !auxdata_[idx] )
	    continue;

	const int nrmarkerstyles = auxdata_[idx]->markerstyles_.size();
	if ( !nrmarkerstyles )
	    continue;

	const Rect wr = getWorldRect( ids_[idx] );
	mGetRCol2CoordTransform( transform, wr, mousearea_ );

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
	    if ( seldatasetidx==-1 || sqdist<minsqdist )
	    {
		seldatasetidx = idx;
		if ( selptidxlist )
		    *selptidxlist += idy;
		minsqdist = sqdist;
	    }
	}
    }
}


bool AuxDataEditor::updateSelection( const Geom::Point2D<int>& pt )
{
    findSelection( pt, seldatasetidx_, &selptidx_ );
    return seldatasetidx_!=-1;
}


int AuxDataEditor::dataSetIdxAt( const Geom::Point2D<int>& pt ) const
{
    int datasetidx;
    findSelection( pt, datasetidx, 0 );
    return datasetidx;
}


const Point* AuxDataEditor::markerPosAt(
				    const Geom::Point2D<int>& mousepos ) const
{
    if ( sower_->mode() == Sower::SequentSowing )
	return 0;

    int datasetidx;
    TypeSet<int> selptidxlist;
    findSelection( mousepos, datasetidx, &selptidxlist );

    if ( !selptidxlist.size() || datasetidx<0 || datasetidx>=auxdata_.size() )
	return 0;

    const int selptidx = selptidxlist[selptidxlist.size()-1];

    if ( selptidx<0 || selptidx>=auxdata_[datasetidx]->poly_.size() )
	return 0;

    return &auxdata_[datasetidx]->poly_[selptidx];
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


Sower::Sower( AuxDataEditor& ade, MouseEventHandler& meh )
    : editor_( ade )
    , mouseeventhandler_( meh )
    , mode_( Idle )
    , singleseeded_( true )
    , curknotid_( -1 )
    , curknotstamp_( mUdf(int) )
{
    sowingline_ = editor_.viewer().createAuxData( 0 );
    editor_.viewer().addAuxData( sowingline_ );
    reInitSettings();
}


Sower::~Sower()
{
    deepErase( eventlist_ );
    delete editor_.viewer().removeAuxData( sowingline_ );
}


void Sower::reInitSettings()
{
    reversesowingorder_ = false;
    alternatesowingorder_ = false;
    intersow_ = false;

    setIfDragInvertMask( false );
    setSequentSowMask();
    setLaserMask();
    setEraserMask();
}


void Sower::reverseSowingOrder( bool yn )
{ reversesowingorder_ = yn; }


void Sower::alternateSowingOrder( bool yn )
{ alternatesowingorder_ = yn; }


void Sower::intersow( bool yn )
{ intersow_ = yn; }


void Sower::setView( const Rect& curview,const Geom::Rectangle<int>& mousearea )
{
    mGetRCol2CoordTransform( trans, curview, mousearea );
    transformation_ = trans;
    mouserectangle_ = mousearea;
}


#define mReturnHandled( yn ) \
{ \
    mouseeventhandler_.setHandled( yn ); \
    return yn; \
}


bool Sower::activate( const Color& color, const MouseEvent& mouseevent )
{
    if ( mode_ != Idle )
	mReturnHandled( false );

    if ( mouseevent.rightButton() )
	mReturnHandled( false );

    if ( editor_.markerPosAt(mouseevent.pos()) )
	mReturnHandled( false );

    if ( editor_.isSelActive() )
	mReturnHandled( false );

    mode_ = Furrowing;
    furrowstamp_ = Time::getMilliSeconds();

    if ( !accept(mouseevent, false) )
    {
	mode_ = Idle;
	mReturnHandled( false );
    }

    sowingline_->linestyle_ = LineStyle( LineStyle::Solid, 1, color );
    sowingline_->enabled_ = true;

    mReturnHandled( true );
}


Geom::Point2D<int> Sower::pivotPos() const
{
    if ( mode_<FirstSowing || eventlist_.isEmpty() )
	return Geom::Point2D<int>::udf();

    Geom::Point2D<int> sum = eventlist_[0]->pos();
    sum += eventlist_[eventlist_.size()-1]->pos();

    return sum/2;
}


bool Sower::moreToSow() const
{ return mode_>=FirstSowing && bendpoints_.size()>1; }


void Sower::stopSowing()
{ bendpoints_.erase(); }


bool Sower::accept( const MouseEvent& mouseevent, bool released )
{
    if ( mouseevent.tabletInfo() )
	return acceptTablet( mouseevent, released );

    return acceptMouse( mouseevent, released );
}


#define mRehandle( mouseeventhandler, mouseevent, tityp, tbtyp ) \
    if ( (mouseevent).tabletInfo() ) \
	(mouseevent).tabletInfo()->eventtype_ = TabletInfo::tityp; \
    mouseeventhandler.triggerButton##tbtyp( mouseevent ); 


bool Sower::acceptMouse( const MouseEvent& mouseevent, bool released )
{
    if ( mode_==Idle && released && !editor_.isDragging() &&
	 !editor_.markerPosAt(mouseevent.pos()) )
	mReturnHandled( true );

    if ( mode_ != Furrowing )
	mReturnHandled( false );

    const int sz = eventlist_.size();
    if ( !released )
    {
	if ( mouserectangle_.isOutside(mouseevent.pos()) )
	    mReturnHandled( true );

	if ( sz && mouseevent.pos()==eventlist_[sz-1]->pos() )
	    mReturnHandled( true );

	const RowCol rc = RowCol( mouseevent.x(), mouseevent.y() );
	const Point pt = transformation_.transform( rc );
	sowingline_->poly_ += pt;
	if ( sowingline_->poly_.size() == 1 )	    // Do not want the marker  
	    sowingline_->poly_ += pt;		    // from one-point polyline

	editor_.viewer().handleChange( Viewer::Annot );

	const Coord mousepos( mouseevent.x(), mouseevent.y() );

	if ( !sz )
	    singleseeded_ = true;
	else
	{
	    const Coord prevpos( eventlist_[0]->x(), eventlist_[0]->y() );
	    if ( mousepos.distTo(prevpos) > 5 )
		singleseeded_ = false;
	}

	eventlist_ += new MouseEvent( mouseevent );
	mousecoords_ += mousepos;
	mReturnHandled( true );
    }

    if ( !sz || !sowingline_->enabled_ )
    {
	reset();
	mReturnHandled( true );
    }

    MouseCursorChanger mousecursorchanger( MouseCursor::Wait );

    if ( Time::passedSince(furrowstamp_) < 200 )
	singleseeded_ = true;

    int butstate = eventlist_[0]->buttonState();
    if ( !singleseeded_ )
	butstate ^= ifdraginvertmask_;

    eventlist_[0]->setButtonState( (OD::ButtonState) butstate );
    butstate &= sequentsowmask_;

    for ( int idx=sz-1; idx>0; idx--)
    {
	if ( singleseeded_ )
	{
	    eventlist_.remove( idx );
	    mousecoords_.remove( idx );
	}
	else
	    eventlist_[idx]->setButtonState( (OD::ButtonState) butstate );
    }

    const bool intersowing = intersow_ && eventlist_.size()>1;

    float bendthreshold = 2.0;
    mSettUse( get, "dTect.Seed dragging", "Bend threshold", bendthreshold );
    if ( intersowing )
	bendthreshold *= 2;
    if ( bendthreshold < 0.1 )
	bendthreshold = 0.1;

    BendPointFinder2D bpfinder( mousecoords_, bendthreshold );
    bpfinder.execute( true );

    bendpoints_.erase();

    const int last = intersowing ? eventlist_.size()-1
				 : bpfinder.bendPoints().size()-1;

    for ( int idx=0; idx<=last; idx++ )
    {
	int eventidx = idx;
	if ( alternatesowingorder_ )
	    eventidx = idx%2 ? last-idx/2 : idx/2;

	bendpoints_ += intersowing ? eventidx : bpfinder.bendPoints()[eventidx];
	if ( intersowing && bpfinder.bendPoints().indexOf(eventidx)>=0 )
	    bendpoints_ += eventidx;
    }

    if ( reversesowingorder_ )
	bendpoints_.reverse();

    if ( intersowing )
	bendpoints_.insert( 1,  bendpoints_[bendpoints_.size()-1] );

    mode_ = FirstSowing;
    int count = 0;
    while ( bendpoints_.size() )
    {
	int idx = bendpoints_[0];

	editor_.mousedown_ = false;	// Dirty but effective.
	mRehandle( mouseeventhandler_, *eventlist_[idx], Press, Pressed );
	mRehandle( mouseeventhandler_, *eventlist_[idx], Release, Released );

	bendpoints_.remove( 0 );

	count++;
	if ( !intersowing || count>2 )
	    mode_ = SequentSowing;
    }

    reset();
    mReturnHandled( true );
}


void Sower::reset()
{
    sowingline_->enabled_ = false;
    sowingline_->poly_.erase();
    editor_.viewer().handleChange( Viewer::Annot );
    deepErase( eventlist_ );
    mousecoords_.erase();

    mode_ = Idle;
}


bool Sower::acceptTablet( const MouseEvent& mouseevent, bool released )
{
    if ( !mouseevent.tabletInfo() )
	mReturnHandled( false );

    int knotid = editor_.dataSetIdxAt( mouseevent.pos() );

    if ( knotid != curknotid_ )
    {
	curknotstamp_ = Time::getMilliSeconds();
	curknotid_ = knotid;
    }

    if ( mouseevent.tabletInfo()->pointertype_ == TabletInfo::Eraser )
    {
	if ( knotid >= 0 )
	    return acceptEraser( mouseevent, released );

	mReturnHandled( true );
    }

    if ( mode_==Idle && !released && !editor_.isDragging() &&
	 knotid>=0 && !mIsUdf(curknotstamp_) &&
	 Time::passedSince(curknotstamp_) > 300 )
    {
	curknotstamp_ = mUdf(int);

	// Dirty but effective
	const bool enablelaser = knotid<editor_.allowmove_.size() &&
				 editor_.allowmove_[knotid];

	if ( enablelaser )
	    return acceptLaser( mouseevent, released );

	mReturnHandled(false);
    }

    if ( knotid>=0 && mode_==Furrowing && singleseeded_ )
	sowingline_->enabled_ = false;

    return acceptMouse( mouseevent, released );
}


bool Sower::acceptLaser( const MouseEvent& mouseevent, bool released )
{
    if ( mode_ != Idle )
	mReturnHandled( false );

    mode_ = Lasering;

    MouseEvent newevent( mouseevent );

    int butstate = newevent.buttonState() | lasermask_;
    newevent.setButtonState( (OD::ButtonState) butstate );

    editor_.mousedown_ = false;		// Dirty but effective.
    mRehandle( mouseeventhandler_, newevent, Press, Pressed );
    mRehandle( mouseeventhandler_, newevent, Release, Released );

    mode_ = Idle;
    mReturnHandled( true );
}


bool Sower::acceptEraser( const MouseEvent& mouseevent, bool released )
{
    if ( mode_ != Idle )
	mReturnHandled( false );

    if ( !released && !mouseevent.tabletInfo()->pressure_ )
	mReturnHandled( false );

    mode_ = Erasing;

    MouseEvent newevent( mouseevent );

    int butstate = newevent.buttonState() | erasermask_;
    newevent.setButtonState( (OD::ButtonState) butstate );

    editor_.mousedown_ = false;		// Dirty but effective.
    mRehandle( mouseeventhandler_, newevent, Press, Pressed );
    mRehandle( mouseeventhandler_, newevent, Release, Released );

    mode_ = Idle;
    mReturnHandled( true );
}


void Sower::setSequentSowMask( bool yn, OD::ButtonState mask )
{ sequentsowmask_ = yn ? mask : OD::ButtonState(~OD::NoButton); }


void Sower::setIfDragInvertMask( bool yn, OD::ButtonState mask )
{ ifdraginvertmask_ = yn ? mask : OD::NoButton; }


void Sower::setLaserMask( bool yn, OD::ButtonState mask )
{ lasermask_ = yn ? mask : OD::NoButton; }


void Sower::setEraserMask( bool yn, OD::ButtonState mask )
{ erasermask_ = yn ? mask : OD::NoButton; }


};
