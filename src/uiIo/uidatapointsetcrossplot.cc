/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2008
 RCS:           $Id: uidatapointsetcrossplot.cc,v 1.48 2009-08-11 07:43:57 cvssatyaki Exp $
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uidatapointsetcrossplot.cc,v 1.48 2009-08-11 07:43:57 cvssatyaki Exp $";

#include "uidatapointsetcrossplot.h"

#include "uidatapointset.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uiprogressbar.h"
#include "uirgbarray.h"
#include "uitaskrunner.h"
#include "uiworld2ui.h"

#include "arraynd.h"
#include "arrayndimpl.h"
#include "coltabsequence.h"
#include "densitycalc.h"
#include "datapointset.h"
#include "envvars.h"
#include "linear.h"
#include "pixmap.h"
#include "mathexpression.h"
#include "mousecursor.h"
#include "polygon.h"
#include "randcolor.h"
#include "rowcol.h"
#include "sorting.h"
#include "settings.h"
#include "task.h"
#include "timer.h"
#include "thread.h"

static const int cMaxPtsForMarkers = 20000;
static const int cMinPtsForDensity = 20000;
static const int cMaxPtsForDisplay = 100000;

uiDataPointSetCrossPlotter::uiDataPointSetCrossPlotter( uiParent* p,
			    uiDataPointSet& uidps,
			    const uiDataPointSetCrossPlotter::Setup& su )
    : uiRGBArrayCanvas(p,*new uiRGBArray(false))
    , dps_(uidps.pointSet())
    , uidps_(uidps)
    , setup_(su)
    , meh_(scene().getMouseEventHandler())
    , mincolid_(1-uidps.pointSet().nrFixedCols())
    , selrow_(-1)
    , x_(*this,uiRect::Bottom)
    , y_(*this,uiRect::Left)
    , y2_(*this,uiRect::Right)
    , selectionChanged( this )
    , pointsSelected( this )
    , removeRequest( this )
    , drawTypeChanged( this )
    , coltabRgChanged( this )
    , doy2_(true)
    , dobd_(false)
    , eachrow_(1)
    , curgrp_(0)
    , selrowisy2_(false)
    , rectangleselection_(true)
    , lsy1_(*new LinStats2D)
    , lsy2_(*new LinStats2D)
    , mathobj_(0)
    , userdefy1lp_(*new LinePars)
    , userdefy2lp_(*new LinePars)
    , yptitems_(0)
    , y2ptitems_(0)
    , selectionpolygonitem_(0)
    , selectionrectitem_(0)
    , regrlineitm_(0)
    , y1userdeflineitm_(0)
    , y2userdeflineitm_(0)
    , selyitems_(0)
    , sely2items_(0)
    , selrectitems_(0)
    , selpolyitems_(0)
    , eachcount_(0)
    , selectable_(false)
    , isy1selectable_(true)
    , isy2selectable_(false)
    , mousepressed_(false)
    , isdensityplot_(false)
    , timer_(*new Timer())
    , trmsg_("Calculating Density" )
{
    enableImageSave();
    enableScrollZoom();
    x_.defaxsu_.style_ = setup_.xstyle_;
    y_.defaxsu_.style_ = setup_.ystyle_;
    y2_.defaxsu_.style_ = setup_.y2style_;
    x_.defaxsu_.border_ = setup_.minborder_;
    y_.defaxsu_.border_ = setup_.minborder_;
    y2_.defaxsu_.border_ = setup_.minborder_;
    
    meh_.buttonPressed.notify( mCB(this,uiDataPointSetCrossPlotter,mouseClick));
    meh_.buttonReleased.notify( mCB(this,uiDataPointSetCrossPlotter,mouseRel));

    reSize.notify( mCB(this,uiDataPointSetCrossPlotter,reSizeDraw) );
    reDrawNeeded.notify( mCB(this,uiDataPointSetCrossPlotter,reDraw) );
    getMouseEventHandler().buttonPressed.notify(
	    mCB(this,uiDataPointSetCrossPlotter,getSelStarPos) );
    getMouseEventHandler().movement.notify(
	    mCB(this,uiDataPointSetCrossPlotter,drawPolygon) );
    getMouseEventHandler().buttonReleased.notify(
	    mCB(this,uiDataPointSetCrossPlotter,itemsSelected) );

    timer_.tick.notify( mCB(this,uiDataPointSetCrossPlotter,reDraw) );
    setStretch( 2, 2 );
    setDragMode( uiGraphicsView::ScrollHandDrag );
    scene().setMouseEventActive( true );
    setScrollBarPolicy( true, uiGraphicsView::ScrollBarAlwaysOff );
    setScrollBarPolicy( false, uiGraphicsView::ScrollBarAlwaysOff );
}


uiDataPointSetCrossPlotter::~uiDataPointSetCrossPlotter()
{
    delete &lsy1_;
    delete &lsy2_;
    if ( yptitems_ ) scene().removeItem( yptitems_ );
    if ( y2ptitems_ ) scene().removeItem( y2ptitems_ );
    if ( selrectitems_ ) scene().removeItem( selrectitems_ );
    if ( selrectitems_ ) scene().removeItem( selrectitems_ );
}

#define mHandleAxisAutoScale(axis) \
    axis.handleAutoScale( uidps_.getRunCalc( axis.colid_ ) ); \

