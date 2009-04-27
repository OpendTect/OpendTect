/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2008
 RCS:           $Id: uidatapointsetcrossplot.cc,v 1.38 2009-04-27 10:38:28 cvssatyaki Exp $
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uidatapointsetcrossplot.cc,v 1.38 2009-04-27 10:38:28 cvssatyaki Exp $";

#include "uidatapointsetcrossplotwin.h"

#include "uibutton.h"
#include "uidpscrossplotpropdlg.h"
#include "uidatapointset.h"
#include "uidialog.h"
#include "uilineedit.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uirgbarray.h"
#include "uicombobox.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uispinbox.h"
#include "uitable.h"
#include "uitoolbar.h"

#include "arraynd.h"
#include "arrayndimpl.h"
#include "draw.h"
#include "datapointset.h"
#include "datainpspec.h"
#include "envvars.h"
#include "linear.h"
#include "mathexpression.h"
#include "mousecursor.h"
#include "polygon.h"
#include "randcolor.h"
#include "rowcol.h"
#include "statruncalc.h"
#include "sorting.h"
#include "settings.h"

static const int cMaxPtsForMarkers = 20000;
static const int cMaxPtsForDisplay = 100000;

uiDataPointSetCrossPlotter::Setup uiDataPointSetCrossPlotWin::defsetup_;

uiDataPointSetCrossPlotter::Setup::Setup()
    : noedit_(false)
    , markerstyle_(MarkerStyle2D::Square)
    , xstyle_(LineStyle::Solid,1,Color::Black())
    , ystyle_(LineStyle::Solid,1,Color::stdDrawColor(0))
    , y2style_(LineStyle::Dot,1,Color::stdDrawColor(1))
    , minborder_(10,20,20,5)
    , showcc_(true)
    , showregrline_(false)
    , showy1userdefline_(false)
    , showy2userdefline_(false)
{
}


uiDataPointSetCrossPlotter::AxisData::AxisData( uiDataPointSetCrossPlotter& cp,
						uiRect::Side s )
    : uiAxisData( s )
    , cp_( cp )
{
}


void uiDataPointSetCrossPlotter::AxisData::stop()
{
    if ( isreset_ ) return;
    colid_ = cp_.mincolid_ - 1;
    uiAxisData::stop();
}


void uiDataPointSetCrossPlotter::AxisData::setCol( DataPointSet::ColID cid )
{
    if ( axis_ && cid == colid_ )
	return;

    stop();
    colid_ = cid;
    newColID();
}


void uiDataPointSetCrossPlotter::AxisData::newColID()
{
    if ( colid_ < cp_.mincolid_ )
	return;

    renewAxis( cp_.uidps_.userName(colid_), &cp_.scene(), cp_.width(),
	       cp_.height(), 0 );
    handleAutoScale( cp_.uidps_.getRunCalc( colid_ ) );
}


uiDataPointSetCrossPlotter::SelectionArea::SelectionArea( uiRect* rect )
    : type_( uiDataPointSetCrossPlotter::SelectionArea::Rectangle )
    , rect_(rect)
    , poly_(0)
{
}


uiDataPointSetCrossPlotter::SelectionArea::SelectionArea( ODPolygon<int>* poly )
    : type_( uiDataPointSetCrossPlotter::SelectionArea::Polygon )
    , rect_(0)
    , poly_(poly)
{
}


uiDataPointSetCrossPlotter::SelectionArea::~SelectionArea()
{
    delete rect_;
    delete poly_;
}


bool uiDataPointSetCrossPlotter::SelectionArea::isInside(
	const uiPoint& pos ) const
{
    return type_ == Polygon ? poly_->isInside(pos,true,0)
			    : rect_->contains( pos );
}


bool uiDataPointSetCrossPlotter::SelectionArea::isValid() const
{
    return type_ == Polygon ? !poly_->isSelfIntersecting()
			    : (rect_->width()>1 && rect_->height()>1);
}


