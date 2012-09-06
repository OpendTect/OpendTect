/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2008
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uifunctiondisplay.cc,v 1.70 2012-09-06 17:39:36 cvsnanne Exp $";

#include "uifunctiondisplay.h"
#include "uiaxishandler.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicssaveimagedlg.h"
#include "mouseevent.h"
#include "axislayout.h"
#include <iostream>

uiFunctionDisplay::uiFunctionDisplay( uiParent* p,
				      const uiFunctionDisplay::Setup& su )
    : uiGraphicsView(p,"Function display viewer")
    , setup_(su)
    , xax_(0)
    , yax_(0)
    , xmarklineval_(mUdf(float))
    , ymarklineval_(mUdf(float))
    , xmarkline2val_(mUdf(float))
    , ymarkline2val_(mUdf(float))
    , selpt_(0)
    , ypolyitem_(0)
    , y2polyitem_(0)
    , ypolygonitem_(0)
    , y2polygonitem_(0)
    , ypolylineitem_(0)
    , y2polylineitem_(0)
    , ymarkeritems_(0)
    , y2markeritems_(0)
    , xmarklineitem_(0)
    , ymarklineitem_(0)
    , xmarkline2item_(0)
    , ymarkline2item_(0)
    , borderrectitem_(0)
    , pointSelected(this)
    , pointChanged(this)
    , mousedown_(false)
{
    disableScrollZoom();
    setPrefWidth( setup_.canvaswidth_ );
    setPrefHeight( setup_.canvasheight_ );
    setStretch( 2, 2 );
    gatherInfo();
    uiAxisHandler::Setup asu( uiRect::Bottom, setup_.canvaswidth_,
	    		      setup_.canvasheight_ );
    asu.noaxisline( setup_.noxaxis_ );
    asu.noaxisannot( asu.noaxisline_ ? true : !setup_.annotx_ );
    asu.nogridline( asu.noaxisline_ ? true : setup_.noxgridline_ );
    asu.border_ = setup_.border_;
    asu.epsaroundzero_ = setup_.epsaroundzero_;
    xax_ = new uiAxisHandler( &scene(), asu );
    asu.noaxisline( setup_.noyaxis_ );
    asu.noaxisannot( asu.noaxisline_ ? true : !setup_.annoty_ );
    asu.nogridline( asu.noaxisline_ ? true : setup_.noygridline_ );
    asu.side( uiRect::Left );
    yax_ = new uiAxisHandler( &scene(), asu );
    asu.noaxisline( setup_.noy2axis_ );
    asu.noaxisannot( asu.noaxisline_ ? true : !setup_.annoty2_ );
    asu.nogridline( setup_.noy2gridline_ );
    xax_->setBegin( yax_ ); yax_->setBegin( xax_ );
    asu.side( uiRect::Right );
    y2ax_ = new uiAxisHandler( &scene(), asu );

    if ( setup_.editable_ )
    {
	getMouseEventHandler().buttonPressed.notify(
				mCB(this,uiFunctionDisplay,mousePress) );
	getMouseEventHandler().buttonReleased.notify(
				mCB(this,uiFunctionDisplay,mouseRelease) );
	getMouseEventHandler().movement.notify(
				mCB(this,uiFunctionDisplay,mouseMove) );
	getMouseEventHandler().doubleClick.notify(
				mCB(this,uiFunctionDisplay,mouseDClick) );
    }

    setToolTip( "Press Ctrl-P to save as image" );
    reSize.notify( mCB(this,uiFunctionDisplay,reSized) );
    setScrollBarPolicy( true, uiGraphicsView::ScrollBarAlwaysOff );
    setScrollBarPolicy( false, uiGraphicsView::ScrollBarAlwaysOff );
    draw();
}


uiFunctionDisplay::~uiFunctionDisplay()
{
    cleanUp();
    delete xax_; delete yax_; delete y2ax_;
}


