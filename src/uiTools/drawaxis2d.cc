/*+
________________________________________________________________________

 CopyRight:     ( C ) dGB Beheer B.V.
 Author:        Duntao Wei
 Date:          Mar. 2005
 RCS:           $Id: drawaxis2d.cc,v 1.9 2007-03-13 20:07:35 cvskris Exp $
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
    , xaxis_( 0, 1 )
    , yaxis_( 0, 1 )
    , xrg_( 0, 1 )
    , yrg_( 0, 1 )
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
    xrg_.start = wr.left();
    xrg_.stop = wr.right();
    xaxis_ = AxisLayout(Interval<float>( wr.left(), wr.right())).sd;

    yrg_.start = wr.top();
    yrg_.stop = wr.bottom();
    yaxis_ = AxisLayout(Interval<float>( wr.top(), wr.bottom())).sd;
}


void DrawAxis2D::setup( const StepInterval<float>& xrg,
			const StepInterval<float>& yrg )
{
    xrg_.setFrom( xrg );
    yrg_.setFrom( yrg );
    xaxis_ = SamplingData<double>( xrg.start, xrg.step );
    yaxis_ = SamplingData<double>( yrg.start, yrg.step );
}


void DrawAxis2D::drawAxes( bool xdir, bool ydir,
			   bool topside, bool leftside ) const
{
    if ( xdir ) drawXAxis( topside );
    if ( ydir ) drawYAxis( leftside );
}

#define mLoopStart( dim ) \
    const int nrsteps = mNINT(dim##rg_.width(false)/dim##axis_.step)+1; \
    for ( int idx=0; idx<nrsteps; idx++ ) \
    { \
	const double dim##pos = dim##axis_.atIndex(idx); \
	if ( !dim##rg_.includes(dim##pos,true) ) \
	    continue 


void DrawAxis2D::drawXAxis( bool topside ) const
{
    const uiRect drawarea = getDrawArea();
    const uiWorld2Ui transform( uiWorldRect( xrg_.start, yrg_.start,
					     xrg_.stop, yrg_.stop ),
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

    mLoopStart( x );
	BufferString text;
	const double displaypos = getAnnotTextAndPos(true,xpos,&text);
	const int wx = transform.toUiX( displaypos ) + drawarea.left();
	drawarea_->drawTool()->drawLine( wx, baseline, wx, baseline+bias );

	Alignment al( Alignment::Middle, Alignment::Start );
	if ( bias<0 ) al.ver = Alignment::Stop;
	drawarea_->drawTool()->drawText( wx, baseline+bias, text.buf(), al );
    }
}


void DrawAxis2D::drawYAxis( bool leftside ) const
{
    const uiRect drawarea = getDrawArea();
    const uiWorld2Ui transform(
	    uiWorldRect( xrg_.start, yrg_.start,
			 xrg_.stop, yrg_.stop ),
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
    
    mLoopStart( y );
	BufferString text;
	const double displaypos =
	    getAnnotTextAndPos( false, ypos, &text );
	const int wy = transform.toUiY( displaypos ) + drawarea.top();
	drawarea_->drawTool()->drawLine( baseline, wy, baseline+bias, wy );

	Alignment al( Alignment::Start, Alignment::Middle );
	if ( bias < 0 ) al.hor = Alignment::Stop;
	drawarea_->drawTool()->drawText( baseline+bias, wy , text.buf(), al );
    }
}


void DrawAxis2D::drawGridLines( bool xdir, bool ydir ) const
{
    const uiRect drawarea = getDrawArea();
    const uiWorld2Ui transform( uiWorldRect( xrg_.start, yrg_.start,
					     xrg_.stop, yrg_.stop ),
				drawarea.getPixelSize() );
    if ( xdir )
    {
	const int top = drawarea.top();
	const int bot = drawarea.bottom();
	mLoopStart( x );
	    const double displaypos = getAnnotTextAndPos( true, xpos );
	    const int wx = transform.toUiX( displaypos ) + drawarea.left();
	    drawarea_->drawTool()->drawLine( wx, top, wx, bot );
	}
    }

    if ( ydir )
    {
	const int left = drawarea.left();
	const int right = drawarea.right();
	mLoopStart( y );
	    const double displaypos = getAnnotTextAndPos(false, ypos);
	    const int wy = transform.toUiY( displaypos ) + drawarea.top();
	    drawarea_->drawTool()->drawLine( left, wy, right, wy );
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


double DrawAxis2D::getAnnotTextAndPos( bool isx, double proposedpos,
				     BufferString* text ) const
{
    if ( text ) (*text) = proposedpos;
    return proposedpos;
}