uiDataPointSetCrossPlotter::uiDataPointSetCrossPlotter( uiParent* p,
			    uiDataPointSet& uidps,
			    const uiDataPointSetCrossPlotter::Setup& su )
    : uiGraphicsView(p,"Data pointset crossplotter")
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
    , regrlineitm_(0)
    , y1userdeflineitm_(0)
    , y2userdeflineitm_(0)
    , selyitems_(0)
    , sely2items_(0)
    , eachcount_(0)
    , selectable_(false)
    , isy1selectable_(true)
    , isy2selectable_(false)
    , mousepressed_(false)
    , pointstobeselected_(false)
{
    enableImageSave();
    x_.defaxsu_.style_ = setup_.xstyle_;
    y_.defaxsu_.style_ = setup_.ystyle_;
    y2_.defaxsu_.style_ = setup_.y2style_;
    x_.defaxsu_.border_ = setup_.minborder_;
    y_.defaxsu_.border_ = setup_.minborder_;
    y2_.defaxsu_.border_ = setup_.minborder_;
    
    meh_.buttonPressed.notify( mCB(this,uiDataPointSetCrossPlotter,mouseClick));
    meh_.buttonReleased.notify( mCB(this,uiDataPointSetCrossPlotter,mouseRel));
    initDraw();
    drawContent();

    reSize.notify( mCB(this,uiDataPointSetCrossPlotter,reDraw) );
    reDrawNeeded.notify( mCB(this,uiDataPointSetCrossPlotter,reDraw) );
    getMouseEventHandler().buttonPressed.notify(
	    mCB(this,uiDataPointSetCrossPlotter,getSelStarPos) );
    getMouseEventHandler().movement.notify(
	    mCB(this,uiDataPointSetCrossPlotter,drawPolygon) );
    getMouseEventHandler().buttonReleased.notify(
	    mCB(this,uiDataPointSetCrossPlotter,itemsSelected) );

    setStretch( 2, 2 );
    setDragMode( uiGraphicsView::ScrollHandDrag );
    setScrollBarPolicy( true, uiGraphicsView::ScrollBarAlwaysOff );
    setScrollBarPolicy( false, uiGraphicsView::ScrollBarAlwaysOff );
}


uiDataPointSetCrossPlotter::~uiDataPointSetCrossPlotter()
{
    delete &lsy1_;
    delete &lsy2_;
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


void uiDataPointSetCrossPlotter::reDraw( CallBacker* )
{
    MouseCursorChanger cursorchanger(MouseCursor::Wait);
    setDraw();
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
    for (  int idx=0; idx<yptitems_->getSize(); idx++ )
    {
	yptitems_->getUiItem(idx)->setPenColor(
		axisHandler(1)->setup().style_.color_ );
	yptitems_->getUiItem(idx)->setSelected( false );
    }
    if ( y2ptitems_ && isY2Shown() )
    {
	for (  int idx=0; idx<y2ptitems_->getSize(); idx++ )
	{
	    y2ptitems_->getUiItem(idx)->setPenColor(
		    axisHandler(2)->setup().style_.color_ );
	    y2ptitems_->getUiItem(idx)->setSelected( false );
	}
    }
}


void uiDataPointSetCrossPlotter::removePoint( uiDataPointSet::DRowID rid,
					      bool isy2, int itmidx )
{
    for ( int idx=0; idx<selareaset_.size(); idx++ )
    {
	SelectionArea* selarea = selareaset_[idx];
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
		yrowidxs_[ itmidx ] = -1;
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
		y2rowidxs_[ itmidx ] = -1;
		BinIDValueSet::Pos pos = dps_.bvsPos(rid);
		float* vals = dps_.bivSet().getVals( pos );
		vals[ dps_.nrFixedCols()+y2_.colid_ ] = mUdf(float);
	    }
	}
    }
}


void uiDataPointSetCrossPlotter::deleteSelections( bool isy2 )
{
    TypeSet<BinIDValueSet::Pos> removepos;
    int itmidx = 0;
    TypeSet<int>& rowidxs = isy2 ? y2rowidxs_ : yrowidxs_;
    for ( int rid=0; rid<rowidxs.size(); rid++ )
    {
	if ( rowidxs[rid] < 0 )
	    continue;
	removePoint( rowidxs[rid], isy2, itmidx );
	itmidx++;
    }
    dps_.dataChanged();
    uidps_.redoAll();
}


void uiDataPointSetCrossPlotter::setSelectionAreas(
	const ObjectSet<SelectionArea>& areaset )
{
    selareaset_ = areaset;
    pointstobeselected_ = true;
    reDrawNeeded.trigger();
}


