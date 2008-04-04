/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2008
 RCS:           $Id: uifunctiondisplay.cc,v 1.8 2008-04-04 07:48:29 cvsbert Exp $
________________________________________________________________________

-*/

#include "uifunctiondisplay.h"
#include "uiaxishandler.h"
#include "mouseevent.h"
#include "linear.h"

static const int cBoundarySz = 10;

uiFunctionDisplay::uiFunctionDisplay( uiParent* p,
				      const uiFunctionDisplay::Setup& su )
    : uiCanvas(p,su.bgcol_,"Function display canvas" )
    , setup_(su)
    , xax_(0)
    , yax_(0)
    , xmarkval_(mUdf(float))
    , ymarkval_(mUdf(float))
    , selpt_(0)
    , pointSelected(this)
    , pointChanged(this)
{
    setPrefWidth( setup_.canvaswidth_ );
    setPrefHeight( setup_.canvasheight_ );
    setStretch( 2, 2 );
    preDraw.notify( mCB(this,uiFunctionDisplay,gatherInfo) );
    uiAxisHandler::Setup asu( uiRect::Bottom );
    asu.border_ = setup_.border_;
    xax_ = new uiAxisHandler( drawTool(), asu );
    asu.side( uiRect::Left ).noannot( !setup_.annoty_ );
    yax_ = new uiAxisHandler( drawTool(), asu );
    xax_->setBegin( yax_ ); yax_->setBegin( xax_ );
    asu.side( uiRect::Right ).noannot( !setup_.annoty_ );
    y2ax_ = new uiAxisHandler( drawTool(), asu );

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
}


uiFunctionDisplay::~uiFunctionDisplay()
{
    delete xax_;
    delete yax_;
}


void uiFunctionDisplay::setVals( const float* xvals, const float* yvals,
				 int sz )
{
    xvals_.erase(); yvals_.erase();
    if ( sz < 2 ) return;

    for ( int idx=0; idx<sz; idx++ )
	{ xvals_ += xvals[idx]; yvals_ += yvals[idx]; }

    gatherInfo(); update();
}


void uiFunctionDisplay::setVals( const Interval<float>& xrg, const float* yvals,
				 int sz )
{
    xvals_.erase(); yvals_.erase();
    if ( sz < 2 ) return;

    const float dx = (xrg.stop-xrg.start) / (sz-1);
    for ( int idx=0; idx<sz; idx++ )
	{ xvals_ += xrg.start + idx * dx; yvals_ += yvals[idx]; }

    gatherInfo(); update();
}


void uiFunctionDisplay::setY2Vals( const float* xvals, const float* yvals,
				   int sz )
{
    y2xvals_.erase(); y2yvals_.erase();
    if ( sz < 2 ) return;

    for ( int idx=0; idx<sz; idx++ )
    {
	y2xvals_ += xvals[idx];
	y2yvals_ += yvals[idx];
    }

    gatherInfo(); update();
}


void uiFunctionDisplay::setMarkValue( float val, bool is_x )
{
    (is_x ? xmarkval_ : ymarkval_) = val;
}