void uiFunctionDisplay::cleanUp()
{
    delete ypolylineitem_; delete y2polylineitem_;
    delete ypolygonitem_; delete y2polygonitem_;
    delete ymarkeritems_; delete y2markeritems_;
    delete xmarklineitem_; delete ymarklineitem_;
    delete xmarkline2item_; delete ymarkline2item_;
    ypolylineitem_ = y2polylineitem_ = 0;
    ypolygonitem_ = y2polygonitem_ = 0;
    ymarkeritems_ = y2markeritems_ = 0;
    xmarklineitem_ = ymarklineitem_ = xmarkline2item_ = ymarkline2item_ = 0;
    ypolyitem_ = y2polyitem_ = 0;
}


void uiFunctionDisplay::reSized( CallBacker* )
{
    draw();
}

void uiFunctionDisplay::saveImageAs( CallBacker* )
{
}


void uiFunctionDisplay::setVals( const float* xvals, const float* yvals,
				 int sz )
{
    xvals_.erase(); yvals_.erase();
    if ( sz > 0 )
    {
	for ( int idx=0; idx<sz; idx++ )
	    { xvals_ += xvals[idx]; yvals_ += yvals[idx]; }
    }

    gatherInfo(); draw();
}


void uiFunctionDisplay::setVals( const Interval<float>& xrg, const float* yvals,
				 int sz )
{
    xvals_.erase(); yvals_.erase();
    if ( sz > 0 )
    {
	const float dx = (xrg.stop-xrg.start) / (sz-1);
	for ( int idx=0; idx<sz; idx++ )
	    { xvals_ += xrg.start + idx * dx; yvals_ += yvals[idx]; }
    }

    gatherInfo(); draw();
}


void uiFunctionDisplay::setY2Vals( const float* xvals, const float* yvals,
				   int sz )
{
    y2xvals_.erase(); y2yvals_.erase();
    if ( sz > 0 )
    {
	for ( int idx=0; idx<sz; idx++ )
	{
	    y2xvals_ += xvals[idx];
	    y2yvals_ += yvals[idx];
	}
    }

    gatherInfo(); draw();
}


void uiFunctionDisplay::setY2Vals( const Interval<float>& xrg, 
				   const float* yvals,
				   int sz )
{
    y2xvals_.erase(); y2yvals_.erase();
    if ( sz > 0 )
    {
	const float dx = (xrg.stop-xrg.start) / (sz-1);
	for ( int idx=0; idx<sz; idx++ )
	{ 
	    y2xvals_ += xrg.start + idx * dx; 
	    y2yvals_ += yvals[idx]; 
	}
    }

    gatherInfo(); draw();
}


void uiFunctionDisplay::setMarkValue( float val, bool is_x )
{
    (is_x ? xmarklineval_ : ymarklineval_) = val;
    drawMarkLines();
}


void uiFunctionDisplay::setMark2Value( float val, bool is_x )
{
    (is_x ? xmarkline2val_ : ymarkline2val_) = val;
    drawMarkLines();
}


void uiFunctionDisplay::setEmpty()
{
    xmarklineval_ = ymarklineval_ =
    xmarkline2val_ = ymarkline2val_ = mUdf(float);
    setVals( 0, 0, 0 );
    setY2Vals( 0, 0, 0 );
    cleanUp();
}


void uiFunctionDisplay::gatherInfo()
{
    if ( yvals_.isEmpty() ) return;
    const bool havey2 = !y2xvals_.isEmpty();
    if ( havey2 )
	{ xax_->setEnd( y2ax_ ); y2ax_->setBegin( xax_ ); }

    StepInterval<float> xrg, yrg;
    getRanges( xvals_, yvals_, setup_.xrg_, setup_.yrg_, xrg, yrg );

    xax_->setBounds( xrg );
    yax_->setBounds( yrg );

    if ( havey2 )
    {
	if ( !setup_.useyscalefory2_ )
	    getRanges( y2xvals_, y2yvals_, setup_.xrg_, setup_.y2rg_, xrg, yrg);
	y2ax_->setBounds( yrg );
    }
}