void uiDataPointSetCrossPlotter::removeSelections()
{
    drawDeSelectedItems();
    selrowcols_.erase();
    deepErase( selareaset_ );
    selyitems_ = 0;
    sely2items_ = 0;
    
    if ( selectionpolygonitem_ )
    {
	scene().removeItem( selectionpolygonitem_ );
	selectionpolygonitem_ = 0;
    }
    
    for ( int idx=0; idx<dps_.size(); idx++ )
	dps_.setSelected( idx, false );

    pointstobeselected_ = true;
    reDrawNeeded.trigger();
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
    if ( !isCtrlPressed() && selareaset_.size() )
	removeSelections();

    mousepressed_ = true;
    if ( !rectangleselection_ )
    {
	ODPolygon<int>* poly = new ODPolygon<int>;
	selareaset_ += new SelectionArea( poly );
	poly->add( getCursorPos() );
	if ( !selectionpolygonitem_ )
	{
	    selectionpolygonitem_ = new uiPolygonItem();
	    selectionpolygonitem_->setPenColor( Color(255,0,0) );
	    selectionpolygonitem_->setZValue( 5 );
	    scene().addItem( selectionpolygonitem_ );
	}
	else
	    selectionpolygonitem_->setPolygon( *poly );
    }
    else
	selareaset_ += new SelectionArea( new uiRect(getCursorPos(),-1) );

    curselarea_ = selareaset_.size() - 1;
}


void uiDataPointSetCrossPlotter::drawPolygon( CallBacker* )
{
    if ( !selectable_ || rectangleselection_ || !mousepressed_ )
	return;

    ODPolygon<int>* poly = selareaset_.validIdx(curselarea_) ?
	selareaset_[curselarea_]->poly_ : 0;
    if ( poly )
	poly->add( getCursorPos() );
    selectionpolygonitem_->setPolygon( *poly );
}


void uiDataPointSetCrossPlotter::itemsSelected( CallBacker* )
{
    mousepressed_ = false;
    if ( !selectable_ )
	return;

    if ( !selareaset_[curselarea_]->isValid() )
    {
	selareaset_.remove( curselarea_ );
	uiMSG().error( "Selection area is not valid" );
	return;
    }

    uiPoint pt;
    if ( rectangleselection_ )
	selareaset_[curselarea_]->rect_->setBottomRight( getCursorPos() );
    
    pointstobeselected_ = true;
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
	if ( rid % eachrow_ || (curgrp_ > 0 && dps_.group(rid) != curgrp_) )
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
    const bool isdel = ev.ctrlStatus() && !ev.shiftStatus() && !ev.altStatus();
    
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
    {
	y2rowidxs_.erase();
	y2ptitems_->removeAll( true );
    }
    
    if ( pointstobeselected_ )
	pointsSelected.trigger();
    pointstobeselected_ = false;
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
	isy2 ? y2rowidxs_ += rid : yrowidxs_ += rid;
    }
    else
	isy2 ? y2rowidxs_[itmidx] = rid : yrowidxs_[itmidx] = rid;
}


void uiDataPointSetCrossPlotter::setItem( uiGraphicsItem* item, bool isy2,
	const uiPoint& pt )
{
    item->setPos( pt.x, pt.y ); 
    eachcount_ ++;
    if ( eachcount_ == eachrow_ )
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
	    mathobj_->setVariable( idx, yval );
	}

	const float result = mathobj_->getValue();
	if ( mIsZero(result,mDefEps) || mIsUdf(result) )
	    return false;
    }
    return true;
}


void uiDataPointSetCrossPlotter::checkSelection( uiDataPointSet::DRowID rid,
	     uiGraphicsItem* item, bool isy2,
	     const uiDataPointSetCrossPlotter::AxisData& yad )
{
    if ( !item ) return;
    for ( int idx=0; idx<selareaset_.size(); idx++ )
    {
	SelectionArea* selarea = selareaset_.size() ? selareaset_[idx] : 0;
	if ( !selarea )
	    continue;

	uiRect* rect = selarea->rect_;
	const uiPoint itempos = item->getPos();
	const bool itmselected = selarea->isInside( itempos );
	if ( isy1selectable_ && !isy2 )
	{
	    if ( itmselected )
	    {
		if ( !isSelectionValid(rid) )
		    continue;
		item->setPenColor( Color::DgbColor() );
		item->setSelected( true );
		
		dps_.setSelected( rid, true );
		selrowcols_ += RowCol( uidps_.tRowID(rid),
				       uidps_.tColID(yad.colid_) );
		
		selyitems_++;
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
		item->setPenColor( Color(100,230,220) );
		item->setSelected( true );
		
		dps_.setSelected( rid, true );
		selrowcols_ += RowCol( uidps_.tRowID(rid),
				       uidps_.tColID(yad.colid_) );
		
		sely2items_++;
	    }
	    else
		dps_.setSelected( rid, false );
	}
    }
    
}