void uiFunctionDisplay::gatherInfo()
{
    if ( yvals_.isEmpty() ) return;
    const bool havey2 = !y2xvals_.isEmpty();
    if ( havey2 )
	{ xax_->setEnd( y2ax_ ); y2ax_->setBegin( xax_ ); }

    StepInterval<float> xrg, yrg;
    getRanges( xvals_, yvals_, setup_.xrg_, setup_.yrg_, xrg, yrg );

    AxisLayout axlyo; axlyo.setDataRange( xrg );
    xrg.step = axlyo.sd.step;
    if ( !mIsEqual(xrg.start,axlyo.sd.start,axlyo.sd.step*1e-6) )
	axlyo.sd.start += axlyo.sd.step;
    xax_->setRange( xrg, &axlyo.sd.start );

    axlyo.setDataRange( yrg ); yrg.step = axlyo.sd.step;
    yax_->setRange( yrg );

    if ( havey2 )
    {
	getRanges( y2xvals_, y2yvals_, setup_.xrg_, setup_.y2rg_, xrg, yrg );
	axlyo.setDataRange( yrg ); yrg.step = axlyo.sd.step;
	y2ax_->setRange( yrg );
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

    if ( !mIsUdf(setupxrg.start) ) xrg.start = setupxrg.start;
    if ( !mIsUdf(setupyrg.start) ) yrg.start = setupyrg.start;
    if ( !mIsUdf(setupxrg.stop) ) xrg.stop = setupxrg.stop;
    if ( !mIsUdf(setupyrg.stop) ) yrg.stop = setupyrg.stop;
}


void uiFunctionDisplay::reDrawHandler( uiRect )
{
    ioDrawTool& dt = drawTool();
    if ( yvals_.isEmpty() ) return;
    const bool havey2 = !y2xvals_.isEmpty();

    xax_->newDevSize();
    yax_->newDevSize();
    if ( havey2 ) y2ax_->newDevSize();

    xax_->plotAxis();
    if ( setup_.annoty_ )
    {
	yax_->plotAxis();
	if ( havey2 )
	    y2ax_->plotAxis();
    }

    TypeSet<uiPoint> yptlist, y2ptlist;
    const int nrpts = size();
    const uiPoint closept( xax_->getPix(xax_->range().start),
	    		   yax_->getPix(yax_->range().start) );
    if ( setup_.fillbelow_ )
	yptlist += closept;

    for ( int idx=0; idx<nrpts; idx++ )
    {
	const int xpix = xax_->getPix( xvals_[idx] );
	const uiPoint pt( xpix, yax_->getPix(yvals_[idx]) );
	yptlist += pt;
	if ( setup_.fillbelow_ && idx == nrpts-1 )
	{
	    yptlist += uiPoint( pt.x, closept.y );
	    yptlist += closept;
	}

    }

    if ( havey2 )
    {
	Interval<int> xpixintv( xax_->getPix( xax_->range().start ),
				xax_->getPix( xax_->range().stop ) );
	for ( int idx=0; idx<y2xvals_.size(); idx++ )
	{
	    const int xpix = xax_->getPix( y2xvals_[idx] );
	    if ( xpixintv.includes(xpix) )
		yptlist += uiPoint( xpix, y2ax_->getPix(y2yvals_[idx]) );
	}
    }

    dt.setPenColor( setup_.ycol_ );
    dt.setFillColor( setup_.fillbelow_ ? setup_.ycol_ : Color::NoColor );
    if ( setup_.fillbelow_ )
	dt.drawPolygon( yptlist );
    else
	dt.drawPolyline( yptlist );

    dt.setFillColor( Color::NoColor );
    if ( havey2 )
    {
	dt.setPenColor( setup_.y2col_ );
	dt.drawPolygon( y2ptlist );
    }

    if ( setup_.pointsz_ > 0 )
    {
	const MarkerStyle2D mst( MarkerStyle2D::Square, setup_.pointsz_,
				 setup_.ycol_ );
	for ( int idx=0; idx<nrpts; idx++ )
	{
	    const int xpix = xax_->getPix( xvals_[idx] );
	    const uiPoint pt( xpix, yax_->getPix(yvals_[idx]) );
	    dt.drawMarker( pt, mst );
	}
    }

    if ( !mIsUdf(xmarkval_) )
    {
	dt.setPenColor( setup_.xmarkcol_ );
	xax_->drawGridLine( xax_->getPix(xmarkval_) );
    }
    if ( !mIsUdf(ymarkval_) )
    {
	dt.setPenColor( setup_.ymarkcol_ );
	yax_->drawGridLine( yax_->getPix(ymarkval_) );
    }
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
    const bool isnorm = !isctrl && !isoth

static const float cRelTol = 0.01; // 1% of axis size


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
    if ( mindistsq > cRelTol*cRelTol ) return false;
    selpt_ = newsel;
    return true;
}


void uiFunctionDisplay::mousePress( CallBacker* )
{
    if ( mousedown_ ) return; mousedown_ = true;
    mGetMousePos();
    if ( isoth || !setSelPt() ) return;

    if ( isnorm )
	pointSelected.trigger();
}


void uiFunctionDisplay::mouseRelease( CallBacker* )
{
    if ( !mousedown_ ) return; mousedown_ = false;
    mGetMousePos();
    if ( !isctrl || selpt_ < 0 || xvals_.size() < 3 ) return;

    xvals_.remove( selpt_ );
    yvals_.remove( selpt_ );

    selpt_ = -1;
    pointChanged.trigger();
    update();
}


void uiFunctionDisplay::mouseMove( CallBacker* )
{
    if ( !mousedown_ ) return;
    mGetMousePos();
    if ( !isnorm || selpt_ < 0 ) return;

    float xval = xax_->getVal(ev.pos().x);
    if ( selpt_ > 0 && xvals_[selpt_-1] >= xval )
	return;
    if ( selpt_ < xvals_.size() - 1 && xvals_[selpt_+1] <= xval )
	return;

    const float yval = yax_->getVal(ev.pos().y);
    xvals_[selpt_] = xval; yvals_[selpt_] = yval;

    pointChanged.trigger();
    update();
}


void uiFunctionDisplay::mouseDClick( CallBacker* )
{
    mousedown_ = false;
    mGetMousePos();
    if ( !isnorm ) return;

    const float xval = xax_->getVal(ev.pos().x);
    const float yval = yax_->getVal(ev.pos().y);
    if ( xval > xvals_[xvals_.size()-1] )
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
    update();
}
