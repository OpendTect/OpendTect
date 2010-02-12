/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Feb 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiodviewer2d.cc,v 1.22 2010-02-12 08:49:31 cvsumesh Exp $";

#include "uiodviewer2d.h"

#include "uiattribpartserv.h"
#include "uiemviewer2dmanager.h"
#include "uiflatauxdataeditor.h"
#include "uiflatviewdockwin.h"
#include "uiflatviewer.h"
#include "uiflatviewmainwin.h"
#include "uiflatviewslicepos.h"
#include "uiflatviewstdcontrol.h"
#include "uiflatviewpropdlg.h"
#include "uigraphicsscene.h"
#include "uiodmain.h"
#include "uiodscenemgr.h"
#include "uirgbarraycanvas.h"
#include "uitoolbar.h"
#include "uivispartserv.h"

#include "attribdatacubes.h"
#include "attribdatapack.h"
#include "attribdataholder.h"
#include "attribsel.h"
#include "emhorizonpainter.h"
#include "horflatvieweditor.h"
#include "mpefssflatvieweditor.h"
#include "settings.h"

#include "visseis2ddisplay.h"

void initSelSpec( Attrib::SelSpec& as )
{ as.set( 0, Attrib::SelSpec::cNoAttrib(), false, 0 ); }

uiODViewer2D::uiODViewer2D( uiODMain& appl, int visid )
    : appl_(appl)
    , visid_(visid)
    , vdselspec_(*new Attrib::SelSpec)
    , wvaselspec_(*new Attrib::SelSpec)
    , viewwin_(0)
    , horfveditor_(0)
    , fssfveditor_(0)
    , emviewer2dman_(0)
    , slicepos_(0)
{
    basetxt_ = "2D Viewer - ";
    BufferString info;
    appl.applMgr().visServer()->getObjectInfo( visid, info );
    if ( info.isEmpty() )
	info = appl.applMgr().visServer()->getObjectName( visid );
    basetxt_ += info;

    initSelSpec( vdselspec_ );
    initSelSpec( wvaselspec_ );
}


uiODViewer2D::~uiODViewer2D()
{
    if ( viewstdcontrol_ && viewstdcontrol_->propDialog() )
	viewstdcontrol_->propDialog()->close();

    mDynamicCastGet(uiFlatViewDockWin*,fvdw,viewwin_)
    if ( fvdw )
	appl_.removeDockWindow( fvdw );

    if ( horfveditor_ )
    {
	horfveditor_->updateoldactivevolinuimpeman.remove(
		mCB(this,uiODViewer2D,updateOldActiveVolInUiMPEManCB) );
	horfveditor_->restoreactivevolinuimpeman.remove(
		mCB(this,uiODViewer2D,restoreActiveVolInUiMPEManCB) );
	horfveditor_->updateseedpickingstatus.remove(
		mCB(this,uiODViewer2D,updateHorFlatViewerSeedPickStatus) );
    }

    delete horfveditor_;
    delete viewwin_;
}


void uiODViewer2D::setUpView( DataPack::ID packid, bool wva )
{
    DataPack* dp = DPM(DataPackMgr::FlatID()).obtain( packid, false );
    mDynamicCastGet(Attrib::Flat3DDataPack*,dp3d,dp)
    mDynamicCastGet(Attrib::Flat2DDataPack*,dp2d,dp)
    mDynamicCastGet(Attrib::Flat2DDHDataPack*,dp2ddh,dp)
    const bool isnew = !viewwin_;
    if ( isnew )
	createViewWin( (dp3d && dp3d->isVertical()) ||
		       (dp2d && dp2d->isVertical()) );

    if ( slicepos_ )
	slicepos_->getToolBar()->display( dp3d );

    if ( dp3d )
    {
	const CubeSampling& cs = dp3d->cube().cubeSampling();
	if ( slicepos_ ) slicepos_->setCubeSampling( cs );
	if ( dp3d->isVertical() )
	{
	    emviewer2dman_->setCubeSampling( cs );
	    horfveditor_->setCubeSampling( cs );
	    horfveditor_->setSelSpec( &vdselspec_, false );
	    horfveditor_->setSelSpec( &wvaselspec_, true );
	}
	fssfveditor_->setCubeSampling( cs );
    }
    else if ( dp2ddh )
    {
	emviewer2dman_->setPainterLineName(
		appl_.applMgr().visServer()->getObjectName(visid_) );
	dp2ddh->getPosDataTable( emviewer2dman_->getHorPainter()->getTrcNos(),
			emviewer2dman_->getHorPainter()->getDistances());
	emviewer2dman_->setCubeSampling(
			dp2ddh->dataholder().getCubeSampling() );
	horfveditor_->setCubeSampling( dp2ddh->dataholder().getCubeSampling() );
	horfveditor_->setSelSpec( &vdselspec_, false );
	horfveditor_->setSelSpec( &wvaselspec_, true );
	horfveditor_->setLineName(
		appl_.applMgr().visServer()->getObjectName(visid_) );
	mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
			appl_.applMgr().visServer()->getObject(visid_));
	if ( s2d )
	{
	    horfveditor_->setLineSetID( s2d->lineSetID() );
	    horfveditor_->set2D( true );
	}
    }

    fssfveditor_->drawFault();

    DataPack::ID curpackid = viewwin_->viewer().packID( wva );
    DPM(DataPackMgr::FlatID()).release( curpackid );

    FlatView::DataDispPars& ddp = viewwin_->viewer().appearance().ddpars_;
    (wva ? ddp.wva_.show_ : ddp.vd_.show_) = true;

    viewwin_->viewer().setPack( wva, packid, false, isnew );
    viewwin_->start();
}