void uiFunctionDisplay::getRanges(
	const TypeSet<float>& xvals, const TypeSet<float>& yvals,
	const Interval<float>& setupxrg, const Interval<float>& setupyrg,
	StepInterval<float>& xrg, StepInterval<float>& yrg ) const
{
    for ( int idx=0; idx<xvals.size(); idx++ )
    {
	if ( idx == 0 )
	{
	    xrg.start = xrg.stop = xvals[0];
	    yrg.start = yrg.stop = yvals[0];
	}
	else
	{
	    if ( xvals[idx] < xrg.start ) xrg.start = xvals[idx];
	    if ( yvals[idx] < yrg.start ) yrg.start = yvals[idx];
	    if ( xvals[idx] > xrg.stop ) xrg.stop = xvals[idx];
	    if ( yvals[idx] > yrg.stop ) yrg.stop = yvals[idx];
	}
    }

    if ( !setup_.fixdrawrg_ ) 
	return;

    if ( !mIsUdf(setupxrg.start) ) xrg.start = setupxrg.start;
    if ( !mIsUdf(setupyrg.start) ) yrg.start = setupyrg.start;
    if ( !mIsUdf(setupxrg.stop) ) xrg.stop = setupxrg.stop;
    if ( !mIsUdf(setupyrg.stop) ) yrg.stop = setupyrg.stop;
}


void uiFunctionDisplay::setUpAxis( bool havey2 )
{
    xax_->updateDevSize();
    yax_->updateDevSize();
    if ( havey2 ) y2ax_->updateDevSize();

    xax_->plotAxis();
    yax_->plotAxis();
    if ( havey2 )
	y2ax_->plotAxis();
}


void uiFunctionDisplay::getPointSet( TypeSet<uiPoint>& ptlist, bool y2 )
{
    const int nrpts = y2 ? y2xvals_.size() : xvals_.size();
    const uiPoint closept( xax_->getPix(xax_->range().start),
	    		   y2 ? y2ax_->getPix(y2ax_->range().start)
	   		      : yax_->getPix(yax_->range().start) );
    const bool fillbelow = y2 ? setup_.fillbelowy2_ : setup_.fillbelow_;
    if ( fillbelow )
	ptlist += closept;

    const Interval<int> xpixintv( xax_->getPix(xax_->range().start),
				  xax_->getPix(xax_->range().stop) );
    const Interval<int> ypixintv( y2 ? y2ax_->getPix(y2ax_->range().start)
				     : yax_->getPix(yax_->range().start),
				  y2 ? y2ax_->getPix(y2ax_->range().stop)
				     : yax_->getPix(yax_->range().stop) );
    uiPoint pt = closept;
    for ( int idx=0; idx<nrpts; idx++ )
    {
	const int xpix = xax_->getPix( y2 ? y2xvals_[idx] : xvals_[idx] );
	const int ypix = y2 ? y2ax_->getPix(y2yvals_[idx]) 
			    : yax_->getPix(yvals_[idx]);
	if ( xpixintv.includes(xpix,true) && ypixintv.includes(ypix,true) )
	{
	    pt.x = xpix;
	    pt.y = ypix;
	    ptlist += pt;
	}
    }
	
    if ( setup_.closepolygon_ && fillbelow )
	ptlist += uiPoint( pt.x, closept.y );
}


