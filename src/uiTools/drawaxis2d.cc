/*+
________________________________________________________________________

 CopyRight:     ( C ) dGB Beheer B.V.
 Author:        Duntao Wei
 Date:          Mar. 2005
 RCS:           $Id: drawaxis2d.cc,v 1.1 2005-03-22 08:17:39 cvsduntao Exp $
________________________________________________________________________

-*/

#include "ranges.h"
#include "linear.h"
#include "draw.h"
#include "iodrawtool.h"
#include "drawaxis2d.h"
#include "uiworld2ui.h"


DrawAxis2D::DrawAxis2D()
	: wrd2uicnv( 0 )
	, ticlen(5)
{
    minx_ = miny_ = 0;
    maxx_ = maxy_ = 1;
    stepx_ = stepy_ = 1;
}


DrawAxis2D::DrawAxis2D( const uiWorld2Ui* wrd2ui,  const uiRect* rc )
	: wrd2uicnv( 0 )
	, ticlen(5)
{
    setupAxis( wrd2ui, rc );
}


void DrawAxis2D::setupAxis( const uiWorld2Ui* wrd2ui,  const uiRect* rc)
{
    wrd2uicnv = wrd2ui;
    axislineposset_ = false;
    if ( rc )
    {
	axisrect = *rc;
	axislineposset_ = true;
    }
    
    wrd2uicnv->getWorldXRange( minx_, maxx_ );
    wrd2uicnv->getWorldYRange( miny_, maxy_ );
    wrd2uicnv->getRecmMarkStep( stepx_, stepy_ );
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


void DrawAxis2D::drawXAxis( bool topside, ioDrawTool* drawtool )
{
    if ( !drawtool ) return;

    int baseline, bias;
    if ( topside )
    {
	baseline = axislineposset_ ? axisrect.top() : wrd2uicnv->toUiY(maxy_);
	bias = -ticlen;
    }
    else {
	baseline = axislineposset_ ? axisrect.bottom() : wrd2uicnv->toUiY(miny_);
	bias = ticlen;
    }

    int left = wrd2uicnv->toUiX( minx_ );
    int right = wrd2uicnv->toUiX( maxx_ );
    drawtool->drawLine( left, baseline, right, baseline );
    
    for ( float x = minx_; x <= maxx_; x += stepx_)
    {
	int wx = wrd2uicnv->toUiX( x );
	drawtool->drawLine( wx, baseline, wx, baseline+bias );
	BufferString txt = x;
	Alignment al = bias < 0 ? Alignment( Alignment::Middle,Alignment::Stop )
	    : Alignment( Alignment::Middle,Alignment::Start );
	drawtool->drawText( wx, baseline+bias, txt, al, true );
    }
}


void DrawAxis2D::drawYAxis( bool leftside, ioDrawTool* drawtool )
{
    if ( !drawtool ) return;

    int baseline, bias;
    if ( leftside )
    {
	baseline = axislineposset_ ? axisrect.left() : wrd2uicnv->toUiX(minx_);
	bias = -ticlen;
    }
    else
    {
	baseline = axislineposset_ ? axisrect.right() : wrd2uicnv->toUiX(maxx_);
	bias = ticlen;
    }

    int top = wrd2uicnv->toUiY( maxy_ );
    int bot = wrd2uicnv->toUiY( miny_ );
    drawtool->drawLine( baseline, top, baseline, bot );
    
    for ( float y = miny_; y <= maxy_; y += stepy_ )
    {
	int wy = wrd2uicnv->toUiY( y );
	drawtool->drawLine( baseline, wy, baseline+bias, wy );
	BufferString txt = y;
	Alignment al = bias < 0 ? Alignment( Alignment::Stop,Alignment::Middle )
	    : Alignment( Alignment::Start,Alignment::Middle );
	drawtool->drawText( baseline+bias, wy , txt, al, true );
    }
}


void DrawAxis2D::drawGridLine( ioDrawTool* drawtool )
{
    if ( !drawtool ) return;

    int top = wrd2uicnv->toUiY( maxy_ );
    int bot = wrd2uicnv->toUiY( miny_ );
    for ( float x = minx_; x <= maxx_; x += stepx_ )
    {
	int wx = wrd2uicnv->toUiX( x );
	drawtool->drawLine( wx, top, wx, bot );
    }

    int left = wrd2uicnv->toUiX( minx_ );
    int right = wrd2uicnv->toUiX( maxx_ );
    for ( float y = miny_; y <= maxy_; y += stepy_ )
    {
	int wy = wrd2uicnv->toUiY( y );
	drawtool->drawLine( left, wy, right, wy );
    }
}
