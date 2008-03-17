/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		July 2006
 RCS:		$Id: uivisisosurface.cc,v 1.8 2008-03-17 15:26:54 cvskris Exp $
________________________________________________________________________

-*/

#include "uivisisosurface.h"

#include "iodrawtool.h"
#include "mouseevent.h"
#include "uicanvas.h"
#include "mousecursor.h"
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
    const SamplingData<float> sd(0,rg.start, histogram.size()-1, rg.stop );

    histogramdisplay_ = new uiCanvas( this );
    histogramdisplay_->setPrefWidth( mHistWidth );
    histogramdisplay_->setPrefHeight( mHistHeight );
    histogramdisplay_->getMouseEventHandler().buttonPressed.notify(
	    mCB( this, uiVisIsoSurfaceThresholdDlg,mousePressed) );
    histogramdisplay_->getMouseEventHandler().doubleClick.notify(
	    mCB( this, uiVisIsoSurfaceThresholdDlg,doubleClick) );

    histogrampainter_ = new uiHistogramDisplay( histogramdisplay_ );
    histogrampainter_->setHistogram( histogram,sd );
    histogrampainter_->setColor( Color(100,100,100,0) );
    histogramdisplay_->postDraw.notify(
	    mCB(this,uiVisIsoSurfaceThresholdDlg,updateHistogramDisplay) );

    thresholdfld_ = new uiGenInput( this, "Iso value",
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
    const float val = histogrampainter_->getXValue( pt.x );
    thresholdfld_->setValue( val );
    if ( isdouble ) updateIsoDisplay( val );
    else histogramdisplay_->update();
}


void uiVisIsoSurfaceThresholdDlg::updateIsoDisplay( float nv )
{
    MouseCursorChanger changer( MouseCursor::Wait );
    vd_->setIsoValue( isosurfacedisplay_, nv );
    histogramdisplay_->update();
}


void uiVisIsoSurfaceThresholdDlg::updateHistogramDisplay( CallBacker* cb )
{
    histogrampainter_->reDraw( cb );
    ioDrawTool& dt = histogramdisplay_->drawTool();

    if ( !mIsUdf(initialvalue_) )
    {
	dt.setPenColor( Color::Black );
	const int val = histogrampainter_->getPixel(initialvalue_);
	dt.drawLine( val, 0, val, dt.getDevHeight() );
    }

    if ( !mIsUdf(thresholdfld_->getfValue() ) )
    {
	dt.setPenColor( Color(0,255,0,0) );
	const int val = histogrampainter_->getPixel(thresholdfld_->getfValue());
	dt.drawLine( val, 0, val, dt.getDevHeight() );
    }

    if ( !mIsUdf(vd_->isoValue( isosurfacedisplay_ ) ) )
    {
	dt.setPenColor( Color(255,0,0,0) );
	const int val =
	    histogrampainter_->getPixel( vd_->isoValue( isosurfacedisplay_ ) );
	dt.drawLine( val, 0, val, dt.getDevHeight() );
    }
}