void uiFunctionDisplay::drawYCurve( const TypeSet<uiPoint>& ptlist )
{
    bool polydrawn = false;
    if ( setup_.fillbelow_ )
    {
	if ( !ypolygonitem_ )
	    ypolygonitem_ = scene().addPolygon( ptlist, setup_.fillbelow_ );
	else
	    ypolygonitem_->setPolygon( ptlist );
	ypolygonitem_->setFillColor( setup_.ycol_ );
	ypolyitem_ = ypolygonitem_;
	polydrawn = true;
    }
    else if ( setup_.drawliney_ )
    {
	if ( !ypolylineitem_ )
	    ypolylineitem_ = scene().addPolyLine( ptlist );
	else
	    ypolylineitem_->setPolyLine( ptlist );
	ypolyitem_ = ypolylineitem_;
	polydrawn = true;
    }

    if ( polydrawn )
    {
	LineStyle ls;
	ls.width_ = setup_.ywidth_;    
	ls.color_ = setup_.ycol_;    
	ypolyitem_->setPenStyle( ls );
	ypolyitem_->setZValue( setup_.curvzvaly_ );
	ypolyitem_->setVisible( true );
    }
    else if ( ypolyitem_ )
	ypolyitem_->setVisible( false );
}


void uiFunctionDisplay::drawY2Curve( const TypeSet<uiPoint>& ptlist,
				     bool havey2 )
{
    bool polydrawn = false;
    if ( setup_.fillbelowy2_ )
    {
	if ( !y2polygonitem_ )
	    y2polygonitem_ = scene().addPolygon( ptlist, setup_.fillbelowy2_ );
	else
	    y2polygonitem_->setPolygon( ptlist );
	y2polygonitem_->setFillColor(
		setup_.fillbelowy2_ ? setup_.y2col_ : Color::NoColor());
	y2polyitem_ = y2polygonitem_;
	polydrawn = true;
    }
    else if ( setup_.drawliney2_ )
    {
	if ( !y2polylineitem_ )
	    y2polylineitem_ = scene().addPolyLine( ptlist );
	else
	    y2polylineitem_->setPolyLine( ptlist );
	y2polyitem_ = y2polylineitem_;
	polydrawn = true;
    }

    if ( polydrawn )
    {
	LineStyle ls;
	ls.width_ = setup_.y2width_;    
	ls.color_ = setup_.y2col_;    
	y2polyitem_->setPenStyle( ls );
	y2polyitem_->setZValue( setup_.curvzvaly2_ );
	y2polyitem_->setVisible( true );
    }
    else if ( y2polyitem_ && !havey2)
	y2polyitem_->setVisible( false );
}


void uiFunctionDisplay::drawMarker( const TypeSet<uiPoint>& ptlist, bool isy2 )
{
    if ( isy2 ? !y2markeritems_ : !ymarkeritems_ )
    {
	if ( isy2 )
	{
	    if ( !setup_.drawscattery2_ )
	    {
		if ( y2markeritems_ )
		    y2markeritems_->setVisible( false );
		return;
	    }
	    
	    y2markeritems_ = new uiGraphicsItemGroup( true );
	    scene().addItemGrp( y2markeritems_ );
	}
	else
	{
	    if ( !setup_.drawscattery1_ )
	    {
		if ( ymarkeritems_ )
		    ymarkeritems_->setVisible( false );
		return;
	    }
	    
	    ymarkeritems_ = new uiGraphicsItemGroup( true );
	    scene().addItemGrp( ymarkeritems_ );
	}
    }

    uiGraphicsItemGroup* curitmgrp = isy2 ? y2markeritems_ : ymarkeritems_;
    curitmgrp->setVisible( true );
    const MarkerStyle2D mst( MarkerStyle2D::Square, 3,
			     isy2 ? setup_.y2col_ : setup_.ycol_ );
    for ( int idx=0; idx<ptlist.size(); idx++ )
    {
	if ( idx >= curitmgrp->size() )
	{
	    uiMarkerItem* markeritem = new uiMarkerItem( mst, false );
	    curitmgrp->add( markeritem );
	}
	uiGraphicsItem* itm = curitmgrp->getUiItem(idx);
	itm->setPos( ptlist[idx].x, ptlist[idx].y );
	itm->setPenColor( isy2 ? setup_.y2col_ : setup_.ycol_ );
    }

    if ( ptlist.size() < curitmgrp->size() )
    {
	for ( int idx=ptlist.size(); idx<curitmgrp->size(); idx++ )
	    curitmgrp->getUiItem(idx)->setVisible( false );
    }
    curitmgrp->setZValue( isy2 ? setup_.curvzvaly2_ : setup_.curvzvaly_ );
}