void uiDataPointSetCrossPlotter::setMathObj( MathExpression* mathobj )
{
    if ( mathobj )
	mathobj_ = mathobj->clone();
    else
	mathobj_ = 0;
}


void uiDataPointSetCrossPlotter::reSizeDraw( CallBacker* )
{
    if ( isdensityplot_ )
    {
	trmsg_ = "Calculating Density";
	timer_.start( 1200, true );
    }
    else
	reDraw( 0 );
    return;
}


void uiDataPointSetCrossPlotter::reDraw( CallBacker* )
{
    MouseCursorChanger cursorchanger(MouseCursor::Wait);
    setDraw();
    reDrawSelArea();
    drawContent();
}


void uiDataPointSetCrossPlotter::showY2( bool yn )
{
    if ( y2ptitems_ )
	y2ptitems_->setVisible( yn );
}


bool uiDataPointSetCrossPlotter::isY2Shown() const
{
     return y2_.axis_ && y2ptitems_ && y2ptitems_->isVisible();
}


void uiDataPointSetCrossPlotter::dataChanged()
{
    mHandleAxisAutoScale( x_ )
    mHandleAxisAutoScale( y_ )
    mHandleAxisAutoScale( y2_ )
    calcStats();
    setDraw();
    drawDeSelectedItems();
    drawContent();
}


void uiDataPointSetCrossPlotter::drawDeSelectedItems()
{
    if ( selpolyitems_ )
    {
	selpolyitems_->removeAll( true );
	selectionpolygonitem_ = 0;
    }
    if ( selrectitems_ )
	selrectitems_->removeAll( true );
}


void uiDataPointSetCrossPlotter::removePoint( uiDataPointSet::DRowID rid,
					      bool isy2, int itmidx )
{
    const Interval<int> udfrg( 0, 1 );
    const Interval<int> xpixrg( x_.axis_->pixRange() ),
	  		ypixrg( y_.axis_ ? y_.axis_->pixRange() : udfrg ),
			y2pixrg( y2_.axis_ ? y2_.axis_->pixRange() : udfrg );

    for ( int idx=0; idx<selareaset_.size(); idx++ )
    {
	const SelectionArea* selarea = selareaset_[idx];
	if ( !selarea )
	    continue;

	uiGraphicsItemGroup* curitmgrp = isy2 ? y2ptitems_ : yptitems_ ;
	if ( !curitmgrp )
	    return;
	uiGraphicsItem* item = curitmgrp->getUiItem( itmidx );
	if ( !item )
	    continue;
	uiPoint itempos = item->getPos();
	const bool itmselected = selarea->isInside( itempos );
	if ( isy1selectable_ && !isy2 )
	{
	    if ( itmselected )
	    {
		curitmgrp->getUiItem( itmidx )->setVisible( false );
		BinIDValueSet::Pos pos = dps_.bvsPos(rid);
		float* vals = dps_.bivSet().getVals( pos );
		vals[ dps_.nrFixedCols()+y_.colid_ ] = mUdf(float);
	    }
	}
	if ( isy2selectable_ && y2ptitems_ && isY2Shown() && isy2 )
	{
	    if ( itmselected )
	    {
		curitmgrp->getUiItem( itmidx )->setVisible( false );
		BinIDValueSet::Pos pos = dps_.bvsPos(rid);
		float* vals = dps_.bivSet().getVals( pos );
		vals[ dps_.nrFixedCols()+y2_.colid_ ] = mUdf(float);
	    }
	}
    }
}


void uiDataPointSetCrossPlotter::deleteSelections()
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );
    if ( isdensityplot_ )
    {
	uidps_.setUnsavedChg( true );
	trmsg_ = "Deleting Selected Points";
	drawDensityPlot( true );
    }
    else
    {
	drawData( y_, false, true );
	if ( y2_.axis_ && doy2_ )
	    drawData( y2_ , true, true );
    }
    uidps_.redoAll();
}


void uiDataPointSetCrossPlotter::setSelectionAreas(
	const ObjectSet<SelectionArea>& areaset )
{
    selareaset_ = areaset;
    for ( int idx=0; idx<selareaset_.size(); idx++ )
	setWorldSelArea( idx );
    reDrawSelArea();
    if ( !isdensityplot_ )
	reDrawNeeded.trigger();
}


void uiDataPointSetCrossPlotter::removeSelections()
{
    drawDeSelectedItems();
    selrowcols_.erase();
    deepErase( selareaset_ );
    selyitems_ = 0;
    sely2items_ = 0;
    
    for ( int idx=0; idx<dps_.size(); idx++ )
	dps_.setSelected( idx, false );
}


void uiDataPointSetCrossPlotter::setSelectable( bool y1, bool y2 )
{
    isy1selectable_ = y1;
    isy2selectable_ = y2;
}