bool uiDataPointSetCrossPlotter::drawPoints( uiGraphicsItemGroup* curitmgrp,
	const uiDataPointSetCrossPlotter::AxisData& yad, bool isy2,
	MarkerStyle2D& mstyle )
{
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
	if ( mIsUdf(xval) || mIsUdf(yval) ) continue;

	const uiPoint pt( xah.getPix(xval), yah.getPix(yval) );
	
	if ( !xpixrg.includes(pt.x) || !ypixrg.includes(pt.y) )
	    continue;

	addItemIfNew( itmidx, mstyle, curitmgrp, yah, rid, isy2 );

	setItem( curitmgrp->getUiItem(itmidx), isy2, pt );
	checkSelection( rid, curitmgrp->getUiItem(itmidx), isy2, yad );
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
	    isy2 ? y2rowidxs_[ itmidx ] = -1 : yrowidxs_[ itmidx ] = -1;
	    curitmgrp->getUiItem(idx)->setVisible(false);
	}
    }
    
    if ( nrptsdisp < 1 )
	return false;
    return true;
}


void uiDataPointSetCrossPlotter::drawData(
    const uiDataPointSetCrossPlotter::AxisData& yad, bool isy2 )
{
    uiAxisHandler& xah = *x_.axis_;
    uiAxisHandler& yah = *yad.axis_;

    int estpts = dps_.size() / eachrow_;
    if ( curgrp_ > 0 ) estpts = cMaxPtsForMarkers;

    MarkerStyle2D mstyle( setup_.markerstyle_ );
    if ( estpts > cMaxPtsForMarkers )
	mstyle.type_ = MarkerStyle2D::None;
    mstyle.color_.setRgb( yah.setup().style_.color_.rgb() );

    prepareItems( isy2 );
    uiGraphicsItemGroup* curitmgrp = isy2 ? y2ptitems_ : yptitems_ ;
    if ( !curitmgrp )
	return;

    if( !drawPoints(curitmgrp,yad,isy2,mstyle) )
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


uiDataPointSetCrossPlotWin::uiDataPointSetCrossPlotWin( uiDataPointSet& uidps )
    : uiMainWin(&uidps,BufferString(uidps.pointSet().name()," Cross-plot"),
	    			    2,false)
    , uidps_(uidps)
    , plotter_(*new uiDataPointSetCrossPlotter(this,uidps,defsetup_))
    , disptb_(*new uiToolBar(this,"Crossplot display toolbar"))
    , seltb_(*new uiToolBar(this,"Crossplot selection toolbar"))
    , maniptb_(*new uiToolBar(this,"Crossplot manipulation toolbar"))
    , grpfld_(0)
    , showSelPts(this)
{
    windowClosed.notify( mCB(this,uiDataPointSetCrossPlotWin,closeNotif) );

    const int nrpts = uidps.pointSet().size();
    const int eachrow = 1 + nrpts / cMaxPtsForDisplay;
    uiGroup* dispgrp = new uiGroup( &disptb_, "Display grp" );
    
    eachfld_ = new uiSpinBox( dispgrp, 0, "Each" );
    eachfld_->setValue( eachrow );
    eachfld_->setInterval( 1, mUdf(int), 1 );
    eachfld_->valueChanged.notify(
	    		mCB(this,uiDataPointSetCrossPlotWin,eachChg) );
    plotter_.eachrow_ = eachrow;
    
    uiLabel* eachlabel = new uiLabel( dispgrp, "Plot each", eachfld_ );
    disptb_.addObject( dispgrp->attachObj() );
    
    showy2tbid_ = disptb_.addButton( "showy2.png",
	    	  mCB(this,uiDataPointSetCrossPlotWin,showY2),
		  "Toggle show Y2", true );
    disptb_.turnOn( showy2tbid_, plotter_.doy2_ );
    
    uiGroup* selgrp = new uiGroup( &seltb_, "Each grp" );
    selfld_ = new uiComboBox( selgrp, "Selection Option" );
    selfld_->addItem( "Select only Y1" );
    selfld_->addItem( "Select only Y2" );
    selfld_->addItem( "Select both" );
    selfld_->selectionChanged.notify( mCB(this,uiDataPointSetCrossPlotWin,
					  selOption) );
    selfld_->setSensitive( false );
    seltb_.addObject( selgrp->attachObj() );

    setselecttbid_ = seltb_.addButton( "view.png",
	    	  mCB(this,uiDataPointSetCrossPlotWin,setSelectable),
		  "Set selectable", true );
    seltb_.turnOn( setselecttbid_, true );


    showselptswstbid_ = seltb_.addButton( "picks.png",
	    	  mCB(this,uiDataPointSetCrossPlotWin,showPtsInWorkSpace),
		  "Show selected points in workSpace", false );
    seltb_.turnOn( setselecttbid_, true );

    selmodechgtbid_ = seltb_.addButton( "rectangleselect.png",
	   mCB(this,uiDataPointSetCrossPlotWin,setSelectionMode) ,
	   "Selection mode" ); 
    seltb_.turnOn( selmodechgtbid_, plotter_.isRubberBandingOn() );

    seltb_.addButton( "clearselection.png",
	    mCB(this,uiDataPointSetCrossPlotWin,removeSelections), 
	    "Remove all selections" );
    
    seltb_.addButton( "trashcan.png",
	    mCB(this,uiDataPointSetCrossPlotWin,deleteSelections), 
	    "Delete all selections" );

    seltb_.addButton( "seltable.png",
	    mCB(this,uiDataPointSetCrossPlotWin,showTableSel), 
	    "Show selections in table" );

    selsettingstbid_ = seltb_.addButton( "settings.png",
	    mCB(this,uiDataPointSetCrossPlotWin,setSelectionDomain), 
	    "Selection settings" );

    maniptb_.addObject( plotter_.getSaveImageTB(this) );

    maniptb_.addButton( "xplotprop.png",
	    mCB(this,uiDataPointSetCrossPlotWin,editProps),"Properties",false );

    const int nrgrps = uidps_.groupNames().size();
    if ( nrgrps > 1 )
    {
	grpfld_ = new uiComboBox( dispgrp, "Group selection" );
	BufferString txt( nrgrps == 2 ? "Both " : "All " );
	txt += uidps_.groupType(); txt += "s";
	grpfld_->addItem( txt );
	for ( int idx=0; idx<uidps_.groupNames().size(); idx++ )
	    grpfld_->addItem( uidps_.groupNames().get(idx) );
	grpfld_->attach( rightOf, eachfld_ );
	grpfld_->setCurrentItem( 0 );
	grpfld_->selectionChanged.notify(
			    mCB(this,uiDataPointSetCrossPlotWin,grpChg) );
    }

    seltb_.setSensitive( selmodechgtbid_, false );
    seltb_.setSensitive( showselptswstbid_, false );
    seltb_.setSensitive( selsettingstbid_, false );
    plotter_.setPrefWidth( 600 );
    plotter_.setPrefHeight( 500 );
}


void uiDataPointSetCrossPlotWin::removeSelections( CallBacker* )
{
    plotter_.removeSelections();
}


void uiDataPointSetCrossPlotWin::showTableSel( CallBacker* )
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );
    uidps_.notifySelectedCell();
}


