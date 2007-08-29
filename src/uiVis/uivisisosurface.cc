/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		July 2006
 RCS:		$Id: uivisisosurface.cc,v 1.3 2007-08-29 14:25:51 cvskris Exp $
________________________________________________________________________

-*/

#include "uivisisosurface.h"

#include "iodrawtool.h"
#include "isosurface.h"
#include "mouseevent.h"
#include "uicanvas.h"
#include "uigeninput.h"
#include "uihistogramdisplay.h"
#include "uibutton.h"
#include "visvolumedisplay.h"

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

    histogramdisplay_ = new uiCanvas( this );
    histogramdisplay_->setPrefWidth( 200 );
    histogramdisplay_->setPrefHeight( 100 );
    histogramdisplay_->getMouseEventHandler().buttonPressed.notify(
	    mCB( this, uiVisIsoSurfaceThresholdDlg,mousePressed) );
    histogramdisplay_->getMouseEventHandler().doubleClick.notify(
	    mCB( this, uiVisIsoSurfaceThresholdDlg,doubleClick) );

    histogrampainter_ = new uiHistogramDisplay( histogramdisplay_ );
    histogrampainter_->setHistogram( histogram,SamplingData<float>(0,1), true );
    histogrampainter_->setColor( Color(128,128,128,0) );

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

    eventhandler.setHandled( true );
    uiPoint pt = event.pos();
    const float val = histogrampainter_->getTransform().transform( pt ).x;
    thresholdfld_->setValue( val );
    if ( isdouble ) updateIsoDisplay( val );
}


void uiVisIsoSurfaceThresholdDlg::updateIsoDisplay( float nv )
{
    vd_->setIsoValue( isosurfacedisplay_, nv );
}


void uiVisIsoSurfaceThresholdDlg::updateHistogramDisplay( CallBacker* )
{
    ioDrawTool& dt = histogramdisplay_->drawTool();

    dt.setPenColor( Color::Black );
    uiWorldPoint wpt( initialvalue_, 0 );
    uiPoint pt =  histogrampainter_->getTransform().transform( wpt );
    dt.drawLine( pt.x, 0, pt.x, dt.getDevHeight() );

    dt.setPenColor( Color(255,0,0,0) );
    
    wpt.x = vd_->isoValue( isosurfacedisplay_ );
    pt =  histogrampainter_->getTransform().transform( wpt );
    dt.drawLine( pt.x, 0, pt.x, dt.getDevHeight() );
}