void uiDataPointSetCrossPlotter::getSelStarPos( CallBacker* )
{
    if ( !selectable_ )
	return;

    mousepressed_ = true;
    if ( rectangleselection_ )
    {
	selareaset_ += new SelectionArea(
		new uiRect(getCursorPos(),uiSize(0,0)) );
	if ( !selectionrectitem_ )
	    selectionrectitem_ = new uiRectItem( 0, 0, 1, 1 );
	else
	    selectionrectitem_->setRect( 0, 0, 1, 1 );

	selectionrectitem_->setPenColor( isdensityplot_ ? ctab_.markColor()
							: Color(255,0,0) );
    }
    else
    {
	ODPolygon<int>* poly = new ODPolygon<int>;
	selareaset_ += new SelectionArea( poly );
	poly->add( getCursorPos() );
	if ( !selectionpolygonitem_ )
	{
	    selectionpolygonitem_ = new uiPolygonItem();
	    if ( !selpolyitems_ )
	    {
		selpolyitems_ = new uiGraphicsItemGroup();
		scene().addItemGrp( selpolyitems_ );
		selpolyitems_->setZValue( 5 );
	    }
	    
	    selpolyitems_->add( selectionpolygonitem_ );
	}
	selectionpolygonitem_->setPenColor( isdensityplot_ ? ctab_.markColor()
							   : Color(255,0,0) );
    }

    curselarea_ = selareaset_.size() - 1;
}


void uiDataPointSetCrossPlotter::drawPolygon( CallBacker* )
{
    if ( !selectable_ || !mousepressed_ )
	return;

    if ( !rectangleselection_ )
    {
	ODPolygon<int>* poly = selareaset_.validIdx(curselarea_) ?
	    selareaset_[curselarea_]->poly_ : 0;
	if ( poly )
	    poly->add( getCursorPos() );
	
	if ( !selectionpolygonitem_ )
	{
	    selectionpolygonitem_ = new uiPolygonItem();
	    if ( !selpolyitems_ )
	    {
		selpolyitems_ = new uiGraphicsItemGroup();
		scene().addItemGrp( selpolyitems_ );
		selpolyitems_->setZValue( 5 );
	    }
	    selpolyitems_->add( selectionpolygonitem_ );
	}
	selectionpolygonitem_->setPolygon( *poly );
	
    }
}


void uiDataPointSetCrossPlotter::setCTMapper( const ColTab::MapperSetup& setup )
{ ctmapper_.setup_ = setup; }


void uiDataPointSetCrossPlotter::setWorldSelArea( int selareaidx  )
{
    const uiAxisHandler& xah = *x_.axis_;
    const uiAxisHandler& yah = *y_.axis_;

    if ( selareaset_[selareaidx]->type_ == SelectionArea::Rectangle )
    {
	const uiRect* selarea = selareaset_[selareaidx]->rect_;
	uiWorldRect* worldselarea =
	    new uiWorldRect( xah.getVal(selarea->left()),
		    	     yah.getVal(selarea->top()),
			     xah.getVal(selarea->right()),
			     yah.getVal(selarea->bottom()) );
	selareaset_[selareaidx]->worldrect_ = worldselarea;
    }
    else
    {
	ODPolygon<double>* worldpoly = new ODPolygon<double>;
	const ODPolygon<int>* selpoly = selareaset_[selareaidx]->poly_;
	TypeSet<uiPoint> polypts = selpoly->data();
	for (  int idx=0; idx<polypts.size(); idx++ )
	{
	    worldpoly->add( uiWorldPoint(xah.getVal(polypts[idx].x),
					 yah.getVal(polypts[idx].y)) );
	}
	selareaset_[selareaidx]->worldpoly_= worldpoly;
    }
}


void uiDataPointSetCrossPlotter::reDrawSelArea()
{
    const uiAxisHandler& xah = *x_.axis_;
    const uiAxisHandler& yah = *y_.axis_;
    
    int nrrect = 0;
    int nrpoly = 0;
    for ( int idx=0; idx<selareaset_.size(); idx++ )
    {
	SelectionArea* selarea = selareaset_.size() ? selareaset_[idx] : 0;
	if ( !selarea )
	    continue;
	if ( selareaset_[idx]->type_ == SelectionArea::Rectangle )
	{
	    const uiWorldRect* worldselarea = selareaset_[idx]->worldrect_;
	    uiRect* selarea =
		new uiRect( xah.getPix(worldselarea->left()),
			    yah.getPix(worldselarea->top()),
			    xah.getPix(worldselarea->right()),
			    yah.getPix(worldselarea->bottom()) );
	    selareaset_[idx]->rect_ = selarea;
	    if ( !selrectitems_ )
	    {
		selrectitems_ = new uiGraphicsItemGroup();
		scene().addItemGrp( selrectitems_ );
		selrectitems_->setZValue( 5 );
		if ( !selectionrectitem_ )
		    selectionrectitem_ =
			new uiRectItem( selarea->left(), selarea->top(),
					selarea->width(), selarea->height() );
		else
		    selectionrectitem_->setRect( selarea->left(),selarea->top(),
						 selarea->width(),
						 selarea->height() );
		selrectitems_->add( selectionrectitem_ );
		selectionrectitem_ = 0;
	    }
	    else
	    {
		uiGraphicsItem* item = selrectitems_->getUiItem( nrrect );
		mDynamicCastGet( uiRectItem*, rect, item );
		if ( !rect )
		    continue;
		rect->setRect( selarea->left(), selarea->top(),
			       selarea->width(), selarea->height() );
		nrrect ++;
	    }
	}
	else
	{
	    ODPolygon<int>* poly = new ODPolygon<int>;
	    const ODPolygon<double>* worldpoly = selareaset_[idx]->worldpoly_;
	    TypeSet<uiWorldPoint> polypts = worldpoly->data();
	    for (  int nrpts=0; nrpts<polypts.size(); nrpts++ )
	    {
		poly->add( uiPoint(xah.getPix(polypts[nrpts].x),
				   yah.getPix(polypts[nrpts].y)) );
	    }
	    selareaset_[idx]->poly_= poly;
	    
	    uiGraphicsItem* item = selpolyitems_->getUiItem( nrpoly );
	    mDynamicCastGet( uiPolygonItem*, polyitem, item );
	    if ( !polyitem )
		continue;
	    polyitem->setPolygon( *poly );
	    nrpoly ++;
	}
    }
}


