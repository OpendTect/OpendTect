/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		July 2006
________________________________________________________________________

-*/

#include "uivisisosurface.h"

#include "coltabmapper.h"
#include "datadistributionextracter.h"
#include "marchingcubes.h"
#include "mousecursor.h"
#include "mouseevent.h"
#include "picksettr.h"
#include "seisdatapack.h"
#include "survinfo.h"
#include "uiaxishandler.h"
#include "uibutton.h"
#include "uihistogramdisplay.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uitaskrunner.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uistatsdisplay.h"
#include "vismarchingcubessurface.h"
#include "visvolumedisplay.h"


uiVisIsoSurfaceThresholdDlg::uiVisIsoSurfaceThresholdDlg( uiParent* p,
	visBase::MarchingCubesSurface* isosurface,
	visSurvey::VolumeDisplay* vd, int attrib )
    : uiDlgGroup( p, tr("Iso surface threshold") )
    , isosurfacedisplay_( isosurface )
    , initiallineitem_( 0 )
    , thresholdlineitem_( 0 )
    , isovallineitem_( 0 )
    , initialvalue_( vd->isoValue( isosurface ) )
    , vd_( vd )
{
    bool fullmode = vd_->isFullMode(isosurface);
    modefld_ = new uiGenInput( this, uiStrings::sMode(),
	    BoolInpSpec(true,tr("Full volume"),tr("Seed based")) );
    modefld_->setValue( fullmode );
    modefld_->valuechanged.notify(
	    mCB(this,uiVisIsoSurfaceThresholdDlg,modeChangeCB) );

    seedselfld_ = new uiIOObjSel( this, *mMkCtxtIOObj(PickSet),
				  uiStrings::sSeed(mPlural) );
    DBKey mid = vd_->getSeedsID( isosurface );
    if ( mid.isValid() )
	seedselfld_->setInput( mid );

    seedselfld_->display( !fullmode );
    seedselfld_->attach( alignedBelow, modefld_ );

    aboveisovaluefld_ = new uiGenInput( this, tr("Seeds value"),
	    BoolInpSpec(true,tr("Above iso-value"),tr("Below iso-value")) );
    aboveisovaluefld_->display( !fullmode );
    aboveisovaluefld_->setValue( vd_->seedAboveIsovalue(isosurface) );
    aboveisovaluefld_->attach( alignedBelow, seedselfld_ );

    uiStatsDisplay::Setup su; su.withtext(false);
    statsdisplay_ = new uiStatsDisplay( this, su );
    const RegularSeisDataPack* rsdp = vd_->getCacheVolume( attrib );
    if ( rsdp && rsdp->nrArrays() > 0 )
    {
	RangeLimitedDataDistributionExtracter<float> extr( *rsdp->arrayData(0),
				SilentTaskRunnerProvider() );
	statsdisplay_->funcDisp()->setDistribution( *extr.getDistribution() );
    }
    statsdisplay_->attach( leftAlignedBelow, aboveisovaluefld_ );

    funcDisp().setDragMode( uiGraphicsView::NoDrag );
    funcDisp().scene().getMouseEventHandler().buttonPressed.notify(
	    mCB(this,uiVisIsoSurfaceThresholdDlg,mousePressed) );
    funcDisp().scene().getMouseEventHandler().doubleClick.notify(
	    mCB(this,uiVisIsoSurfaceThresholdDlg,doubleClick) );
    funcDisp().reSize.notify(
	    mCB( this,uiVisIsoSurfaceThresholdDlg,reDrawCB) );

    thresholdfld_ = new uiGenInput( this, tr("Iso value"),
				    FloatInpSpec(initialvalue_) );
    thresholdfld_->setValue( vd_->isoValue(isosurface) );
    thresholdfld_->attach( leftAlignedBelow, statsdisplay_ );
    updatebutton_ = new uiPushButton( this, uiStrings::sUpdate(),
	    mCB(this,uiVisIsoSurfaceThresholdDlg,updatePressed), true );
    updatebutton_->attach( rightOf, thresholdfld_ );
}