void uiODViewer2D::createViewWin( bool isvert )
{    
    bool wantdock = false;
    Settings::common().getYN( "FlatView.Use Dockwin", wantdock );
    uiParent* controlparent = 0;
    if ( !wantdock )
    {
	uiFlatViewMainWin* fvmw = new uiFlatViewMainWin( 0,
		uiFlatViewMainWin::Setup(basetxt_).deleteonclose(true)
	       					  .withhanddrag(true) );
	fvmw->windowClosed.notify( mCB(this,uiODViewer2D,winCloseCB) );

	slicepos_ = new uiSlicePos2DView( fvmw );
	slicepos_->positionChg.notify( mCB(this,uiODViewer2D,posChg) );
	viewwin_ = fvmw;
    }
    else
    {
	uiFlatViewDockWin* dwin = new uiFlatViewDockWin( &appl_,
				   uiFlatViewDockWin::Setup(basetxt_) );
	appl_.addDockWindow( *dwin, uiMainWin::Top );
	dwin->setFloating( true );
	viewwin_ = dwin;
	controlparent = &appl_;
    }

    viewwin_->setInitialSize( 600, 400 );
    for ( int ivwr=0; ivwr<viewwin_->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewwin_->viewer( ivwr );
	vwr.appearance().setDarkBG( wantdock );
	vwr.appearance().setGeoDefaults(isvert);
	vwr.appearance().annot_.setAxesAnnot(true);
	if ( ivwr == 0 )
	{
	    viewstdcontrol_ = new uiFlatViewStdControl( vwr,
		    uiFlatViewStdControl::Setup(controlparent).helpid("51.0.0")
							      .withedit(true) );
	    viewwin_->addControl( viewstdcontrol_ );
	    auxdataeditor_ = new uiFlatViewAuxDataEditor( vwr );
	    emviewer2dman_ = new EM::uiEMViewer2DManager( vwr, auxdataeditor_ );
	    emviewer2dman_->initEMFlatViewControl( & appl_,
		    				   viewstdcontrol_->toolBar() );
	    auxdataeditor_->setSelActive( false );
	    horfveditor_ = new MPE::HorizonFlatViewEditor( auxdataeditor_ );
	    horfveditor_->updateoldactivevolinuimpeman.notify(
		    mCB(this,uiODViewer2D,updateOldActiveVolInUiMPEManCB) );
	    horfveditor_->restoreactivevolinuimpeman.notify(
		    mCB(this,uiODViewer2D,restoreActiveVolInUiMPEManCB) );
	    horfveditor_->updateseedpickingstatus.notify(
		     mCB(this,uiODViewer2D,updateHorFlatViewerSeedPickStatus) );
	    horfveditor_->setMouseEventHandler( 
		    	&vwr.rgbCanvas().scene().getMouseEventHandler() );
	    fssfveditor_ = new MPE::FaultStickSetFlatViewEditor(auxdataeditor_);
	    fssfveditor_->setMouseEventHandler(
		    	&vwr.rgbCanvas().scene().getMouseEventHandler() );
	    vwr.dataChanged.notify(  mCB(this,uiODViewer2D,dataChangedCB) );
	}
    }
}


