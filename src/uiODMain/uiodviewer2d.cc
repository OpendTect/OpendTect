/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Feb 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiodviewer2d.cc,v 1.31 2010-04-16 03:33:22 cvsnanne Exp $";

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
#include "uiwelltoseismicmainwin.h"

#include "attribdatacubes.h"
#include "attribdatapack.h"
#include "attribdataholder.h"
#include "attribsel.h"
#include "emhorizonpainter.h"
#include "horflatvieweditor.h"
#include "mpef3dflatvieweditor.h"
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

    mDynamicCastGet(uiFlatViewDockWin*,fvdw,viewwin())
    if ( fvdw )
	appl_.removeDockWindow( fvdw );

    for ( int idx=0; idx<horfveditors_.size(); idx++ )
    {
	horfveditors_[idx]->updateoldactivevolinuimpeman.remove(
		mCB(this,uiODViewer2D,updateOldActiveVolInUiMPEManCB) );
	horfveditors_[idx]->restoreactivevolinuimpeman.remove(
		mCB(this,uiODViewer2D,restoreActiveVolInUiMPEManCB) );
	horfveditors_[idx]->updateseedpickingstatus.remove(
		mCB(this,uiODViewer2D,updateHorFlatViewerSeedPickStatus) );
    }

    deepErase ( horfveditors_ );
    deepErase( fssfveditors_ );
    deepErase( f3dfveditors_ );
    deepErase( auxdataeditors_ );
    deepErase( emviewer2dmans_ );

    delete viewwin();
}


void uiODViewer2D::setUpView( DataPack::ID packid, bool wva )
{
    DataPack* dp = DPM(DataPackMgr::FlatID()).obtain( packid, false );
    mDynamicCastGet(Attrib::Flat3DDataPack*,dp3d,dp)
    mDynamicCastGet(Attrib::Flat2DDataPack*,dp2d,dp)
    mDynamicCastGet(Attrib::Flat2DDHDataPack*,dp2ddh,dp)
    const bool isnew = !viewwin();
    if ( isnew )
	createViewWin( (dp3d && dp3d->isVertical()) ||
		       (dp2d && dp2d->isVertical()) );

    if ( slicepos_ )
	slicepos_->getToolBar()->display( dp3d );

    for ( int ivwr=0; ivwr<viewwin()->nrViewers(); ivwr++ )
    {
	if ( dp3d )
	{
	    const CubeSampling& cs = dp3d->cube().cubeSampling();
	    if ( slicepos_ ) slicepos_->setCubeSampling( cs );
	    if ( dp3d->isVertical() )
	    {
		emviewer2dmans_[ivwr]->setCubeSampling( cs );
		horfveditors_[ivwr]->setCubeSampling( cs );
		horfveditors_[ivwr]->setSelSpec( &vdselspec_, false );
		horfveditors_[ivwr]->setSelSpec( &wvaselspec_, true );
	    }
	    fssfveditors_[ivwr]->setCubeSampling( cs );
	    f3dfveditors_[ivwr]->setCubeSampling( cs );
	}
	else if ( dp2ddh )
	{
	    emviewer2dmans_[ivwr]->setPainterLineName(
		    appl_.applMgr().visServer()->getObjectName(visid_) );
	    dp2ddh->getPosDataTable( 
		    emviewer2dmans_[ivwr]->getHorPainter()->getTrcNos(),
		    emviewer2dmans_[ivwr]->getHorPainter()->getDistances() );
	    dp2ddh->getPosDataTable( fssfveditors_[ivwr]->getTrcNos(),
				     fssfveditors_[ivwr]->getDistances() );
	    dp2ddh->getCoordDataTable( fssfveditors_[ivwr]->getTrcNos(),
				       fssfveditors_[ivwr]->getCoords() );
	    emviewer2dmans_[ivwr]->setCubeSampling(
		    dp2ddh->dataholder().getCubeSampling() );
	    horfveditors_[ivwr]->setCubeSampling( 
		    dp2ddh->dataholder().getCubeSampling() );
	    horfveditors_[ivwr]->setSelSpec( &vdselspec_, false );
	    horfveditors_[ivwr]->setSelSpec( &wvaselspec_, true );
	    horfveditors_[ivwr]->setLineName(
		    appl_.applMgr().visServer()->getObjectName(visid_) );
	    fssfveditors_[ivwr]->setLineName(
		    appl_.applMgr().visServer()->getObjectName(visid_) );
	    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    appl_.applMgr().visServer()->getObject(visid_));
	    if ( s2d )
	    {
		horfveditors_[ivwr]->setLineSetID( s2d->lineSetID() );
		fssfveditors_[ivwr]->setLineID( s2d->lineSetID() );
		horfveditors_[ivwr]->set2D( true );
		fssfveditors_[ivwr]->set2D( true );
	    }
	}
	fssfveditors_[ivwr]->drawFault();
	f3dfveditors_[ivwr]->drawFault();

	DataPack::ID curpackid = viewwin()->viewer(ivwr).packID( wva );
	DPM(DataPackMgr::FlatID()).release( curpackid );

	FlatView::DataDispPars& ddp = 
	    		viewwin()->viewer(ivwr).appearance().ddpars_;
	(wva ? ddp.wva_.show_ : ddp.vd_.show_) = true;
	viewwin()->viewer(ivwr).setPack( wva, packid, false, isnew );
    }
    viewwin()->start();
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
	uiFlatViewer& vwr = viewwin()->viewer( ivwr);
	vwr.appearance().setDarkBG( wantdock );
	vwr.appearance().setGeoDefaults(isvert);
	vwr.appearance().annot_.setAxesAnnot(true);
    }

    uiFlatViewer& mainvwr = viewwin()->viewer();
    viewstdcontrol_ = new uiFlatViewStdControl( mainvwr,
	    uiFlatViewStdControl::Setup(controlparent).helpid("51.0.0")
						      .withedit(true) );
    viewwin_->addControl( viewstdcontrol_ );
    createViewWinEditors();
}


