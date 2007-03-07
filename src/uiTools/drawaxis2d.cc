/*+
________________________________________________________________________

 CopyRight:     ( C ) dGB Beheer B.V.
 Author:        Duntao Wei
 Date:          Mar. 2005
 RCS:           $Id: drawaxis2d.cc,v 1.6 2007-03-07 10:40:38 cvsbert Exp $
________________________________________________________________________

-*/

#include "drawaxis2d.h"

#include "linear.h"
#include "draw.h"
#include "iodrawtool.h"
#include "uiworld2ui.h"

#define mTickLen	5


DrawAxis2D::DrawAxis2D(ioDrawArea* da )
    : drawarea_( da )
    , inside_(false)
    , drawaxisline_(true)
    , xaxis_( 0, 1, 1 )
    , yaxis_( 0, 1, 1 )
    , useuirect_( false )
{ }


void DrawAxis2D::setDrawArea( ioDrawArea* da )
{
    drawarea_ = da; 
}


void DrawAxis2D::setDrawRectangle( const uiRect* ur )
{
    if ( !ur )
	useuirect_ = false;

    uirect_ = *ur;
    useuirect_ = true;
}


void DrawAxis2D::setup( const uiWorldRect& wr )
{
    xaxis_.start = wr.left();
    xaxis_.stop = wr.right();
    xaxis_.step = AxisLayout(Interval<float>( wr.left(), wr.right())).sd.step;

    yaxis_.start = wr.top();
    yaxis_.stop = wr.bottom();
    yaxis_.step = AxisLayout(Interval<float>( wr.top(), wr.bottom())).sd.step;
}


void DrawAxis2D::setup( const StepInterval<float>& xrg,
			const StepInterval<float>& yrg )
{
    xaxis_ = xrg;
    yaxis_ = yrg;
}


void DrawAxis2D::drawAxes( bool xdir, bool ydir,
			   bool topside, bool leftside ) const
{
    if ( xdir ) drawXAxis( topside );
    if ( ydir ) drawYAxis( leftside );
}


void DrawAxis2D::drawXAxis( bool topside ) const
{
    const uiRect drawarea = getDrawArea();
    const uiWorld2Ui transform( uiWorldRect( xaxis_.start, yaxis_.start,
					     xaxis_.stop, yaxis_.stop ),
				drawarea.getPixelSize() );

    int baseline, bias;
    if ( topside )
    {
	baseline = drawarea.top();
	bias = inside_ ? mTickLen : -mTickLen;
    }
    else
    {
	baseline = drawarea.bottom();
	bias = inside_ ? -mTickLen : mTickLen;
    }

    if ( drawaxisline_ )
	drawarea_->drawTool()->drawLine( drawarea.left(), baseline,
				       drawarea.right(), baseline );
   
    float x = xaxis_.start; 
    while ( x<=xaxis_.stop )
    {
	const int wx = transform.toUiX( x ) + drawarea.left();
	drawarea_->drawTool()->drawLine( wx, baseline, wx, baseline+bias );

	BufferString txt = x;
	Alignment al( Alignment::Middle, Alignment::Start );
	float textbias = bias;
	if ( bias<0 ) al.ver = Alignment::Stop;
	drawarea_->drawTool()->drawText( wx, baseline+bias, txt, al );

	x += xaxis_.step;
    }
}


void DrawAxis2D::drawYAxis( bool leftside ) const
{
    const uiRect drawarea = getDrawArea();
    const uiWorld2Ui transform(
	    uiWorldRect( xaxis_.start, yaxis_.start,
			 xaxis_.stop, yaxis_.stop ),
	    drawarea.getPixelSize() );

    int baseline, bias;
    if ( leftside )
    {
	baseline = drawarea.left();
	bias = inside_ ? mTickLen : -mTickLen;
    }
    else
    {
	baseline = drawarea.right();
	bias = inside_ ? -mTickLen : mTickLen;
    }

    if ( drawaxisline_ )
	drawarea_->drawTool()->drawLine( baseline, drawarea.top(),
				       baseline, drawarea.bottom() );
    
    float y = yaxis_.start; 
    while ( y<=yaxis_.stop )
    {
	const int wy = transform.toUiY( y ) + drawarea.top();
	drawarea_->drawTool()->drawLine( baseline, wy, baseline+bias, wy );

	const BufferString txt = y;
	Alignment al( Alignment::Start, Alignment::Middle );
	if ( bias < 0 ) al.hor = Alignment::Stop;
	drawarea_->drawTool()->drawText( baseline+bias, wy , txt, al );

	y += yaxis_.step;
    }
}


void DrawAxis2D::drawGridLines( bool xdir, bool ydir ) const
{
    const uiRect drawarea = getDrawArea();
    const uiWorld2Ui transform( uiWorldRect( xaxis_.start, yaxis_.start,
					     xaxis_.stop, yaxis_.stop ),
				drawarea.getPixelSize() );
    if ( xdir )
    {
	const int top = drawarea.top();
	const int bot = drawarea.bottom();
	float x = xaxis_.start; 
	while ( x<=xaxis_.stop )
	{
	    const int wx = transform.toUiX( x ) + drawarea.left();
	    drawarea_->drawTool()->drawLine( wx, top, wx, bot );
	    x += xaxis_.step;
	}
    }

    if ( ydir )
    {
	const int left = drawarea.left();
	const int right = drawarea.right();
	float y = yaxis_.start; 
	while ( y<=yaxis_.stop )
	{
	    const int wy = transform.toUiY( y ) + drawarea.top();
	    drawarea_->drawTool()->drawLine( left, wy, right, wy );
	    y += yaxis_.step;
	}
    }
}


uiRect DrawAxis2D::getDrawArea() const
{
    if ( useuirect_ )
	return uirect_;

    return uiRect( 0, 0, drawarea_->drawTool()->getDevWidth(),
	    	   drawarea_->drawTool()->getDevHeight() );
}