void uiDataPointSetCrossPlotWin::deleteSelections( CallBacker* )
{
    plotter_.deleteSelections( true );
    plotter_.deleteSelections( false );
    plotter_.removeSelections();
}


void uiDataPointSetCrossPlotWin::closeNotif( CallBacker* )
{
    defsetup_ = plotter_.setup();
    plotter_.eachrow_ = mUdf(int); // Make sure eachChg knows we are closing
}


void uiDataPointSetCrossPlotWin::setSelectionMode( CallBacker* )
{
    plotter_.setRectSelection( !plotter_.isRectSelection() );
    seltb_.setPixmap( selmodechgtbid_,
	   	       plotter_.isRectSelection() ? "rectangleselect.png"
			      			  : "polygonselect.png" );
    plotter_.setDragMode( plotter_.isRectSelection() ?
	    			uiGraphicsView::RubberBandDrag :
	    			uiGraphicsView::NoDrag );
}


class uiSetSelDomainDlg : public uiDialog
{
public:

struct DataColInfo
{
    			DataColInfo( const BufferStringSet& colnames,
				     const TypeSet<int>& colids )
			    : colnms_(colnames), colids_(colids) {}

    BufferStringSet	colnms_;
    TypeSet<int>	colids_;
};


uiSetSelDomainDlg( uiParent* p , DataColInfo& info,
		   const BufferString& mathobjstr, const TypeSet<int>& colids )
    : uiDialog( p, uiDialog::Setup("Refine Selection","Set selectable domain",
				    mNoHelpID).savebutton(true)
	    				      .savetext("Select on Ok") )
    , mathobj_(0)
    , datainfo_(info)
    , dcolids_(colids)
{
    uiLabel* label =
	new uiLabel( this, "Ranges (e.g. 0>x0 && x0>1.5 && -6125<x1)" );
    uiLabel* linelabel = new uiLabel( this, "Enter Ranges" );
    linelabel->attach( leftAlignedBelow, label );
    inpfld_ = new uiLineEdit( this, "Enter Ranges" );
    inpfld_->setHSzPol( uiObject::WideMax );
    inpfld_->attach( rightTo, linelabel );

    setbut_ = new uiPushButton( this, "Set", true );
    setbut_->activated.notify( mCB(this,uiSetSelDomainDlg,parsePush) );
    setbut_->attach( rightTo, inpfld_ );

    vartable_ = new uiTable( this,uiTable::Setup().rowdesc("X")
					.minrowhgt(1.5) .maxrowhgt(2)
					.mincolwdt(3*uiObject::baseFldSize())
					.maxcolwdt(3.5*uiObject::baseFldSize())
					.defrowlbl("") .fillcol(true)
					.fillrow(true) .defrowstartidx(0),
					"Variable X attribute table" );
    const char* xcollbls[] = { "Select input for", 0 };
    vartable_->setColumnLabels( xcollbls );
    vartable_->setNrRows( 2 );
    vartable_->setStretch( 2, 0 );
    vartable_->setRowResizeMode( uiTable::Fixed );
    vartable_->setColumnResizeMode( uiTable::Fixed );
    vartable_->attach( alignedBelow, inpfld_ );
    vartable_->display( false );
    if ( !mathobjstr.isEmpty() )
    {
	inpfld_->setvalue_( mathobjstr );
	parsePush(0);
    }
}


void parsePush( CallBacker* )
{
    mathobj_ = MathExpression::parse( inpfld_->getvalue_() );
    mathexprstring_ = inpfld_->getvalue_();
    if ( !mathobj_ )
    {
	dcolids_.erase();
	vartable_->display( false );
	return;
    }
    updateDisplay();
}


void updateDisplay()
{
     vartable_->setNrRows( mathobj_->getNrVariables() );
     for ( int idx=0; idx<mathobj_->getNrVariables(); idx++ )
     {
	 uiComboBox* varsel = new uiComboBox( 0, datainfo_.colnms_, "Variable");
	 if ( !dcolids_.isEmpty() )
	     varsel->setCurrentItem( dcolids_[idx] );
	 vartable_->setCellObject( RowCol(idx,0), varsel );
     }
     vartable_->display( true );
}


bool acceptOK( CallBacker* )
{
    dcolids_.erase();
    if ( !mathobj_ )
	return true;

    for ( int idx=0; idx<mathobj_->getNrVariables(); idx++ )
    {
	uiObject* obj = vartable_->getCellObject( RowCol(idx,0) );
	mDynamicCastGet( uiComboBox*, box, obj );
	if ( !box )
	    continue;
	dcolids_ += datainfo_.colids_[box->currentItem()];
    }

    PtrMan<MathExpression> testexpr = mathobj_->clone();
    for ( int idx=0; idx<testexpr->getNrVariables(); idx++ )
	testexpr->setVariable( idx, 100 );

    if ( !mIsZero(testexpr->getValue(),mDefEps) &&
	 !mIsZero(testexpr->getValue()-1,mDefEps) )
    {
	uiMSG().error( "Equation should return true or false" );
	return false;
    }

    return true;
}
    
