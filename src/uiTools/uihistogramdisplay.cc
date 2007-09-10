/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Duntao Wei
 Date:          Mid 2005
 RCS:           $Id: uihistogramdisplay.cc,v 1.5 2007-09-10 06:37:29 cvskris Exp $
________________________________________________________________________

-*/

#include "uihistogramdisplay.h"

#include "iodrawtool.h"

uiHistogramDisplay::uiHistogramDisplay( ioDrawArea* da )
    : drawarea_( da )
    , ignoreextremes_( true )
{
    histogram_ = &ownhistogram_;
    scale_ = &ownscale_;
}


uiHistogramDisplay::~uiHistogramDisplay()
{ }


void uiHistogramDisplay::setHistogram( const TypeSet<float>& histogram,
		   const SamplingData<float>& xaxis, bool copy )
{
    if ( copy )
    {
	ownhistogram_ = histogram;
	histogram_ = &ownhistogram_;
	ownscale_ = xaxis;
	scale_ = &ownscale_;
    }
    else
    {
	histogram_ = &histogram;
	scale_ = &xaxis;
	ownhistogram_.erase();
    }

    pointlist_.erase();
}


void uiHistogramDisplay::setTransform( const uiWorld2Ui& w2u )
{
    transform_ = w2u;
    pointlist_.erase();
}


void uiHistogramDisplay::touch()
{
    pointlist_.erase();
}


const uiWorld2Ui& uiHistogramDisplay::getTransform() const
{
    return transform_;
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


void uiHistogramDisplay::reDraw( CallBacker* )
{
    const int histogramsize = histogram_->size();
    if ( !pointlist_.size() && histogramsize )
    {
	float maxval = 0;
	for ( int idx=0; idx<histogramsize; idx++ )
	{
	    if ( ignoreextremes_ && ( !idx || idx==histogramsize-1 ) )
		continue;

	    if ( (*histogram_)[idx] > maxval ) maxval = (*histogram_)[idx];
	}

	uiPoint prevpt(0,transform_.world2UiData().sz.height());
	bool prevcommitted = false;

	for ( int idx=0; idx<histogramsize; idx++ )
	{
	    if ( ignoreextremes_ && ( !idx || idx==histogramsize-1 ) )
		continue;

	    const float newx = scale_
		? scale_->atIndex( idx )
		: (float)idx / (histogramsize-1);

	    const float newy = (*histogram_)[idx] *
		transform_.world2UiData().wr.top() / maxval;

	    const uiPoint newpt( transform_.transform(uiWorldPoint(newx,newy)));
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
	pointlist_ += uiPoint( pointlist_[pointlist_.size()-1].x+1,
			       transform_.world2UiData().sz.height() );
    }

    ioDrawTool& dt = drawarea_->drawTool();

    dt.setPenColor( color_ );
    dt.setFillColor( color_ );
    dt.drawPolygon( pointlist_ );
}