uiVisIsoSurfaceThresholdDlg::~uiVisIsoSurfaceThresholdDlg()
{
}


void uiVisIsoSurfaceThresholdDlg::reDrawCB( CallBacker* )
{ drawHistogram(); }


bool uiVisIsoSurfaceThresholdDlg::acceptOK()
{
    if ( mIsUdf(thresholdfld_->getFValue()) )
    {
	uiMSG().error( tr("Please define the Iso threshold value.") );
	return false;
    }

    updatePressed( 0 );

    if ( !vd_->isFullMode(isosurfacedisplay_) &&
	  vd_->getSeedsID(isosurfacedisplay_).isInvalid() )
	return false;

    return true;
}


bool uiVisIsoSurfaceThresholdDlg::rejectOK()
{
    modefld_->setValue( vd_->isFullMode(isosurfacedisplay_) );
    aboveisovaluefld_->setValue( vd_->seedAboveIsovalue(isosurfacedisplay_) );
    seedselfld_->setInput( vd_->getSeedsID(isosurfacedisplay_) );

    return revertChanges();
}


bool uiVisIsoSurfaceThresholdDlg::revertChanges()
{
    const float curvalue = vd_->isoValue( isosurfacedisplay_ );
    const float prec = (float) ( (curvalue+initialvalue_) * 5e-4 );
    if ( !mIsEqual(curvalue,initialvalue_,prec) )
	updateIsoDisplay(initialvalue_);

    return true;
}


void uiVisIsoSurfaceThresholdDlg::updatePressed(CallBacker*)
{
    const float oldthreshold = vd_->isoValue( isosurfacedisplay_ );
    const float newthreshold = thresholdfld_->getFValue();
    if ( mIsUdf(newthreshold) )
	return;

    const bool fullmode = modefld_->getBoolValue();
    const bool aboveisoval = aboveisovaluefld_->getBoolValue();
    DBKey dbky;
    if ( !fullmode && seedselfld_->commitInput() &&
	  seedselfld_->ctxtIOObj().ioobj_ )
	dbky = seedselfld_->ctxtIOObj().ioobj_->key();

    if ( isosurfacedisplay_->getSurface() &&
	!isosurfacedisplay_->getSurface()->isEmpty() )
    {

	if ( mIsEqual(oldthreshold, newthreshold,
		      (oldthreshold+newthreshold)*5e-5) )
	{
	    if ( fullmode )
	    {
		if ( vd_->isFullMode(isosurfacedisplay_) == 1 )
		    return;
	    }
	    else if ( !vd_->isFullMode(isosurfacedisplay_) )
	    {
		bool isseedabv = vd_->seedAboveIsovalue(isosurfacedisplay_)==1;
		if ( aboveisoval==isseedabv &&
		     dbky==vd_->getSeedsID(isosurfacedisplay_) )
		    return;
	    }
	}
    }

    vd_->setFullMode( isosurfacedisplay_, fullmode );
    if ( !fullmode )
    {
	if ( dbky.isInvalid() )
	{
	    uiMSG().error(tr("Cannot find input seeds"));
	    return;
	}

	vd_->setSeedAboveIsovalue( isosurfacedisplay_, aboveisoval );
	if ( seedselfld_->ctxtIOObj().ioobj_ )
	    vd_->setSeedsID( isosurfacedisplay_, dbky );
    }

    updateIsoDisplay( newthreshold );
}


void uiVisIsoSurfaceThresholdDlg::mousePressed( CallBacker* cb )
{
    handleClick( cb, false );
}


void uiVisIsoSurfaceThresholdDlg::modeChangeCB( CallBacker* )
{
    const bool fullmode = modefld_->getBoolValue();
    seedselfld_->display( !fullmode );
    aboveisovaluefld_->display( !fullmode );
}