    BufferString	mathexprstring_;
    DataColInfo&	datainfo_;
    MathExpression*	mathobj_;
    TypeSet<int>	dcolids_;
    
    MathExpression*	mathObject()		{ return mathobj_; }
    
    uiLineEdit*		inpfld_;
    uiPushButton*	setbut_;
    uiTable*		vartable_;
};


void uiDataPointSetCrossPlotWin::setSelectionDomain( CallBacker* )
{
    BufferStringSet colnames;
    TypeSet<int> colids;
    const DataPointSet& dps = plotter_.dps();
    for ( uiDataPointSet::DColID dcid=0; dcid<dps.nrCols(); dcid++ )
    {
	colids += dcid;
	colnames.add( dps.colName(dcid) ); 
    }
    uiSetSelDomainDlg::DataColInfo datainfo( colnames, colids );
    uiSetSelDomainDlg dlg( this, datainfo, plotter_.mathObjStr(),
	    		   plotter_.modifiedColIds() );
    if ( dlg.go() )
    {
	plotter_.setMathObj( dlg.mathObject() );
	plotter_.setMathObjStr( dlg.mathexprstring_ );
	plotter_.setModifiedColIds( dlg.dcolids_ );
	if ( dlg.saveButtonChecked() )
	{
	    plotter_.removeSelections();
	    if ( !dlg.mathObject() )
		return;
	    ObjectSet<uiDataPointSetCrossPlotter::SelectionArea> selareas;
	    selareas += new uiDataPointSetCrossPlotter::SelectionArea(
		    new uiRect(0,0,plotter_.width(),plotter_.height()) );
	    plotter_.setSelectionAreas( selareas );
	}
    }
}