void uiDataPointSetCrossPlotter::itemsSelected( CallBacker* )
{
    mousepressed_ = false;
    if ( !selectable_ || !selareaset_.validIdx(curselarea_) )
	return;

    if ( !getMouseEventHandler().event().ctrlStatus() && selareaset_.size()>1 )
    {
	SelectionArea* curselarea = selareaset_[curselarea_];
	selareaset_ -= curselarea;
	curselarea_ = 0;
	removeSelections();

	selareaset_ += curselarea;
    }
    
    if ( rectangleselection_ )
    {
	uiRect selrect = scene().getSelectedArea();
	selareaset_[curselarea_]->rect_->setTopLeft( selrect.topLeft() );
	selareaset_[curselarea_]->rect_->setBottomRight(
		selrect.bottomRight() );
	selectionrectitem_->setRect( selrect.left(), selrect.top(),
				     selrect.width(), selrect.height() );
	if ( !selrectitems_ )
	{
	    selrectitems_ = new uiGraphicsItemGroup();
	    scene().addItemGrp( selrectitems_ );
	    selrectitems_->setZValue( 5 );
	}
	selrectitems_->add( selectionrectitem_ );
	selectionrectitem_ = 0;
    }
    else
    {
	if ( !selectionpolygonitem_ )
	{
	    selectionpolygonitem_ = new uiPolygonItem();
	    selectionpolygonitem_->setPolygon(
		    *selareaset_[curselarea_]->poly_ );
	    selectionpolygonitem_->setPenColor(
		    isdensityplot_ ? ctab_.markColor() : Color(255,0,0) );
	    selpolyitems_->add( selectionpolygonitem_ );
	}
	selectionpolygonitem_ = 0;
    }
	
    setWorldSelArea( curselarea_ );

    if ( !selareaset_[curselarea_]->isValid() )
    {
	selareaset_.remove( curselarea_ );
	return;
    }

    if ( !isdensityplot_ )
	reDrawNeeded.trigger();
}


static void updLS( const TypeSet<float>& inpxvals,
		   const TypeSet<float>& inpyvals,
		   const uiDataPointSetCrossPlotter::AxisData& axdx,
		   const uiDataPointSetCrossPlotter::AxisData& axdy,
		   LinStats2D& ls )
{
    const int inpsz = inpxvals.size();
    if ( inpsz < 2 ) { ls = LinStats2D(); return; }

    int firstxidx = 0, firstyidx = 0;
    if ( axdx.autoscalepars_.doautoscale_ && axdy.autoscalepars_.doautoscale_ )
    {
	firstxidx = (int)(axdx.autoscalepars_.clipratio_ * inpsz + .5);
	firstyidx = (int)(axdy.autoscalepars_.clipratio_ * inpsz + .5);
	if ( firstxidx < 1 && firstyidx < 1 )
	{
	    ls.use( inpxvals.arr(), inpyvals.arr(), inpsz );
	    return;
	}
    }

    Interval<float> xrg; assign( xrg, axdx.axis_->range() );
    if ( firstxidx > 0 )
    {
	TypeSet<float> sortvals( inpxvals );
	sortFor( sortvals.arr(), inpsz, firstxidx );
	xrg.start = sortvals[firstxidx]; xrg.stop = sortvals[inpsz-firstxidx-1];
    }
    Interval<float> yrg; assign( yrg, axdy.axis_->range() );
    if ( firstyidx > 0 )
    {
	TypeSet<float> sortvals( inpyvals );
	sortFor( sortvals.arr(), inpsz, firstyidx );
	yrg.start = sortvals[firstyidx]; yrg.stop = sortvals[inpsz-firstyidx-1];
    }

    TypeSet<float> xvals, yvals;
    for ( int idx=0; idx<inpxvals.size(); idx++ )
    {
	const float x = inpxvals[idx]; const float y = inpyvals[idx];
	if ( xrg.includes(x) && yrg.includes(y) )
	    { xvals += x; yvals += y; }
    }

    ls.use( xvals.arr(), yvals.arr(), xvals.size() );
}


void uiDataPointSetCrossPlotter::calcStats()
{
    if ( !x_.axis_ || (!y_.axis_ && !y2_.axis_) )
	return;

    const Interval<int> udfrg( 0, 1 );
    const Interval<int> xpixrg( x_.axis_->pixRange() ),
	  		ypixrg( y_.axis_ ? y_.axis_->pixRange() : udfrg ),
			y2pixrg( y2_.axis_ ? y2_.axis_->pixRange() : udfrg );
    TypeSet<float> xvals, yvals, x2vals, y2vals;

    for ( uiDataPointSet::DRowID rid=0; rid<dps_.size(); rid++ )
    {
	if ( dps_.isInactive(rid)
	  || (curgrp_ > 0 && dps_.group(rid) != curgrp_) )
	    continue;

	const float xval = uidps_.getVal( x_.colid_, rid, true );
	if ( mIsUdf(xval) ) continue;

	if ( y_.axis_ )
	{
	    const float yval = uidps_.getVal( y_.colid_, rid, true );
	    if ( !mIsUdf(yval) )
		{ xvals += xval; yvals += yval; }
	}
	if ( y2_.axis_ )
	{
	    const float yval = uidps_.getVal( y2_.colid_, rid, true );
	    if ( !mIsUdf(yval) )
		{ x2vals += xval; y2vals += yval; }
	}
    }

    updLS( xvals, yvals, x_, y_, lsy1_ );
    updLS( x2vals, y2vals, x_, y2_, lsy2_ );
}


