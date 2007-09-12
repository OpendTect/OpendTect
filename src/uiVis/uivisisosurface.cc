/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		July 2006
 RCS:		$Id: uivisisosurface.cc,v 1.5 2007-09-12 10:44:20 cvskris Exp $
________________________________________________________________________

-*/

#include "uivisisosurface.h"

#include "iodrawtool.h"
#include "mouseevent.h"
#include "uicanvas.h"
#include "uicursor.h"
#include "uigeninput.h"
#include "uihistogramdisplay.h"
#include "uibutton.h"
#include "visvolumedisplay.h"
#include "viscolortab.h"

#define mHistWidth 200
#define mHistHeight 100

uiVisIsoSurfaceThresholdDlg::uiVisIsoSurfaceThresholdDlg( uiParent* p,
	visBase::MarchingCubesSurface* isosurface,
	visSurvey::VolumeDisplay* vd )
    : uiDlgGroup( p, "Iso surface threshold" )
    , isosurfacedisplay_( isosurface )
    , initialvalue_( vd->isoValue( isosurface ) )
    , vd_( vd )
{
    TypeSet<float> histogram;
    if ( vd->getHistogram(0) ) histogram = *vd->getHistogram(0);

    const Interval<float> rg = vd->getColorTab().getInterval();
    uiWorld2Ui w2ui( uiWorldRect(rg.start,255,rg.stop,0),
	    	     uiSize(mHistWidth,mHistHeight) );
    const SamplingData<float> sd(0,rg.start, histogram.size()-1, rg.stop );

    histogramdisplay_ = new uiCanvas( this );
    histogramdisplay_->setPrefWidth( mHistWidth );
    histogramdisplay_->setPrefHeight( mHistHeight );
    histogramdisplay_->getMouseEventHandler().buttonPressed.notify(
	    mCB( this, uiVisIsoSurfaceThresholdDlg,mousePressed) );
    histogramdisplay_->getMouseEventHandler().doubleClick.notify(
	    mCB( this, uiVisIsoSurfaceThresholdDlg,doubleClick) );

    histogrampainter_ = new uiHistogramDisplay( histogramdisplay_ );
    histogrampainter_->setTransform( w2ui );
    histogrampainter_->setHistogram( histogram,sd, true );
    histogrampainter_->setColor( Color(100,100,100,0) );
    histogramdisplay_->postDraw.notify(
	    mCB(this,uiVisIsoSurfaceThresholdDlg,updateHistogramDisplay) );

    thresholdfld_ = new uiGenInput( this, "Threshold",
	    			    FloatInpSpec(initialvalue_) );
    thresholdfld_->attach( alignedBelow, histogramdisplay_ );
    updatebutton_ = new uiPushButton( this, "Update",
	    mCB( this, uiVisIsoSurfaceThresholdDlg, updatePressed ), false );
    updatebutton_->attach( rightOf, thresholdfld_ );
}


uiVisIsoSurfaceThresholdDlg::~uiVisIsoSurfaceThresholdDlg()
{
    histogramdisplay_->getMouseEventHandler().buttonPressed.remove(
	    mCB( this, uiVisIsoSurfaceThresholdDlg,mousePressed) );
    histogramdisplay_->getMouseEventHandler().doubleClick.remove(
	    mCB( this, uiVisIsoSurfaceThresholdDlg,doubleClick) );

    histogramdisplay_->postDraw.remove(
	    mCB(this,uiVisIsoSurfaceThresholdDlg,updateHistogramDisplay) );
    delete histogrampainter_;
}


bool uiVisIsoSurfaceThresholdDlg::acceptOK()
{
    const float curvalue = vd_->isoValue( isosurfacedisplay_ );
    const float fldvalue = thresholdfld_->getfValue();
    const float prec = (curvalue+fldvalue)/2000;
    if ( !mIsEqual(curvalue,fldvalue,prec) )
	updateIsoDisplay( fldvalue );

    return true;
}


bool uiVisIsoSurfaceThresholdDlg::rejectOK()
{
    return revertChanges();
}


bool uiVisIsoSurfaceThresholdDlg::revertChanges()
{
    const float curvalue = vd_->isoValue( isosurfacedisplay_ );
    const float prec = (curvalue+initialvalue_)/2000;
    if ( !mIsEqual(curvalue,initialvalue_,prec) )
	updateIsoDisplay(initialvalue_);

    return true;
}


void uiVisIsoSurfaceThresholdDlg::updatePressed(CallBacker*)
{
    updateIsoDisplay( thresholdfld_->getfValue() );
}


void uiVisIsoSurfaceThresholdDlg::mousePressed( CallBacker* cb )
{
    handleClick( cb, false );
}


void uiVisIsoSurfaceThresholdDlg::doubleClick( CallBacker* cb )
{
    handleClick( cb, true );
}


void uiVisIsoSurfaceThresholdDlg::handleClick( CallBacker* cb, bool isdouble )
{
    MouseEventHandler& eventhandler = histogramdisplay_->getMouseEventHandler();
    if ( eventhandler.isHandled() )
	return;

    const MouseEvent& event = eventhandler.event();
    if ( !event.leftButton() || event.rightButton() || event.middleButton() ||
	 event.ctrlStatus() || event.altStatus() || event.shiftStatus() )
    {
	return;
    }

    const uiPoint& pt = event.pos();
    if ( pt.x<0 || pt.x>=mHistWidth )
	return;

    eventhandler.setHandled( true );
    const float val = histogrampainter_->getTransform().transform( pt ).x;
    thresholdfld_->setValue( val );
    if ( isdouble ) updateIsoDisplay( val );
    else histogramdisplay_->update();
}


void uiVisIsoSurfaceThresholdDlg::updateIsoDisplay( float nv )
{
    uiCursorChanger changer( uiCursor::Wait );
    vd_->setIsoValue( isosurfacedisplay_, nv );
    histogramdisplay_->update();
}


void uiVisIsoSurfaceThresholdDlg::updateHistogramDisplay( CallBacker* cb )
{
    histogrampainter_->reDraw( cb );
    ioDrawTool& dt = histogramdisplay_->drawTool();

    uiWorldPoint wpt( initialvalue_, 0 );
    if ( !mIsUdf(wpt.x) )
    {
	dt.setPenColor( Color::Black );
	uiPoint pt =  histogrampainter_->getTransform().transform( wpt );
	dt.drawLine( pt.x, 0, pt.x, dt.getDevHeight() );
    }

    wpt.x = thresholdfld_->getfValue();
    if ( !mIsUdf(wpt.x) )
    {
	dt.setPenColor( Color(0,255,0,0) );
	uiPoint pt =  histogrampainter_->getTransform().transform( wpt );
	dt.drawLine( pt.x, 0, pt.x, dt.getDevHeight() );
    }

    wpt.x = vd_->isoValue( isosurfacedisplay_ );
    if ( !mIsUdf(wpt.x) )
    {
	dt.setPenColor( Color(255,0,0,0) );
	uiPoint pt =  histogrampainter_->getTransform().transform( wpt );
	dt.drawLine( pt.x, 0, pt.x, dt.getDevHeight() );
    }
}