void uiFunctionDisplay::drawBorder()
{
    if ( setup_.drawborder_ )
    {
	const int scwidth = (int)scene().width();
	const int scheight = (int)scene().height();
	const uiRect r( xAxis()->pixBefore(), yAxis(false)->pixAfter(),
		scwidth -xAxis()->pixAfter()-xAxis()->pixBefore(),
		scheight -yAxis(false)->pixAfter()-yAxis(false)->pixBefore() );

	if ( !borderrectitem_ )
	    borderrectitem_ = scene().addRect( r.left(), r.top(), 
					       r.right(), r.bottom() ); 
	else
	    borderrectitem_->setRect( r.left(), r.top(), 
				      r.right(), r.bottom() );
	borderrectitem_->setPenStyle( setup_.borderstyle_ );
    }
    if ( borderrectitem_ )
	borderrectitem_->setVisible( setup_.drawborder_ );
}


void uiFunctionDisplay::draw()
{
    if ( yvals_.isEmpty() ) return;
    const bool havey2 = !y2xvals_.isEmpty();

    setUpAxis( havey2 );

    TypeSet<uiPoint> yptlist, y2ptlist;
    getPointSet( yptlist, false );
    if ( havey2 )
	getPointSet( y2ptlist, true );

    drawYCurve( yptlist );
    drawY2Curve( y2ptlist, havey2 );
    drawMarker(yptlist,false);
    if ( havey2 )
	drawMarker(y2ptlist,true);
    else if ( y2markeritems_ )
	y2markeritems_->setVisible( false );
    drawBorder();
    drawMarkLines();
}


