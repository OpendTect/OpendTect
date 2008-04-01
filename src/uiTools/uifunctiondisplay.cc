/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2008
 RCS:           $Id: uifunctiondisplay.cc,v 1.1 2008-04-01 13:22:53 cvsbert Exp $
________________________________________________________________________

-*/

#include "uifunctiondisplay.h"
#include "uiaxishandler.h"
#include "linear.h"

static const int cBoundarySz = 10;

uiFunctionDisplay::uiFunctionDisplay( uiParent* p,
				      const uiFunctionDisplay::Setup& su )
    : uiCanvas( p, "Function display canvas" )
    , setup_(su)
    , xax_(0)
    , yax_(0)
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
	{ xvals_ += idx * dx; yvals_ += yvals[idx]; }

    gatherInfo(); update();
}


void uiFunctionDisplay::setY2Vals( const float* vals )
{
    y2vals_.erase();
    if ( size() < 2 ) return;

    for ( int idx=0; idx<size(); idx++ )
	y2vals_ += vals[idx];

    gatherInfo(); update();
}


void uiFunctionDisplay::setMarkValue( float val, bool is_x )
{
    (is_x ? xmarkval_ : ymarkval_) = val;
}


void uiFunctionDisplay::gatherInfo()
{
    if ( yvals_.isEmpty() ) return;
    const bool havey2 = !y2vals_.isEmpty();
    if ( havey2 )
	xax_->setEnd( y2ax_ ); y2ax_->setBegin( xax_ );

    const int nrpts = size();
    StepInterval<float> xrg, yrg, y2rg;
    for ( int idx=0; idx<nrpts; idx++ )
    {
	if ( idx == 0 )
	{
	    xrg.start = xrg.stop = xvals_[0];
	    yrg.start = yrg.stop = yvals_[0];
	    if ( havey2 )
		y2rg.start = y2rg.stop = y2vals_[0];
	}
	else
	{
	    if ( xvals_[idx] < xrg.start ) xrg.start = xvals_[idx];
	    if ( yvals_[idx] < yrg.start ) yrg.start = yvals_[idx];
	    if ( xvals_[idx] > xrg.stop ) xrg.stop = xvals_[idx];
	    if ( yvals_[idx] > yrg.stop ) yrg.stop = yvals_[idx];
	    if ( havey2 )
	    {
		if ( y2vals_[idx] < y2rg.start ) y2rg.start = y2vals_[idx];
		if ( y2vals_[idx] > y2rg.stop ) y2rg.stop = y2vals_[idx];
	    }
	}
    }

    if ( !mIsUdf(setup_.xrg_.start) ) xrg.start = setup_.xrg_.start;
    if ( !mIsUdf(setup_.yrg_.start) ) yrg.start = setup_.yrg_.start;
    if ( !mIsUdf(setup_.xrg_.stop) ) xrg.stop = setup_.xrg_.stop;
    if ( !mIsUdf(setup_.yrg_.stop) ) yrg.stop = setup_.yrg_.stop;
    if ( havey2 )
    {
	if ( !mIsUdf(setup_.y2rg_.start) ) y2rg.start = setup_.y2rg_.start;
	if ( !mIsUdf(setup_.y2rg_.stop) ) y2rg.stop = setup_.y2rg_.stop;
    }

    AxisLayout axlyo; axlyo.setDataRange( xrg );
    xrg.step = axlyo.sd.step;
    if ( !mIsEqual(xrg.start,axlyo.sd.start,axlyo.sd.step*1e-6) )
	axlyo.sd.start += axlyo.sd.step;
    xax_->setRange( xrg, &axlyo.sd.start );

    axlyo.setDataRange( yrg ); yrg.step = axlyo.sd.step;
    yax_->setRange( yrg );

    if ( havey2 )
    {
	axlyo.setDataRange( y2rg ); y2rg.step = axlyo.sd.step;
	y2ax_->setRange( y2rg );
    }
}


void uiFunctionDisplay::reDrawHandler( uiRect )
{
    if ( yvals_.isEmpty() ) return;
    const bool havey2 = !y2vals_.isEmpty();

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

	if ( havey2 )
	    y2ptlist += uiPoint( xpix, y2ax_->getPix(y2vals_[idx]) );
    }

    ioDrawTool& dt = drawTool();
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