bool uiDataPointSetCrossPlotter::selNearest( const MouseEvent& ev )
{
    const uiPoint pt( ev.pos() );
    selrow_ = getRow( y_, pt );
    selrowisy2_ = selrow_ < 0;
    if ( selrowisy2_ )
	selrow_ = getRow( y2_, pt );
    return selrow_ >= 0;
}


static const float cRelTol = 0.01; // 1% of axis size


int uiDataPointSetCrossPlotter::getRow(
	const uiDataPointSetCrossPlotter::AxisData& ad, uiPoint pt ) const
{
    if ( !ad.axis_ ) return -1;

    const float xpix = x_.axis_->getRelPos( x_.axis_->getVal(pt.x) );
    const float ypix = ad.axis_->getRelPos( ad.axis_->getVal(pt.y) );
    int row = -1; float mindistsq = 1e30;
    for ( uiDataPointSet::DRowID rid=0; rid<dps_.size(); rid++ )
    {
	if ( (curgrp_ > 0 && dps_.group(rid) != curgrp_) )
	    continue;

	const float xval = uidps_.getVal( x_.colid_, rid, true );
	const float yval = uidps_.getVal( ad.colid_, rid, true );
	if ( mIsUdf(xval) || mIsUdf(yval) ) continue;

	const float x = x_.axis_->getRelPos( xval );
	const float y = ad.axis_->getRelPos( yval );
	const float distsq = (x-xpix)*(x-xpix) + (y-ypix)*(y-ypix);
	if ( distsq < mindistsq )
	    { row = rid; mindistsq = distsq; }
    }

    return mindistsq < cRelTol*cRelTol ? row : -1;
}


void uiDataPointSetCrossPlotter::mouseClick( CallBacker* cb )
{
    if ( meh_.isHandled() ) return;

    const MouseEvent& ev = meh_.event();
    const bool isnorm = !ev.ctrlStatus() && !ev.shiftStatus() &&!ev.altStatus();

    if ( isnorm && selNearest(ev) )
    {
	selectionChanged.trigger();
	meh_.setHandled( true );
    }
}


void uiDataPointSetCrossPlotter::mouseRel( CallBacker* cb )
{
    if ( meh_.isHandled() ) return;

    const MouseEvent& ev = meh_.event();
    const bool isdel = ev.shiftStatus() && !ev.ctrlStatus() && !ev.altStatus();
    
    uiPoint scenepos( getScenePos(ev.x(),ev.y()) );
    MouseEvent sceneev( ev.buttonState(), scenepos.x, scenepos.y );

    if ( !setup_.noedit_ && isdel && selNearest(sceneev) )
    {
	removeRequest.trigger();
	meh_.setHandled( true );
	selrow_ = -1;
    }
}


void uiDataPointSetCrossPlotter::setCols( DataPointSet::ColID x,
		    DataPointSet::ColID y, DataPointSet::ColID y2 )
{
    if ( y < mincolid_ && y2 < mincolid_ )
	x = mincolid_ - 1;
    if ( x < mincolid_ )
	y = y2 = mincolid_ - 1;

    x_.setCol( x ); y_.setCol( y ); y2_.setCol( y2 );

    if ( y_.axis_ )
    {
	y_.axis_->setBegin( x_.axis_ );
	x_.axis_->setBegin( y_.axis_ );
    }
    else if ( x_.axis_ )
	x_.axis_->setBegin( 0 );

    if ( y2_.axis_ )
    {
	y2_.axis_->setBegin( x_.axis_ );
	x_.axis_->setEnd( y2_.axis_ );
    }
    else if ( x_.axis_ )
	x_.axis_->setEnd( 0 );

    mHandleAxisAutoScale(x_);
    mHandleAxisAutoScale(y_);
    mHandleAxisAutoScale(y2_);
    
    calcStats();
    setDraw();
    drawContent();
}


void uiDataPointSetCrossPlotter::initDraw()
{
    x_.newDevSize();
    y_.newDevSize();
    y2_.newDevSize();
}


void uiDataPointSetCrossPlotter::setDraw()
{
    if ( x_.axis_ )
	x_.axis_->setNewDevSize( width(), height() );
    if ( y_.axis_ )
	y_.axis_->setNewDevSize( height(), width() );
    if ( doy2_ && y2_.axis_ )
	y2_.axis_->setNewDevSize( height(), width() );
}


void uiDataPointSetCrossPlotter::mkNewFill()
{
    if ( !dobd_ ) return;
}


void uiDataPointSetCrossPlotter::drawContent( bool withaxis )
{
    if ( withaxis )
    {
	if ( x_.axis_ )
	    x_.axis_->plotAxis();
	if ( y_.axis_ )
	    y_.axis_->plotAxis();
	if ( doy2_ && y2_.axis_ )
	    y2_.axis_->plotAxis();

	if ( !x_.axis_ || !y_.axis_ )
	    return;
    }

    drawData( y_, false );
    if ( y2_.axis_ && doy2_ )
	drawData( y2_, true );
    else if ( y2ptitems_ )
	y2ptitems_->removeAll( true );
    
    pointsSelected.trigger();
}