void uiDataPointSetCrossPlotWin::setSelectable( CallBacker* cb )
{
    const bool isoff = !seltb_.isOn(setselecttbid_ );
    plotter_.setSceneSelectable( isoff );
    selfld_->setSensitive( plotter_.isY2Shown() ? isoff : false );
    seltb_.setSensitive( selmodechgtbid_, isoff );
    seltb_.setSensitive( showselptswstbid_, isoff );
    seltb_.setSensitive( selsettingstbid_, isoff );
    plotter_.setDragMode(  plotter_.isSceneSelectable()
	    			? ( plotter_.isRectSelection()
				    ? uiGraphicsView::RubberBandDrag
				    : uiGraphicsView::NoDrag )
				: uiGraphicsView::ScrollHandDrag );
}


void uiDataPointSetCrossPlotWin::showY2( CallBacker* )
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );
    if ( plotter_.y2_.axis_ )
    {
	plotter_.showY2( disptb_.isOn(showy2tbid_) );
	setSelComboSensitive( disptb_.isOn(showy2tbid_) );
    }
}


void uiDataPointSetCrossPlotWin::showPtsInWorkSpace( CallBacker* )
{ showSelPts.trigger(); }


void uiDataPointSetCrossPlotWin::selOption( CallBacker* )
{
    const int selitem = selfld_->currentItem();
    if ( selitem == 0 )
	plotter_.setSelectable( true, false );
    else if ( selitem == 1 )
	plotter_.setSelectable( false, true );
    else
	plotter_.setSelectable( true, true );
}


void uiDataPointSetCrossPlotWin::eachChg( CallBacker* )
{
    MouseCursorChanger cursorchanger(MouseCursor::Wait);
    if ( mIsUdf(plotter_.eachrow_) ) return; // window is closing

    int neweachrow = eachfld_->getValue();
    if ( neweachrow < 1 ) neweachrow = 1;
    if ( plotter_.eachrow_ == neweachrow )
	return;
    plotter_.eachrow_ = neweachrow;
    plotter_.drawContent( false );
}


void uiDataPointSetCrossPlotWin::grpChg( CallBacker* )
{
    if ( !grpfld_ ) return;
    plotter_.curgrp_ = grpfld_->currentItem();
    plotter_.dataChanged();
}


void uiDataPointSetCrossPlotWin::setSelComboSensitive( bool yn )
{
    selfld_->setCurrentItem( 0 );
    plotter_.setSelectable( true, false );
    selfld_->setSensitive( plotter_.isSceneSelectable() ? yn : false );
}


void uiDataPointSetCrossPlotWin::editProps( CallBacker* )
{
    uiDataPointSetCrossPlotterPropDlg dlg( &plotter_ );
    dlg.go();
}