void uiVisIsoSurfaceThresholdDlg::doubleClick( CallBacker* cb )
{
    handleClick( cb, true );
}


uiHistogramDisplay& uiVisIsoSurfaceThresholdDlg::funcDisp()
{
    return *statsdisplay_->funcDisp();
}


uiAxisHandler& uiVisIsoSurfaceThresholdDlg::xAxis()
{
    return *statsdisplay_->funcDisp()->xAxis();
}


void uiVisIsoSurfaceThresholdDlg::handleClick( CallBacker* cb, bool isdouble )
{
    MouseEventHandler& eventhandler = funcDisp().scene().getMouseEventHandler();
    if ( eventhandler.isHandled() )
	return;

    const MouseEvent& event = eventhandler.event();
    if ( !event.leftButton() || event.rightButton() || event.middleButton() ||
	 event.ctrlStatus() || event.altStatus() || event.shiftStatus() )
	return;

    const uiPoint& pt = event.pos();

    eventhandler.setHandled( true );
    const float val = xAxis().getVal( pt.x_ );
    thresholdfld_->setValue( val );
    if ( isdouble )
	updateIsoDisplay( val );
    else
	drawHistogram();
}


void uiVisIsoSurfaceThresholdDlg::updateIsoDisplay( float nv )
{
    uiTaskRunner taskrunner( this );
    vd_->setIsoValue( isosurfacedisplay_, nv, &taskrunner );
    drawHistogram();
}


void uiVisIsoSurfaceThresholdDlg::drawHistogram()
{
    OD::LineStyle ls;
    ls.width_ = 2;
    const uiAxisHandler* yaxis = funcDisp().yAxis(false);
    const int valytop = yaxis->getPix( yaxis->range().start );
    const int valybottom = yaxis->getPix( yaxis->range().stop );
    if ( !mIsUdf(initialvalue_) )
    {
	ls.color_ = Color(0,150,0);
	const int valx = xAxis().getPix(initialvalue_);
	if ( valx < xAxis().getPix(xAxis().range().start) ||
	     valx > xAxis().getPix(xAxis().range().stop) )
	    return;

	if ( !initiallineitem_ )
	{
	    initiallineitem_ = funcDisp().scene().addItem( new uiLineItem() );
	    initiallineitem_->setPenStyle( ls );
	    initiallineitem_->setZValue( 3 );
	}

	initiallineitem_->setLine( valx, valytop, valx, valybottom );
    }

    if ( !mIsUdf(thresholdfld_->getFValue()) )
    {
	ls.color_ = Color(0,255,0,0);
	const int valx = xAxis().getPix(thresholdfld_->getFValue());
	if ( valx < xAxis().getPix(xAxis().range().start) ||
	     valx > xAxis().getPix(xAxis().range().stop) )
	    return;

	if ( !thresholdlineitem_ )
	{
	    thresholdlineitem_ = funcDisp().scene().addItem( new uiLineItem() );
	    thresholdlineitem_->setPenStyle( ls );
	    thresholdlineitem_->setZValue( 3 );
	}

	thresholdlineitem_->setLine( valx, valytop, valx, valybottom );
    }

    if ( !mIsUdf(vd_->isoValue(isosurfacedisplay_) ) )
    {
	ls.color_ = Color(255,0,0,0);
	const int valx = xAxis().getPix( vd_->isoValue( isosurfacedisplay_) );
	if ( valx < xAxis().getPix(xAxis().range().start) ||
	     valx > xAxis().getPix(xAxis().range().stop) )
	    return;

	if ( !isovallineitem_ )
	{
	    isovallineitem_ = funcDisp().scene().addItem( new uiLineItem() );
	    isovallineitem_->setPenStyle( ls );
	    isovallineitem_->setZValue( 2 );
	}

	isovallineitem_->setLine( valx, valytop, valx, valybottom );
    }
}