void uiODViewer2D::createViewWinEditors()
{   
    for ( int ivwr=0; ivwr<viewwin()->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewwin()->viewer( ivwr);
	uiFlatViewAuxDataEditor* adeditor = new uiFlatViewAuxDataEditor( vwr );
	auxdataeditors_ += adeditor;

	EM::uiEMViewer2DManager* emviewer2dman = 
	    		new EM::uiEMViewer2DManager( vwr, adeditor );
	emviewer2dman->initEMFlatViewControl( &appl_,
					      viewstdcontrol_->toolBar() );
	emviewer2dmans_ += emviewer2dman;

	adeditor->setSelActive( false );

	MPE::HorizonFlatViewEditor* horfveditor =
	    		new MPE::HorizonFlatViewEditor( adeditor );
	horfveditor->updateoldactivevolinuimpeman.notify(
		mCB(this,uiODViewer2D,updateOldActiveVolInUiMPEManCB) );
	horfveditor->restoreactivevolinuimpeman.notify(
		mCB(this,uiODViewer2D,restoreActiveVolInUiMPEManCB) );
	horfveditor->updateseedpickingstatus.notify(
		 mCB(this,uiODViewer2D,updateHorFlatViewerSeedPickStatus) );
	horfveditor->setMouseEventHandler( 
		    &vwr.rgbCanvas().scene().getMouseEventHandler() );
	horfveditors_ += horfveditor;

	MPE::FaultStickSetFlatViewEditor* fssfveditor =
	    		new MPE::FaultStickSetFlatViewEditor( adeditor );
	fssfveditor->setMouseEventHandler(
		    &vwr.rgbCanvas().scene().getMouseEventHandler() );
	fssfveditors_ += fssfveditor;

	MPE::Fault3DFlatViewEditor* f3dfveditor = 
	    		new MPE::Fault3DFlatViewEditor( adeditor );
	f3dfveditor->setMouseEventHandler(
		    &vwr.rgbCanvas().scene().getMouseEventHandler() );
	f3dfveditors_ += f3dfveditor;

	vwr.dataChanged.notify( mCB(this,uiODViewer2D,dataChangedCB) );
    }
} 