void uiDataPointSetCrossPlotter::prepareItems( bool y2 )
{
    if ( y2 ? !y2ptitems_ : !yptitems_ )
    {
	if ( y2 )
	{
	    y2ptitems_ = new uiGraphicsItemGroup();
	    scene().addItemGrp( y2ptitems_ );
	}
	else
	{
	    yptitems_ = new uiGraphicsItemGroup();
	    scene().addItemGrp( yptitems_ );
	}
    }
}


void uiDataPointSetCrossPlotter::addItemIfNew( int itmidx,MarkerStyle2D& mstyle,
       					       uiGraphicsItemGroup* curitmgrp,
       					       uiAxisHandler& yah,
       					       uiDataPointSet::DRowID rid,
					       bool isy2 )
{
    if ( itmidx >= curitmgrp->getSize() )
    {
	uiGraphicsItem* itm = 0;
	if ( mstyle.type_ == MarkerStyle2D::None )
	    itm = new uiPointItem();
	else
	{
	    mstyle.size_ = 2;
	    itm = new uiMarkerItem( mstyle, false );
	}

	itm->setPenColor( yah.setup().style_.color_ );
	curitmgrp->add( itm );
    }
}


void uiDataPointSetCrossPlotter::setItem( uiGraphicsItem* item, bool isy2,
	const uiPoint& pt )
{
    item->setPos( pt.x, pt.y ); 
    eachcount_ ++;
    if ( eachcount_ >= eachrow_ )
    {
	item->setVisible( isy2 ? isY2Shown() : true ); 
	eachcount_ = 0;
    }
    else
	item->setVisible( false ); 
}


void uiDataPointSetCrossPlotter::setAnnotEndTxt( uiAxisHandler& yah )
{
    if ( setup_.showcc_ )
    {
	float fr100 = (y_.axis_ == &yah ? lsy1_ : lsy2_).corrcoeff * 100;
	int r100 = mNINT(fr100);
	BufferString txt( "r=" );
	if ( r100 < 0 )
	    { txt += "-"; r100 = -r100; }
	if ( r100 == 0 )	txt += "0";
	else if ( r100 == 100 )	txt += "1";
	else
	{
	    txt += "0.";
	    txt += r100%10 ? r100 : r100 / 10;
	}
	yah.annotAtEnd( txt );
    }
}


bool uiDataPointSetCrossPlotter::isSelectionValid( uiDataPointSet::DRowID rid )
{
    if ( modcolidxs_.size() && mathobj_ )
    {
	for ( int idx=0; idx<modcolidxs_.size(); idx++ )
	{
	    const float yval = uidps_.getVal( modcolidxs_[idx], rid, true );
	    mathobj_->setVariableValue( idx, yval );
	}

	const float result = mathobj_->getValue();
	if ( mIsZero(result,mDefEps) || mIsUdf(result) )
	    return false;
    }
    return true;
}


void uiDataPointSetCrossPlotter::checkSelection( uiDataPointSet::DRowID rid,
	     uiGraphicsItem* item, bool isy2,
	     const uiDataPointSetCrossPlotter::AxisData& yad, bool removesel )
{
    if ( !item ) return;
    bool ptselected = false;
    for ( int idx=0; idx<selareaset_.size(); idx++ )
    {
	SelectionArea* selarea = selareaset_.size() ? selareaset_[idx] : 0;
	if ( !selarea )
	    continue;

	const uiPoint itempos = item->getPos();
	const bool itmselected = selarea->isInside( itempos ) ;
	if ( isy1selectable_ && !isy2 )
	{
	    if ( itmselected )
	    {
		if ( !isSelectionValid(rid) )
		    continue;
		if ( removesel )
		{
		    item->setVisible( false );
		    BinIDValueSet::Pos pos = dps_.bvsPos(rid);
		    float* vals = dps_.bivSet().getVals( pos );
		    vals[ dps_.nrFixedCols()+y_.colid_ ] = mUdf(float);
		    return;
		}
		if ( item->isVisible() )
		{
		    item->setPenColor( Color::DgbColor() );
		    dps_.setSelected( rid, true );
		    selrowcols_ += RowCol( uidps_.tRowID(rid),
					   uidps_.tColID(yad.colid_) );
		
		    selyitems_++;
		    ptselected = true;
		}
	    }
	    else
		dps_.setSelected( rid, false );
	}
	if ( isy2selectable_ && y2ptitems_ && isY2Shown() && isy2 )
	{
	    if ( itmselected )
	    {
		if ( !isSelectionValid(rid) )
		    continue;
		if ( removesel )
		{
		    item->setVisible( false );
		    BinIDValueSet::Pos pos = dps_.bvsPos(rid);
		    float* vals = dps_.bivSet().getVals( pos );
		    vals[ dps_.nrFixedCols()+y2_.colid_ ] = mUdf(float);
		    return;
		}
		if ( item->isVisible() )
		{
		    item->setPenColor( Color(100,230,220) );
		    
		    dps_.setSelected( rid, true );
		    selrowcols_ += RowCol( uidps_.tRowID(rid),
					   uidps_.tColID(yad.colid_) );
		    
		    sely2items_++;
		    ptselected = true;
		}
	    }
	    else
		dps_.setSelected( rid, false );
	}
    }

    if ( !ptselected )
	item->setPenColor( yad.axis_->setup().style_.color_ );
    
}

