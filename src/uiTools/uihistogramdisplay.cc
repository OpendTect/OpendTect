/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Duntao Wei
 Date:          Mid 2005
 RCS:           $Id: uihistogramdisplay.cc,v 1.9 2007-12-18 07:30:26 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uihistogramdisplay.h"

#include "iodrawtool.h"
#include "drawaxis2d.h"

uiHistogramDisplay::uiHistogramDisplay( ioDrawArea* da )
    : drawarea_( da )
    , ignoreextremes_( true )
    , width_( -1 )
    , height_( -1 )
{}


uiHistogramDisplay::~uiHistogramDisplay()
{}


void uiHistogramDisplay::setHistogram( const TypeSet<float>& histogram,
		   const SamplingData<float>& xaxis )
{
    histogram_ = histogram;
    scale_ = xaxis;
    pointlist_.erase();
}


float uiHistogramDisplay::getXValue( int pixel ) const
{
    const float factor = ((float) pixel) / width_;
    return scale_.atIndex( factor*histogram_.size() );
}


int uiHistogramDisplay::getPixel( float val ) const
{
    const float factor = scale_.getIndex( val )/histogram_.size();
    return mNINT( factor * width_ );
}


const Color& uiHistogramDisplay::getColor() const
{ return color_; }


void uiHistogramDisplay::setColor( const Color& nc )
{ color_ = nc; }


bool uiHistogramDisplay::ignoresExtremes() const
{ return ignoreextremes_; }


void uiHistogramDisplay::setIgnoresExtremes(bool yn)
{
    if ( yn==ignoreextremes_ ) return;

    ignoreextremes_ = yn;
    pointlist_.erase();
}


void uiHistogramDisplay::setBoundaryRect( const uiRect& bdrect )
{ boundary_ = bdrect; }


void uiHistogramDisplay::setXAxis( const StepInterval<float>& xrg )
{ xrg_ = xrg; }


void uiHistogramDisplay::drawXAxis( const StepInterval<float>& xrg )
{
    ioDrawTool& dt = drawarea_->drawTool();
    dt.setPenColor( Color( 0,0,0 ) );
    dt.setPenWidth( 1 );

    const int height = dt.getDevHeight();
    const int width = dt.getDevWidth();

    DrawAxis2D drwaxis( drawarea_ );
    drwaxis.setup( uiWorldRect(xrg.start,1,xrg.stop,0) );
    uiRect rect( boundary_.left(), height, width-boundary_.right(),
	         height-boundary_.bottom() );
    drwaxis.setDrawRectangle( &rect );
    drwaxis.drawXAxis( false );
}


void uiHistogramDisplay::reDraw( CallBacker* )
{
    ioDrawTool& dt = drawarea_->drawTool();
    const int height = dt.getDevHeight();
    const int width = dt.getDevWidth();

    const int usableheight = height - boundary_.top() - boundary_.bottom();
    const int usablewidth = width - boundary_.left() - boundary_.right();

    const int histogramsize = histogram_.size();
    if ( !pointlist_.size() && histogramsize
	 || height_!=height || width_!=width )
    {
	pointlist_.erase();

	float maxval = 0;
	for ( int idx=0; idx<histogramsize; idx++ )
	{
	    if ( ignoreextremes_ && ( !idx || idx==histogramsize-1 ) )
		continue;

	    if ( histogram_[idx]>maxval ) maxval = histogram_[idx];
	}

	uiPoint prevpt( boundary_.left(), height-boundary_.bottom() );
	bool prevcommitted = false;

	for ( int idx=0; idx<histogramsize; idx++ )
	{
	    if ( ignoreextremes_ && ( !idx || idx==histogramsize-1 ) )
		continue;

	    const float newxf = boundary_.left() + 
		                (float)idx/histogramsize*usablewidth;
	    const float newyf = boundary_.top() + usableheight-
		                histogram_[idx]/maxval*usableheight;
	    const uiPoint newpt( mNINT(newxf),mNINT(newyf) );
	    if ( newpt.y!=prevpt.y )
	    {
		if ( !prevcommitted )
		    pointlist_ += prevpt;
		pointlist_ += newpt;
		prevcommitted = true;
		prevpt = newpt;
	    }
	    else if ( idx==histogramsize-1 )
		pointlist_ += newpt;
	    else
	    {
		prevcommitted = false;
		prevpt = newpt;
	    }
	}

	//Add closing pos
	if ( pointlist_.size() > 0 )
	    pointlist_ += uiPoint( pointlist_[pointlist_.size()-1].x+1,
				   height-boundary_.bottom() );

	height_ = height;
	width_ = width;
    }

    dt.setPenColor( color_ );
    dt.setFillColor( color_ );
    dt.drawPolygon( pointlist_ );

    if ( xrg_.width() > 0 )
	drawXAxis( xrg_ );
}
