/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Duntao Wei
 Date:          Mid 2005
 RCS:           $Id: uihistogramdisplay.cc,v 1.1 2007-01-24 14:30:35 cvskris Exp $
________________________________________________________________________

-*/

#include "uihistogramdisplay.h"

#include "iodrawtool.h"

uiHistogramDisplay::uiHistogramDisplay( ioDrawArea* da )
    : drawarea_( da )
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


void uiHistogramDisplay::reDraw( CallBacker* )
{
    const int histogramsize = histogram_->size();
    if ( !pointlist_.size() && histogramsize )
    {
	float maxval = 0;
	//TODO should go from 1 to size-1
	for ( int idx=0; idx<histogramsize; idx++ )
	    if ( (*histogram_)[idx] > maxval ) maxval = (*histogram_)[idx];

	uiPoint prevpt(0,0);
	pointlist_ += prevpt; //add start point

	for ( int idx=0; idx<histogram_->size(); idx++ )
	{
	    //TODO normalize to worldbox
	    const float newx = (float)idx / (histogramsize-1);
	    const float newy = (*histogram_)[idx] *
		transform_.world2UiData().wr.top() / maxval;

	    const uiPoint newpt( transform_.transform(uiWorldPoint(newx,newy)));
	    if ( newpt.y!=prevpt.y )
	    {
		pointlist_ += prevpt;
		pointlist_ += newpt;
		prevpt = newpt;
	    }
	    else if ( idx==histogramsize-1 )
		pointlist_ += newpt;
	    else
		prevpt = newpt;
	}

	//Add closing pos	
	pointlist_ += uiPoint( pointlist_[pointlist_.size()-1].x+1,0 );
    }

    ioDrawTool* drawtool = drawarea_->drawTool();

    drawtool->beginDraw();
    drawtool->setPenColor( color_ );
    drawtool->setFillColor( color_ );
    drawtool->drawPolygon( pointlist_ );
    drawtool->endDraw();
}