void uiODViewer2D::winCloseCB( CallBacker* cb )
{
    DataPack::ID packid = viewwin()->viewer().packID( true );
    DPM(DataPackMgr::FlatID()).release( packid );
    packid = viewwin()->viewer().packID( false );
    DPM(DataPackMgr::FlatID()).release( packid );

    for ( int idx=0; idx<horfveditors_.size(); idx++ )
    {
	horfveditors_[idx]->updateoldactivevolinuimpeman.remove(
		mCB(this,uiODViewer2D,updateOldActiveVolInUiMPEManCB) );
	horfveditors_[idx]->restoreactivevolinuimpeman.remove(
		mCB(this,uiODViewer2D,restoreActiveVolInUiMPEManCB) );
	horfveditors_[idx]->updateseedpickingstatus.remove(
		mCB(this,uiODViewer2D,updateHorFlatViewerSeedPickStatus) );
    }
	
    deepErase( horfveditors_ );
    deepErase( fssfveditors_ );
    deepErase( f3dfveditors_ );
    deepErase( auxdataeditors_ );
    deepErase( emviewer2dmans_ );

    mDynamicCastGet(uiMainWin*,mw,cb)
    if ( mw ) mw->windowClosed.remove( mCB(this,uiODViewer2D,winCloseCB) );
    if ( slicepos_ )
	slicepos_->positionChg.remove( mCB(this,uiODViewer2D,posChg) );

    if ( viewstdcontrol_ && viewstdcontrol_->propDialog() )
	viewstdcontrol_->propDialog()->close();

    viewstdcontrol_ = 0;
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
    for ( int ivwr=0; ivwr<viewwin()->nrViewers(); ivwr++ )
    {
	const FlatDataPack* wdp = viewwin()->viewer().pack( true );
	if ( wdp && !wdp->name().isEmpty() )
	{
	    if ( wvaselspec_.userRef() && 
		 !strcmp(wdp->name().buf(),wvaselspec_.userRef()) )
		horfveditors_[ivwr]->setSelSpec( &wvaselspec_, true );
	    else if ( vdselspec_.userRef() &&
		      !strcmp(wdp->name().buf(),vdselspec_.userRef()) )
		horfveditors_[ivwr]->setSelSpec( &vdselspec_, true );
	}
	else
	    horfveditors_[ivwr]->setSelSpec( 0, true );

	const FlatDataPack* vddp = viewwin()->viewer().pack( false );
	if ( vddp && !vddp->name().isEmpty() )
	{
	    if ( wvaselspec_.userRef() &&
		 !strcmp(vddp->name().buf(),wvaselspec_.userRef()) )
		horfveditors_[ivwr]->setSelSpec( &wvaselspec_, false );
	    else if ( vdselspec_.userRef() &&
		      !strcmp(vddp->name().buf(),vdselspec_.userRef()) )
		horfveditors_[ivwr]->setSelSpec( &vdselspec_, false );
	}
	else
	    horfveditors_[ivwr]->setSelSpec( 0, false );
    }
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
    for ( int ivwr=0; ivwr<viewwin()->nrViewers(); ivwr++ )
    {
	horfveditors_[ivwr]->setSeedPickingStatus(
		appl_.applMgr().visServer()->isPicking() );
	horfveditors_[ivwr]->setTrackerSetupActive(
		appl_.applMgr().visServer()->isTrackingSetupActive() );
    }
}



uiODWellSeisViewer2D::uiODWellSeisViewer2D( uiODMain& appl, int visid )
    : uiODViewer2D(appl,visid)
{
}


bool uiODWellSeisViewer2D::createViewWin( DataPack::ID id, bool wva )
{    
    uiWellToSeisMGR mgr( &appl_, id, false  );
    uiWellToSeisMainWin* win = mgr.win();
    if ( !win ) 
    { delete win; return false; }
    viewwin_ = win;
    viewstdcontrol_ = (uiFlatViewStdControl*)win->controlView(); 
    win->windowClosed.notify( mCB(this,uiODWellSeisViewer2D,winCloseCB) );
    viewwin()->start();
    createViewWinEditors();
    return true;
}