void uiFunctionDisplay::drawMarkLines()
{
#define mDrawMarkLine(xy,nr,colnr) \
    if ( !mIsUdf(xy##markline##nr##val_) ) \
	drawMarkLine( xy##ax_, xy##markline##nr##val_, \
		      Color::stdDrawColor(colnr), xy##markline##nr##item_)
    mDrawMarkLine(x,,0);
    mDrawMarkLine(y,,0);
    mDrawMarkLine(x,2,1);
    mDrawMarkLine(y,2,1);
}


void uiFunctionDisplay::drawMarkLine( uiAxisHandler* ah, float val, Color col,
       				  uiLineItem*& itm )
{
    delete itm;
    itm = ah->getFullLine( ah->getPix(val) );
    itm->setPenColor( col );
    itm->setZValue( 100 );
    scene().addItem( itm );
}


#define mGetMousePos()  \
    if ( getMouseEventHandler().isHandled() ) \
	return; \
    const MouseEvent& ev = getMouseEventHandler().event(); \
    if ( !(ev.buttonState() & OD::LeftButton ) || \
	  (ev.buttonState() & OD::MidButton ) || \
	  (ev.buttonState() & OD::RightButton ) ) \
        return; \
    const bool isctrl = ev.ctrlStatus(); \
    const bool isoth = ev.shiftStatus() || ev.altStatus(); \
    const bool isnorm mUnusedVar = !isctrl && !isoth;


bool uiFunctionDisplay::setSelPt()
{
    const MouseEvent& ev = getMouseEventHandler().event();

    int newsel = -1; float mindistsq = 1e30;
    const float xpix = xax_->getRelPos( xax_->getVal(ev.pos().x) );
    const float ypix = yax_->getRelPos( yax_->getVal(ev.pos().y) );
    for ( int idx=0; idx<xvals_.size(); idx++ )
    {
	const float x = xax_->getRelPos( xvals_[idx] );
	const float y = yax_->getRelPos( yvals_[idx] );
	const float distsq = (x-xpix)*(x-xpix) + (y-ypix)*(y-ypix);
	if ( distsq < mindistsq )
	    { newsel = idx; mindistsq = distsq; }
    }
    selpt_ = -1;
    if ( mindistsq > setup_.ptsnaptol_*setup_.ptsnaptol_ ) return false;
    if ( newsel < 0 || newsel > xvals_.size() - 1 )
    {
	selpt_ = -1;
	return false;
    }

    selpt_ = newsel;
    return true;
}


void uiFunctionDisplay::mousePress( CallBacker* )
{
    if ( mousedown_ ) return;

    mousedown_ = true;
    mGetMousePos();
    if ( isoth || !setSelPt() ) return;

    if ( isnorm )
	pointSelected.trigger();
}


void uiFunctionDisplay::mouseRelease( CallBacker* )
{
    if ( !mousedown_ ) return;

    mousedown_ = false;
    mGetMousePos();
    if ( !isctrl || selpt_ <= 0 || selpt_ >= xvals_.size()-1
	 || xvals_.size() < 3 ) return;

    xvals_.remove( selpt_ );
    yvals_.remove( selpt_ );

    selpt_ = -1;
    pointChanged.trigger();
    draw();
}


void uiFunctionDisplay::mouseMove( CallBacker* )
{
    if ( !mousedown_ ) return;

    mGetMousePos();
    if ( !isnorm || selpt_<0 ) return;

    float xval = xax_->getVal( ev.pos().x );
    float yval = yax_->getVal( ev.pos().y );

    if ( selpt_>0 && xvals_[selpt_-1]>=xval )
        xval = xvals_[selpt_-1];
    else if ( selpt_<xvals_.size()-1 && xvals_[selpt_+1]<=xval )
        xval = xvals_[selpt_+1];

    if ( xval > xax_->range().stop )
	xval = xax_->range().stop;
    else if ( xval < xax_->range().start )
	xval = xax_->range().start;

    if ( yval > yax_->range().stop )
	yval = yax_->range().stop;
    else if ( yval < yax_->range().start )
	yval = yax_->range().start;

    xvals_[selpt_] = xval; yvals_[selpt_] = yval;

    pointChanged.trigger();
    draw();
}


void uiFunctionDisplay::mouseDClick( CallBacker* )
{
    mousedown_ = false;
    mGetMousePos();
    if ( !isnorm ) return;

    float xval = xax_->getVal(ev.pos().x);
    float yval = yax_->getVal(ev.pos().y);
    
    if ( xval > xax_->range().stop )
	xval = xax_->range().stop;
    else if ( xval < xax_->range().start )
	xval = xax_->range().start;

    if ( yval > yax_->range().stop )
	yval = yax_->range().stop;
    else if ( yval < yax_->range().start )
	yval = yax_->range().start;

    if ( !xvals_.isEmpty() && xval > xvals_.last() )
    {
	xvals_ += xval; yvals_ += yval;
	selpt_ = xvals_.size()-1;
    }
    else
    {
	for ( int idx=0; idx<xvals_.size(); idx++ )
	{
	    if ( xval > xvals_[idx] )
		continue;

	    if ( xval == xvals_[idx] )
		yvals_[idx] = yval;
	    else
	    {
		xvals_.insert( idx, xval );
		yvals_.insert( idx, yval );
	    }

	    selpt_ = idx;
	    break;
	}
    }

    pointSelected.trigger();
    draw();
}


void uiFunctionDisplay::dump( std::ostream& strm, bool y2 ) const
{
    const TypeSet<float>& xvals = y2 ? y2xvals_ : xvals_;
    const TypeSet<float>& yvals = y2 ? y2yvals_ : yvals_;

    strm << std::fixed;
    for ( int idx=0; idx<xvals.size(); idx++ )
	strm << xvals[idx] << '\t' << yvals[idx] << std::endl;
}
