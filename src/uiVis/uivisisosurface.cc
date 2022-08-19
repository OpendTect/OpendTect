/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivisisosurface.h"

#include "coltabmapper.h"
#include "marchingcubes.h"
#include "mousecursor.h"
#include "mouseevent.h"
#include "picksettr.h"
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
    bool fullmode = vd->isFullMode(isosurface);
    modefld_ = new uiGenInput( this, tr("Mode"),
	    BoolInpSpec(true,tr("Full volume"),tr("Seed based")) );
    modefld_->setValue( fullmode );
    modefld_->valuechanged.notify(
	    mCB(this,uiVisIsoSurfaceThresholdDlg,modeChangeCB) );

    seedselfld_ = new uiIOObjSel( this, *mMkCtxtIOObj(PickSet), tr("Seeds") );
    MultiID mid = vd->getSeedsID( isosurface );
    if ( !mid.isUdf() )
    	seedselfld_->setInput( mid );

    seedselfld_->display( !fullmode );
    seedselfld_->attach( alignedBelow, modefld_ );

    aboveisovaluefld_ = new uiGenInput( this, tr("Seeds value"),
	    BoolInpSpec(true,tr("Above iso-value"),tr("Below iso-value")) );
    aboveisovaluefld_->display( !fullmode );
    aboveisovaluefld_->setValue( vd->seedAboveIsovalue(isosurface) );
    aboveisovaluefld_->attach( alignedBelow, seedselfld_ );

    TypeSet<float> histogram;
    if ( vd->getHistogram(attrib) ) histogram = *vd->getHistogram(attrib);
    const ColTab::MapperSetup* ms = vd->getColTabMapperSetup( attrib );
    const Interval<float> rg = ms->range_;

    uiStatsDisplay::Setup su; su.withtext(false);
    statsdisplay_ = new uiStatsDisplay( this, su );
    statsdisplay_->funcDisp()->setHistogram( histogram, rg );
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
    thresholdfld_->setValue( vd->isoValue(isosurface) );
    thresholdfld_->attach( leftAlignedBelow, statsdisplay_ );
    updatebutton_ = new uiPushButton( this, tr("Update"),
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
	uiMSG().error( tr("Please define the threshhold.") );
	return false;
    }

    updatePressed( 0 );

    if ( !vd_->isFullMode(isosurfacedisplay_) &&
	  vd_->getSeedsID(isosurfacedisplay_).isUdf() )
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
    if ( mIsZero(curvalue,mDefEps) && mIsZero(initialvalue_,mDefEps) )
	return true;

    const float prec = mCast(float,(curvalue+initialvalue_)*5e-4);
    if ( !mIsEqual(curvalue,initialvalue_,prec) )
	updateIsoDisplay(initialvalue_);

    return true;
}


void uiVisIsoSurfaceThresholdDlg::updatePressed(CallBacker*)
{
    const float oldthreshhold = vd_->isoValue( isosurfacedisplay_ );
    const float newthreshhold = thresholdfld_->getFValue();
    if ( mIsUdf(newthreshhold) )
	return;

    const bool fullmode = modefld_->getBoolValue();
    const bool aboveisoval = aboveisovaluefld_->getBoolValue();
    MultiID mid;
    mid.setUdf();
    if ( !fullmode && seedselfld_->commitInput() &&
	  seedselfld_->ctxtIOObj().ioobj_ )
	mid = seedselfld_->ctxtIOObj().ioobj_->key();

    if ( isosurfacedisplay_->getSurface() &&
	!isosurfacedisplay_->getSurface()->isEmpty() )
    {

	if ( mIsEqual(oldthreshhold, newthreshhold,
		      (oldthreshhold+newthreshhold)*5e-5) )
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
		     mid==vd_->getSeedsID(isosurfacedisplay_) )
		    return;
	    }
	}
    }

    vd_->setFullMode( isosurfacedisplay_, fullmode );
    if ( !fullmode )
    {
	if ( mid.isUdf() )
	{
	    uiMSG().error(tr("Cannot find input seeds"));
	    return;
	}

	vd_->setSeedAboveIsovalue( isosurfacedisplay_, aboveisoval );
	if ( seedselfld_->ctxtIOObj().ioobj_ )
    	    vd_->setSeedsID( isosurfacedisplay_, mid );
    }

    updateIsoDisplay( newthreshhold );
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
    const float val = xAxis().getVal( pt.x );
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
	ls.color_ = OD::Color(0,150,0);
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
	ls.color_ = OD::Color(0,255,0,0);
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
	ls.color_ = OD::Color(255,0,0,0);
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