void uiODViewer2D::winCloseCB( CallBacker* cb )
{
    DataPack::ID packid = viewwin_->viewer().packID( true );
    DPM(DataPackMgr::FlatID()).release( packid );
    packid = viewwin_->viewer().packID( false );
    DPM(DataPackMgr::FlatID()).release( packid );

    if ( horfveditor_ )
    {
	horfveditor_->updateoldactivevolinuimpeman.remove(
		mCB(this,uiODViewer2D,updateOldActiveVolInUiMPEManCB) );
	horfveditor_->restoreactivevolinuimpeman.remove(
		mCB(this,uiODViewer2D,restoreActiveVolInUiMPEManCB) );
	horfveditor_->updateseedpickingstatus.remove(
		mCB(this,uiODViewer2D,updateHorFlatViewerSeedPickStatus) );
    }
	
    delete horfveditor_; horfveditor_ = 0;

    mDynamicCastGet(uiMainWin*,mw,cb)
    if ( mw ) mw->windowClosed.remove( mCB(this,uiODViewer2D,winCloseCB) );
    if ( slicepos_ )
	slicepos_->positionChg.remove( mCB(this,uiODViewer2D,posChg) );

    if ( viewstdcontrol_ && viewstdcontrol_->propDialog() )
	viewstdcontrol_->propDialog()->close();

    viewwin_ = 0;
}


void uiODViewer2D::setSelSpec( const Attrib::SelSpec* as, bool wva )
{
    if ( as )
	(wva ? wvaselspec_ : vdselspec_) = *as;
    else
	initSelSpec( wva ? wvaselspec_ : vdselspec_ );
}


void uiODViewer2D::posChg( CallBacker* )
{
    if ( !slicepos_ ) return;

    CubeSampling cs = slicepos_->getCubeSampling();
    uiAttribPartServer* attrserv = appl_.applMgr().attrServer();

    if ( vdselspec_.id().asInt() > -1 )
    {
	attrserv->setTargetSelSpec( vdselspec_ );
	DataPack::ID dpid = attrserv->createOutput( cs, DataPack::cNoID() );
	setUpView( dpid, false );
    }

    if ( wvaselspec_.id().asInt() > -1 )
    {
	attrserv->setTargetSelSpec( wvaselspec_ );
	DataPack::ID dpid = attrserv->createOutput( cs, DataPack::cNoID() );
	setUpView( dpid, true );
    }
}


void uiODViewer2D::dataChangedCB( CallBacker* )
{
    const FlatDataPack* wdp = viewwin_->viewer().pack( true );
    if ( wdp && !wdp->name().isEmpty() )
    {
	if ( wvaselspec_.userRef() && 
	     !strcmp(wdp->name().buf(),wvaselspec_.userRef()) )
	    horfveditor_->setSelSpec( &wvaselspec_, true );
	else if ( vdselspec_.userRef() &&
		  !strcmp(wdp->name().buf(),vdselspec_.userRef()) )
	    horfveditor_->setSelSpec( &vdselspec_, true );
    }
    else
	horfveditor_->setSelSpec( 0, true );

    const FlatDataPack* vddp = viewwin_->viewer().pack( false );
    if ( vddp && !vddp->name().isEmpty() )
    {
	if ( wvaselspec_.userRef() &&
	     !strcmp(vddp->name().buf(),wvaselspec_.userRef()) )
	    horfveditor_->setSelSpec( &wvaselspec_, false );
	else if ( vdselspec_.userRef() &&
		  !strcmp(vddp->name().buf(),vdselspec_.userRef()) )
	    horfveditor_->setSelSpec( &vdselspec_, false );
    }
    else
	horfveditor_->setSelSpec( 0, false );
}


void uiODViewer2D::updateOldActiveVolInUiMPEManCB( CallBacker* )
{
    appl_.applMgr().visServer()->updateOldActiVolInuiMPEMan();
}


void uiODViewer2D::restoreActiveVolInUiMPEManCB( CallBacker* )
{
    if ( !appl_.applMgr().visServer()->isTrackingSetupActive() )
	appl_.applMgr().visServer()->restoreActiveVolInuiMPEMan();
}


void uiODViewer2D::updateHorFlatViewerSeedPickStatus( CallBacker* )
{
    horfveditor_->setSeedPickingStatus(
	    appl_.applMgr().visServer()->isPicking() );
    horfveditor_->setTrackerSetupActive(
	    appl_.applMgr().visServer()->isTrackingSetupActive() );
}