int uiDataPointSetCrossPlotter::calcDensity( Array2D<float>* data,
					     bool changedps, bool removesel )
{
    const Interval<int> xpixrg( x_.axis_->pixRange() ),
	  		ypixrg( y_.axis_->pixRange() );
    uiWorld2Ui w2ui( uiSize(rgbarr_.getSize(true), rgbarr_.getSize(false)),
	    	     uiWorldRect((double)arrarea_.left(),(double)arrarea_.top(),
			 	 (double)arrarea_.right(),
				 (double)arrarea_.bottom()) );
    if ( !x_.axis_ || !y_.axis_ )
	return -1;
    DensityCalc densitycalc( uidps_, data, x_, y_, selareaset_, trmsg_.buf() );
    densitycalc.setWorld2Ui( w2ui );
    densitycalc.setMathObj( mathobj_ );
    densitycalc.setModifiedColIds( modcolidxs_ );
    densitycalc.setDPSChangeable( changedps );
    densitycalc.setRemSelected( removesel );
    densitycalc.setCurGroup( curgrp_ );
    uiTaskRunner tr( parent() );
    tr.execute( densitycalc );


    selrowcols_ = densitycalc.selRCs();
    selyitems_ = selrowcols_.size();
    const od_int64 totalsz =
	data->info().getSize(true) * data->info().getSize(false);
    
    int indexsz = densitycalc.indexSize();
    ctmapper_.setData( data->getStorage(), totalsz );
    Interval<float> mapperrg =
	Interval<float>(ctmapper_.range().start<1 ? 1 : ctmapper_.range().start,
			ctmapper_.range().stop > indexsz
			? indexsz : ctmapper_.range().stop );
    if ( ctmapper_.range().start<1 || ctmapper_.range().stop > indexsz )
	ctmapper_.setRange( mapperrg );
    return indexsz;
}


void uiDataPointSetCrossPlotter::drawDensityPlot( bool withremovesel )
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );
    isdensityplot_ = true;
    drawTypeChanged.trigger( true );
    const uiAxisHandler& xah = *x_.axis_;
    const uiAxisHandler& yah = *y_.axis_;
    border_ = uiBorder( xah.pixBefore(), yah.pixAfter(),
	    		xah.pixAfter(), yah.pixBefore() );
    beforeDraw();
    if ( arrarea_.left()>arrarea_.right() || arrarea_.top()>arrarea_.bottom() )
	return;

    Array2D<float>* data = new Array2DImpl<float>( arrarea_.width()+ 1,
						   arrarea_.height() + 1);

    data->setAll( (float)0 );

    int indexsz = calcDensity( data, false, withremovesel );
    if ( indexsz < 1 )
	return;

    Interval<float> mapperrg =
	Interval<float>(ctmapper_.range().start<1 ? 1 : ctmapper_.range().start,
			ctmapper_.range().stop > indexsz
			? indexsz : ctmapper_.range().stop );
    coltabRgChanged.trigger( mapperrg );
    for( int idx=0; idx<data->info().getSize(true); idx++ )
    {
	for( int idy=0; idy<data->info().getSize(false); idy++ )
	{
	    if ( !data->info().validPos(idy,idx) )
		continue;
	    const float val = data->get( idy, idx );
	    const float mappedval = ctmapper_.position( (float)val );
	    Color col = ( val == 0 ) ? Color::White() 
				     : ctab_.color( mappedval );
	    if ( col.isVisible() )
		rgbarr_.set( idy, idx, col );
	}
    }

    PtrMan<ioPixmap> pixmap = new ioPixmap( arrarea_.width(),arrarea_.height());
    pixmap->convertFromRGBArray( rgbarr_ );
    setPixmap( *pixmap );
    draw();
}


bool uiDataPointSetCrossPlotter::drawPoints( uiGraphicsItemGroup* curitmgrp,
	const uiDataPointSetCrossPlotter::AxisData& yad, bool isy2,
	MarkerStyle2D& mstyle, bool removesel )
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );
    isdensityplot_ = false;
    drawTypeChanged.trigger( false );
    PtrMan<ioPixmap> pixmap = new ioPixmap( arrarea_.width(),arrarea_.height());
    pixmap->fill( Color::White() );
    setPixmap( *pixmap );
    draw();
    curitmgrp->setVisible( true );

    int nrptsdisp = 0;
    usedxpixrg_ = Interval<int>(0,0);
    uiAxisHandler& xah = *x_.axis_;
    uiAxisHandler& yah = *yad.axis_;
    int itmidx = 0;
    eachcount_=0;
    const Interval<int> xpixrg( xah.pixRange() ), ypixrg( yah.pixRange() );
    for ( uiDataPointSet::DRowID rid=0; rid<dps_.size(); rid++ )
    {
	if ( dps_.isInactive(rid) || (curgrp_>0 && dps_.group(rid)!=curgrp_) )
	    continue;

	const float xval = uidps_.getVal( x_.colid_, rid, true );
	const float yval = uidps_.getVal( yad.colid_, rid, true );
	if ( mIsUdf(xval) || mIsUdf(yval) )
	{
	    eachcount_ ++;
	    if ( eachcount_ >= eachrow_ )
		eachcount_ = 0;
	    continue;
	}

	const uiPoint pt( xah.getPix(xval), yah.getPix(yval) );
	
	if ( !xpixrg.includes(pt.x) || !ypixrg.includes(pt.y) )
	    continue;

	addItemIfNew( itmidx, mstyle, curitmgrp, yah, rid, isy2 );

	setItem( curitmgrp->getUiItem(itmidx), isy2, pt );
	checkSelection( rid, curitmgrp->getUiItem(itmidx), isy2, yad,removesel);
	nrptsdisp++;
	if ( nrptsdisp > 1 )
	    usedxpixrg_.include( pt.x );
	else
	    usedxpixrg_ = Interval<int>( pt.x, pt.x );

	itmidx++;
    }

    if ( itmidx < curitmgrp->getSize() )
    {
	for ( int idx=itmidx; idx<curitmgrp->getSize(); idx++ )
	{
	    curitmgrp->getUiItem(idx)->setVisible(false);
	}
    }
    
    if ( nrptsdisp < 1 )
	return false;
    return true;
}


