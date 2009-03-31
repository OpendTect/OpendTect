/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uidirectionalplot.cc,v 1.1 2009-03-31 12:13:10 cvsbert Exp $";

#include "uidirectionalplot.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "mouseevent.h"
#include <iostream>


uiDirectionalPlot::uiDirectionalPlot( uiParent* p,
				      const uiDirectionalPlot::Setup& su )
    : uiGraphicsView(p,"Function display viewer",true)
    , setup_(su)
    , selsector_(0)
    , selpart_(0)
    , equicircles_(0)
    , sectorlines_(0)
    , sectorPartSelected(this)
{
    setZoomOnCtrlScroll( false );
    setPrefWidth( setup_.prefsize_.width() );
    setPrefHeight( setup_.prefsize_.height() );
    setStretch( 2, 2 );
    getMouseEventHandler().buttonReleased.notify(
			    mCB(this,uiDirectionalPlot,mouseRelease) );

    reSize.notify( mCB(this,uiDirectionalPlot,reSized) );
    setScrollBarPolicy( true, uiGraphicsView::ScrollBarAlwaysOff );
    setScrollBarPolicy( false, uiGraphicsView::ScrollBarAlwaysOff );
    draw();
}


uiDirectionalPlot::~uiDirectionalPlot()
{
}


void uiDirectionalPlot::reSized( CallBacker* )
{
    draw();
}


void uiDirectionalPlot::setVals( const float* vals, int sz )
{
    data_.erase();
    for ( int idx=0; idx<sz; idx++ )
    {
	Stats::SectorData* sd = new Stats::SectorData;
	*sd += Stats::SectorPartData( vals[idx] );
	data_ += sd;
    }

    gatherInfo(); draw();
}


void uiDirectionalPlot::setVals( const Stats::DirectionalData& dird )
{
    data_ = dird;

    gatherInfo(); draw();
}


void uiDirectionalPlot::gatherInfo()
{
}


void uiDirectionalPlot::draw()
{
    if ( data_.isEmpty() ) return;
}


#define mGetMousePos()  \
    if ( getMouseEventHandler().isHandled() ) \
	return; \
    const MouseEvent& ev = getMouseEventHandler().event(); \
    if ( !(ev.buttonState() & OD::RightButton) ) \
        return; \
    const bool isctrl = ev.ctrlStatus(); \
    const bool isoth = ev.shiftStatus() || ev.altStatus(); \
    const bool isnorm = !isctrl && !isoth; \
    if ( !isnorm ) return


void uiDirectionalPlot::mouseRelease( CallBacker* )
{
    mGetMousePos();
}


bool uiDirectionalPlot::isRose() const
{
    if ( data_.isEmpty() )
	return false;
    for ( int isect=0; isect<data_.size(); isect++ )
    {
	if ( data_.nrParts(isect) != 1 )
	    return false;
    }
    return true;
}
