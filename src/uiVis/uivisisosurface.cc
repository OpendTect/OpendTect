/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		July 2006
 RCS:		$Id: uivisisosurface.cc,v 1.9 2008-04-01 15:43:03 cvsbert Exp $
________________________________________________________________________

-*/

#include "uivisisosurface.h"

#include "iodrawtool.h"
#include "mouseevent.h"
#include "uistatsdisplay.h"
#include "uifunctiondisplay.h"
#include "uiaxishandler.h"
#include "mousecursor.h"
#include "uigeninput.h"
#include "uibutton.h"
#include "visvolumedisplay.h"
#include "viscolortab.h"


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

    uiStatsDisplay::Setup su; su.withtext(false);
    statsdisplay_ = new uiStatsDisplay( this, su );
    funcDisp().getMouseEventHandler().buttonPressed.notify(
	    mCB( this, uiVisIsoSurfaceThresholdDlg,mousePressed) );
    funcDisp().getMouseEventHandler().doubleClick.notify(
	    mCB( this, uiVisIsoSurfaceThresholdDlg,doubleClick) );
    funcDisp().postDraw.notify(
	    mCB(this,uiVisIsoSurfaceThresholdDlg,updateHistogramDisplay) );
    statsdisplay_->setHistogram( histogram, rg );

    thresholdfld_ = new uiGenInput( this, "Iso value",
	    			    FloatInpSpec(initialvalue_) );
    thresholdfld_->attach( alignedBelow, statsdisplay_ );
    updatebutton_ = new uiPushButton( this, "Update",
	    mCB( this, uiVisIsoSurfaceThresholdDlg, updatePressed ), false );
    updatebutton_->attach( rightOf, thresholdfld_ );
}


uiVisIsoSurfaceThresholdDlg::~uiVisIsoSurfaceThresholdDlg()
{
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


uiFunctionDisplay& uiVisIsoSurfaceThresholdDlg::funcDisp()
{
    return *statsdisplay_->funcDisp();
}


uiAxisHandler& uiVisIsoSurfaceThresholdDlg::xAxis()
{
    return *statsdisplay_->funcDisp()->getXAxis();
}


void uiVisIsoSurfaceThresholdDlg::handleClick( CallBacker* cb, bool isdouble )
{
    MouseEventHandler& eventhandler = funcDisp().getMouseEventHandler();
    if ( eventhandler.isHandled() )
	return;

    const MouseEvent& event = eventhandler.event();
    if ( !event.leftButton() || event.rightButton() || event.middleButton() ||
	 event.ctrlStatus() || event.altStatus() || event.shiftStatus() )
	return;

    const uiPoint& pt = event.pos();

    eventhandler.setHandled( true );
    const float val = xAxis().getVal( pt.x );
    thresholdfld_->setValue( val );
    if ( isdouble )	updateIsoDisplay( val );
    else		funcDisp().update();
}


void uiVisIsoSurfaceThresholdDlg::updateIsoDisplay( float nv )
{
    MouseCursorChanger changer( MouseCursor::Wait );
    vd_->setIsoValue( isosurfacedisplay_, nv );
    funcDisp().update();
}


void uiVisIsoSurfaceThresholdDlg::updateHistogramDisplay( CallBacker* cb )
{
    ioDrawTool& dt = funcDisp().drawTool();

    dt.setPenWidth( 2 );
    if ( !mIsUdf(initialvalue_) )
    {
	dt.setPenColor( Color(0,150,0) );
	const int val = xAxis().getPix(initialvalue_);
	dt.drawLine( val, 0, val, dt.getDevHeight() );
    }

    if ( !mIsUdf(thresholdfld_->getfValue() ) )
    {
	dt.setPenColor( Color(0,255,0,0) );
	const int val = xAxis().getPix(thresholdfld_->getfValue());
	dt.drawLine( val, 0, val, dt.getDevHeight() );
    }

    if ( !mIsUdf(vd_->isoValue( isosurfacedisplay_ ) ) )
    {
	dt.setPenColor( Color(255,0,0,0) );
	const int val = xAxis().getPix( vd_->isoValue( isosurfacedisplay_) );
	dt.drawLine( val, 0, val, dt.getDevHeight() );
    }

    dt.setPenWidth( 1 );
}