void uiDataPointSetCrossPlotter::drawData(
    const uiDataPointSetCrossPlotter::AxisData& yad, bool isy2, bool rempts )
{
    uiAxisHandler& xah = *x_.axis_;
    uiAxisHandler& yah = *yad.axis_;

    int estpts = dps_.nrActive() ;
    if ( curgrp_ > 0 ) estpts = cMaxPtsForMarkers;

    MarkerStyle2D mstyle( setup_.markerstyle_ );
    if ( estpts > cMaxPtsForMarkers )
	mstyle.type_ = MarkerStyle2D::None;
    mstyle.color_.setRgb( yah.setup().style_.color_.rgb() );

    prepareItems( isy2 );
    uiGraphicsItemGroup* curitmgrp = isy2 ? y2ptitems_ : yptitems_ ;
    if ( !curitmgrp )
	return;

    if ( estpts > cMinPtsForDensity )
    {
	curitmgrp->setVisible( false );
	if ( !isy2 )
	    drawDensityPlot();
    }
    else if ( !drawPoints(curitmgrp,yad,isy2,mstyle,rempts) )
	return;

    setAnnotEndTxt( yah );
    if ( setup_.showregrline_ )
	drawRegrLine( yah, usedxpixrg_ );
    else if ( regrlineitm_ )
	regrlineitm_->setLine( 0, 0, 0, 0 );
    drawY1UserDefLine( usedxpixrg_, setup_.showy1userdefline_ );
    drawY2UserDefLine( usedxpixrg_, setup_.showy2userdefline_ );
}


void uiDataPointSetCrossPlotter::drawY1UserDefLine( const Interval<int>& xpixrg,
						    bool draw )
{
    if ( !draw )
    {
	if ( y1userdeflineitm_ )
	{
	    scene().removeItem( y1userdeflineitm_ );
	    y1userdeflineitm_ = 0;
	}
	return;
    }

    const uiAxisHandler& xah = *x_.axis_;
    const uiAxisHandler& yah = *y_.axis_;
    Interval<float> xvalrg( xah.getVal(xpixrg.start), xah.getVal(xpixrg.stop) );
    if ( !y1userdeflineitm_ )
    {
	y1userdeflineitm_ = new uiLineItem();
	scene().addItem( y1userdeflineitm_ );
    }
    drawLine( *y1userdeflineitm_, userdefy1lp_, xah, yah, 0);
    y1userdeflineitm_->setPenStyle( y_.defaxsu_.style_ );
}


void uiDataPointSetCrossPlotter::drawY2UserDefLine( const Interval<int>& xpixrg,						    bool draw )
{
    if ( !draw )
    {
	if ( y2userdeflineitm_ )
	{
	    scene().removeItem( y2userdeflineitm_ );
	    y2userdeflineitm_ = 0;
	}
	return;
    }

    const uiAxisHandler& xah = *x_.axis_;
    if ( !y2_.axis_ )
	return;
    const uiAxisHandler& yah = *y2_.axis_;
    Interval<float> xvalrg( xah.getVal(xpixrg.start), xah.getVal(xpixrg.stop) );
    if ( !y2userdeflineitm_ )
    {
	y2userdeflineitm_ = new uiLineItem();
	scene().addItem( y2userdeflineitm_ );
    }
    drawLine( *y2userdeflineitm_, userdefy2lp_, xah, yah, 0 );
    y2userdeflineitm_->setPenStyle( y2_.defaxsu_.style_ );
}


void uiDataPointSetCrossPlotter::drawRegrLine( uiAxisHandler& yah,
					       const Interval<int>& xpixrg )
{
    const uiAxisHandler& xah = *x_.axis_;
    const LinStats2D& ls = y_.axis_ == &yah ? lsy1_ : lsy2_;
    const Interval<int> ypixrg( yah.pixRange() );
    Interval<float> xvalrg( xah.getVal(xpixrg.start), xah.getVal(xpixrg.stop) );
    Interval<float> yvalrg( yah.getVal(ypixrg.start), yah.getVal(ypixrg.stop) );
    if ( !regrlineitm_ )
    {
	regrlineitm_ = new uiLineItem();
	scene().addItem( regrlineitm_ );
    }
    drawLine( *regrlineitm_, ls.lp, xah, yah, &xvalrg );
}

