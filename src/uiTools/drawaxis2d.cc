/*+
________________________________________________________________________

 CopyRight:     ( C ) dGB Beheer B.V.
 Author:        Duntao Wei
 Date:          Mar. 2005
 RCS:           $Id: drawaxis2d.cc,v 1.3 2007-02-23 14:26:15 cvsbert Exp $
________________________________________________________________________

-*/

#include "ranges.h"
#include "linear.h"
#include "draw.h"
#include "iodrawtool.h"
#include "drawaxis2d.h"
#include "uiworld2ui.h"


DrawAxis2D::DrawAxis2D()
	: w2u_( 0 )
	, ticlen_(5)
	, inside_(false)
	, drawaxisline_(true)
{
    minx_ = miny_ = 0;
    maxx_ = maxy_ = 1;
    stepx_ = stepy_ = 1;
}


DrawAxis2D::DrawAxis2D( const uiWorld2Ui* wrd2ui, const uiRect* rc )
	: w2u_( 0 )
	, ticlen_(5)
	, inside_(false)
	, drawaxisline_(true)
{
    setupAxis( wrd2ui, rc );
}


void DrawAxis2D::setupAxis( const uiWorld2Ui* wrd2ui, const uiRect* rc )
{
    w2u_ = wrd2ui;
    axislineposset_ = false;
    if ( rc )
    {
	axisrect_ = *rc;
	axislineposset_ = true;
    }
    
    w2u_->getWorldXRange( minx_, maxx_ );
    w2u_->getWorldYRange( miny_, maxy_ );
    w2u_->getRecmMarkStep( stepx_, stepy_ );
}


void DrawAxis2D::setFixedDataRangeAndStep( float minx, float maxx,
				       	   float miny, float maxy,
					   float xstep, float ystep )
{
    minx_ = minx; maxx_ = maxx;
    miny_ = miny; maxy_ = maxy;
    stepx_ = xstep;
    stepy_ = ystep;
}


void DrawAxis2D::drawAxes( ioDrawTool& dt,
			   bool xdir, bool ydir,
			   bool topside, bool leftside ) const
{
    if ( xdir )
	drawXAxis( dt, topside );
    if ( ydir )
	drawYAxis( dt, leftside );
}


void DrawAxis2D::drawXAxis( ioDrawTool& dt, bool topside ) const
{
    int baseline, bias;
    if ( topside )
    {
	baseline = axislineposset_ ? axisrect_.top() : w2u_->toUiY(maxy_);
	bias = inside_ ? ticlen_ : -ticlen_;
    }
    else {
	baseline = axislineposset_ ? axisrect_.bottom() : w2u_->toUiY(miny_);
	bias = inside_ ? -ticlen_ : ticlen_;
    }

    if ( drawaxisline_ )
    {
	const int left = w2u_->toUiX( minx_ );
	const int right = w2u_->toUiX( maxx_ );
	dt.drawLine( left, baseline, right, baseline );
    }
    
    for ( float x = minx_; x <= maxx_; x += stepx_ )
    {
	const int wx = w2u_->toUiX( x );
	dt.drawLine( wx, baseline, wx, baseline+bias );
	BufferString txt = x;
	Alignment al( Alignment::Middle, Alignment::Start );
	if ( bias < 0 ) al.ver = Alignment::Stop;
	dt.drawText( wx, baseline+bias, txt, al, true );
    }
}


void DrawAxis2D::drawYAxis( ioDrawTool& dt, bool leftside ) const
{
    int baseline, bias;
    if ( leftside )
    {
	baseline = axislineposset_ ? axisrect_.left() : w2u_->toUiX(minx_);
	bias = inside_ ? ticlen_ : -ticlen_;
    }
    else
    {
	baseline = axislineposset_ ? axisrect_.right() : w2u_->toUiX(maxx_);
	bias = inside_ ? -ticlen_ : ticlen_;
    }

    if ( drawaxisline_ )
    {
	const int top = w2u_->toUiY( maxy_ );
	const int bot = w2u_->toUiY( miny_ );
	dt.drawLine( baseline, top, baseline, bot );
    }
    
    for ( float y = miny_; y <= maxy_; y += stepy_ )
    {
	const int wy = w2u_->toUiY( y );
	dt.drawLine( baseline, wy, baseline+bias, wy );
	BufferString txt; txt = y;
	Alignment al( Alignment::Start, Alignment::Middle );
	if ( bias < 0 ) al.hor = Alignment::Stop;
	dt.drawText( baseline+bias, wy , txt, al, true );
    }
}


void DrawAxis2D::drawGridLines( ioDrawTool& dt, bool xdir, bool ydir ) const
{
    if ( xdir )
    {
	const int top = w2u_->toUiY( maxy_ );
	const int bot = w2u_->toUiY( miny_ );
	for ( float x = minx_; x <= maxx_; x += stepx_ )
	{
	    const int wx = w2u_->toUiX( x );
	    dt.drawLine( wx, top, wx, bot );
	}
    }

    if ( ydir )
    {
	const int left = w2u_->toUiX( minx_ );
	const int right = w2u_->toUiX( maxx_ );
	for ( float y = miny_; y <= maxy_; y += stepy_ )
	{
	    const int wy = w2u_->toUiY( y );
	    dt.drawLine( left, wy, right, wy );
	}
    }
}
