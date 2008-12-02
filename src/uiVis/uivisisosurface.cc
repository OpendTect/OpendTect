/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		July 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uivisisosurface.cc,v 1.13 2008-12-02 03:30:32 cvssatyaki Exp $";

#include "uivisisosurface.h"

#include "mouseevent.h"
#include "uistatsdisplay.h"
#include "uifunctiondisplay.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
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
    funcDisp().scene().getMouseEventHandler().buttonPressed.notify(
	    mCB( this, uiVisIsoSurfaceThresholdDlg,mousePressed) );
    funcDisp().scene().getMouseEventHandler().doubleClick.notify(
	    mCB( this, uiVisIsoSurfaceThresholdDlg,doubleClick) );
    statsdisplay_->setHistogram( histogram, rg );

    thresholdfld_ = new uiGenInput( this, "Iso value",
	    			    FloatInpSpec(initialvalue_) );
    thresholdfld_->attach( alignedBelow, statsdisplay_ );
    updatebutton_ = new uiPushButton( this, "Update",
	    mCB( this, uiVisIsoSurfaceThresholdDlg, updatePressed ), false );
    updatebutton_->attach( rightOf, thresholdfld_ );
    drawHistogram();
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
    return *statsdisplay_->funcDisp()->xAxis();
}


void uiVisIsoSurfaceThresholdDlg::handleClick( CallBacker* cb, bool isdouble )
{
    MouseEventHandler& eventhandler = 
	funcDisp().scene().getMouseEventHandler();
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
}


void uiVisIsoSurfaceThresholdDlg::updateIsoDisplay( float nv )
{
    MouseCursorChanger changer( MouseCursor::Wait );
    vd_->setIsoValue( isosurfacedisplay_, nv );
}


void uiVisIsoSurfaceThresholdDlg::drawHistogram()
{
    LineStyle ls;
    ls.width_ = 2;
    uiLineItem* lineitem = new uiLineItem();
    if ( !mIsUdf(initialvalue_) )
    {
	ls.color_ = Color(0,150,0);
	const int val = xAxis().getPix(initialvalue_);
	lineitem = funcDisp().scene().addLine( 
	    val, 0, val, funcDisp().width() );
	lineitem->setPenStyle( ls );
    }

    if ( !mIsUdf(thresholdfld_->getfValue() ) )
    {
	ls.color_ = Color(0,255,0,0); 
	const int val = xAxis().getPix(thresholdfld_->getfValue());
	lineitem = funcDisp().scene().addLine( 
	    val, 0, val, funcDisp().width() );
    }

    if ( !mIsUdf(vd_->isoValue( isosurfacedisplay_ ) ) )
    {
	ls.color_ = Color(255,0,0,0);
	const int val = xAxis().getPix( vd_->isoValue( isosurfacedisplay_) );
	lineitem = funcDisp().scene().addLine( 
	    val, 0, val, funcDisp().width() );
    }
}
