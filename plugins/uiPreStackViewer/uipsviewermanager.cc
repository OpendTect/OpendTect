/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		5-11-2007
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uipsviewermanager.h"

#include "bufstringset.h"
#include "ioman.h"
#include "ioobj.h"
#include "prestackgather.h"
#include "prestackprocessor.h"
#include "settings.h"
#include "survinfo.h"
#include "uidlggroup.h"
#include "uiflatviewer.h"
#include "uiflatviewstdcontrol.h"
#include "uiflatviewmainwin.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodmain.h"
#include "uiodscenemgr.h"
#include "uipsviewershapetab.h"
#include "uipsviewerposdlg.h"
#include "uipsviewer2dposdlg.h"
#include "uipsviewersettingdlg.h"
#include "uipsviewer2dmainwin.h"
#include "uiseispartserv.h"
#include "ui3dviewer.h"
#include "uivispartserv.h"
#include "visflatviewer.h"
#include "visplanedatadisplay.h"
#include "visprestackdisplay.h"
#include "visseis2ddisplay.h"
#include "vistransform.h"
#include "uiamplspectrum.h"
#include "uiobjdisposer.h"


namespace PreStackView
{

uiViewer3DMgr::uiViewer3DMgr()
    : selectpsdatamenuitem_( "D&isplay pre-stack data" )
    , positionmenuitem_( "&Show position window ..." )  
    , proptymenuitem_( "&Properties ..." )				 
    , resolutionmenuitem_( "&Resolution ..." )				 
    , viewermenuitem_( "View in &2D panel" )
    , amplspectrumitem_( "&Amplitude spectrum ..." )
    , removemenuitem_( "&Remove" ) 
    , visserv_( ODMainWin()->applMgr().visServer() )
    , preprocmgr_( new PreStack::ProcessManager )
{
    posdialogs_.allowNull();
    visserv_->removeAllNotifier().notify( mCB(this,uiViewer3DMgr,removeAllCB) );
    visserv_->objectaddedremoved.notify( mCB(this,uiViewer3DMgr,sceneChangeCB));
    RefMan<MenuHandler> menuhandler = visserv_->getMenuHandler();

    IOM().surveyToBeChanged.notify(mCB(this,uiViewer3DMgr,surveyToBeChangedCB));
    ODMainWin()->sessionSave.notify( mCB(this,uiViewer3DMgr,sessionSaveCB) );
    ODMainWin()->sessionRestore.notify(
	    mCB(this,uiViewer3DMgr,sessionRestoreCB) );
       
    menuhandler->createnotifier.notify( mCB(this,uiViewer3DMgr,createMenuCB) );
    menuhandler->handlenotifier.notify( mCB(this,uiViewer3DMgr,handleMenuCB) );
}


uiViewer3DMgr::~uiViewer3DMgr()
{
    visserv_->removeAllNotifier().remove( mCB(this,uiViewer3DMgr,removeAllCB) );
    visserv_->objectaddedremoved.remove( mCB(this,uiViewer3DMgr,sceneChangeCB));
    RefMan<MenuHandler> menuhandler = visserv_->getMenuHandler(); 

    IOM().surveyToBeChanged.remove(mCB(this,uiViewer3DMgr,surveyToBeChangedCB));
    ODMainWin()->sessionSave.remove( mCB(this,uiViewer3DMgr,sessionSaveCB) );
    ODMainWin()->sessionRestore.remove(
	    mCB(this,uiViewer3DMgr,sessionRestoreCB) );
    menuhandler->createnotifier.remove( mCB(this,uiViewer3DMgr,createMenuCB) );
    menuhandler->handlenotifier.remove( mCB(this,uiViewer3DMgr,handleMenuCB) );

    delete visserv_;
    removeAllCB( 0 );
    delete preprocmgr_;
}    


void uiViewer3DMgr::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet( uiMenuHandler*, menu, cb );
    
    RefMan<visBase::DataObject> dataobj = visserv_->getObject( menu->menuID() );

    mDynamicCastGet( visSurvey::PlaneDataDisplay*, pdd, dataobj.ptr() );
    mDynamicCastGet( visSurvey::Seis2DDisplay*, s2d, dataobj.ptr() );
    if ( (pdd && pdd->getOrientation()!=visSurvey::PlaneDataDisplay::Zslice) 
	   || s2d ) 
    {
	uiSeisPartServer* seisserv = ODMainWin()->applMgr().seisServer();

	BufferStringSet gnms; seisserv->getStoredGathersList(!s2d,gnms);
	if ( gnms.size() )
	{
	    selectpsdatamenuitem_.removeItems();
	    selectpsdatamenuitem_.createItems(gnms);
	    
	    mAddMenuItem( menu, &selectpsdatamenuitem_, true, false );
	}
	else
	    mResetMenuItem( &selectpsdatamenuitem_ );
    }
    else
	mResetMenuItem( &selectpsdatamenuitem_ );


    mDynamicCastGet( visSurvey::PreStackDisplay*, psv, dataobj.ptr() );
    resolutionmenuitem_.id = -1;
    resolutionmenuitem_.removeItems();

    if ( psv && psv->flatViewer() )
    {
 	const int nrres = psv->flatViewer()->nrResolutions();
 	BufferStringSet resolutions;
 	for ( int idx=0; idx<nrres; idx++ )
  	    resolutions.add( psv->flatViewer()->getResolutionName(idx) );
       
    	resolutionmenuitem_.createItems( resolutions );
 	for ( int idx=0; idx<resolutionmenuitem_.nrItems(); idx++ )
  	    resolutionmenuitem_.getItem(idx)->checkable = true;
 
	resolutionmenuitem_.getItem(
	  	psv->flatViewer()->getResolution() )->checked = true;
    }
    viewermenuitem_.removeItems();

    const int idxof = psv ? viewers3d_.indexOf(psv) : -1;
    if ( idxof >=0 )
    {
	BufferStringSet vwrtypes;
	vwrtypes.add( "Single &gather" );
	vwrtypes.add( "Multiple &gathers" );
	viewermenuitem_.createItems( vwrtypes );
    }
    if ( idxof < 0  )
    {
	mResetMenuItem( &proptymenuitem_ );
	mResetMenuItem( &resolutionmenuitem_ );
	mResetMenuItem( &viewermenuitem_ );
	mResetMenuItem( &amplspectrumitem_ );
	mResetMenuItem( &positionmenuitem_ );
	mResetMenuItem( &removemenuitem_ );
    }
    else
    {
	mAddMenuItem( menu, &proptymenuitem_, true, false );
	mAddMenuItem( menu, &resolutionmenuitem_, true, false )
    	mAddMenuItem( menu, &viewermenuitem_, true, false ); 
    	mAddMenuItem( menu, &amplspectrumitem_, true, false ); 
	if ( !posdialogs_[idxof] || posdialogs_[idxof]->isHidden() )
	    mAddMenuItem( menu, &positionmenuitem_, true, false )
	else
	    mResetMenuItem( &positionmenuitem_ )

    	mAddMenuItem( menu, &removemenuitem_, true, false ); 
    }
}


static void setDlgPos( uiMainWin* mw, int idx )
{
    if ( !mw ) return;
    static uiMainWin::PopupArea puas[] =
	{ uiMainWin::BottomRight, uiMainWin::BottomLeft,
	     uiMainWin::TopRight, uiMainWin::TopLeft };
    mw->setPopupArea( puas[ idx % 4 ] );
}


void uiViewer3DMgr::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(uiMenuHandler*,menu,caller);

    if ( menu->isHandled() )
	return;

    int sceneid = getSceneID( menu->menuID() );
    if ( sceneid==-1 )
	return;

    const int mnuidx = selectpsdatamenuitem_.itemIndex( mnuid );

    RefMan<visBase::DataObject> dataobj = visserv_->getObject( menu->menuID() );
    mDynamicCastGet(visSurvey::PreStackDisplay*,psv,dataobj.ptr())
    if ( mnuidx < 0 && !psv )
	return;

    if ( mnuidx>=0 )
    {
	menu->setIsHandled( true );
	if ( !add3DViewer( menu, sceneid, mnuidx ) )
	    return;
    }
    else if ( mnuid==removemenuitem_.id )
    {
	menu->setIsHandled( true );
	visserv_->removeObject( psv, sceneid );
	const int idx = viewers3d_.indexOf( psv );
	delete posdialogs_.removeSingle( idx );
	viewers3d_.removeSingle( idx )->unRef();
    }
    else if ( mnuid==proptymenuitem_.id )
    {
	menu->setIsHandled( true );
	uiViewer3DSettingDlg* dlg = new uiViewer3DSettingDlg(
		menu->getParent(), *psv, *this, *preprocmgr_);
	dlg->setDeleteOnClose( true );
	dlg->go();
    }
    else if ( mnuid==positionmenuitem_.id )
    {
	menu->setIsHandled( true );
	const int idx = viewers3d_.indexOf( psv );
	if ( idx >= 0 )
	{
	    uiViewer3DPositionDlg* dlg = posdialogs_[idx];
	    if ( !dlg )
		dlg = mkNewPosDialog( menu, *psv );
	    if ( dlg )
		dlg->show();
	}
    }
    else if ( resolutionmenuitem_.id!=-1 && 
	    resolutionmenuitem_.itemIndex(mnuid)!=-1 )
    {
 	menu->setIsHandled( true );
  	if ( psv->flatViewer() )
   	    psv->flatViewer()->setResolution( 
		    resolutionmenuitem_.itemIndex(mnuid) );
    }
    else if ( viewermenuitem_.itemIndex(mnuid)==1 )
    {
	menu->setIsHandled( true );
	multiviewers2d_ += createMultiGather2DViewer( *psv );
    }
    else if ( viewermenuitem_.itemIndex(mnuid)==0 )
    {
	menu->setIsHandled( true );
	PtrMan<IOObj> ioobj = IOM().get( psv->getMultiID() );
	if ( !ioobj )
	    return;

	BufferString title;
	if ( psv->is3DSeis() )
	    getSeis3DTitle( psv->getBinID(), ioobj->name(), title );
	else
	    getSeis2DTitle( psv->traceNr(), psv->lineName(), title );	

	uiFlatViewMainWin* viewwin = create2DViewer(title,psv->getDataPackID());
	if ( viewwin )
	{
	    viewers2d_ += viewwin;
	    viewwin->start();
	}
    }
    else if ( mnuid==amplspectrumitem_.id )
    {
	menu->setIsHandled( true );
	uiAmplSpectrum* asd = new uiAmplSpectrum( menu->getParent() );
	asd->setDeleteOnClose( true );
	asd->setDataPackID( psv->getDataPackID(), DataPackMgr::FlatID() );
	BufferString capt( "Amplitude spectrum for " );
	capt += psv->getObjectName();
	capt += " at ";
	if ( psv->is3DSeis() )
	    { capt += psv->getPosition().inl; capt += "/"; }
	capt += psv->getPosition().crl;
	asd->setCaption( capt );
	asd->show();
    }
}


int uiViewer3DMgr::getSceneID( int mnid )
{
    int sceneid = -1;
    TypeSet<int> sceneids;
    visserv_->getChildIds( -1, sceneids );
    for ( int idx=0; idx<sceneids.size(); idx++ )
    {
	TypeSet<int> scenechildren;
	visserv_->getChildIds( sceneids[idx], scenechildren );
	if ( scenechildren.indexOf(mnid)>=0 )
	{
	    sceneid = sceneids[idx];
	    break;
	}
    }
    
    return sceneid;
}


#define mErrReturn(msg) { uiMSG().error(msg); return false; }

bool uiViewer3DMgr::add3DViewer( const uiMenuHandler* menu, 
				 int sceneid, int mnuidx )
{
    if ( !menu )
	return false;

    PtrMan<IOObj> ioobj = IOM().getLocal(
	    selectpsdatamenuitem_.getItem(mnuidx)->text );
    if ( !ioobj )
	mErrReturn( "No object selected" )

    RefMan<visBase::DataObject> dataobj =
	visserv_->getObject( menu->menuID() );
		
    mDynamicCastGet( visSurvey::PlaneDataDisplay*, pdd, dataobj.ptr() );
    mDynamicCastGet( visSurvey::Seis2DDisplay*, s2d, dataobj.ptr() );
    if ( !pdd && !s2d )
	mErrReturn( "Display panel is not set." )

    visSurvey::PreStackDisplay* viewer = visSurvey::PreStackDisplay::create();
    viewer->ref();
    viewer->setMultiID( ioobj->key() );
    visserv_->addObject( viewer, sceneid, false );
   
    const Coord3 pickedpos = menu->getPickedPos();
    
    //set viewer position
    bool settingok = true;
    if ( pdd )
    {
	viewer->setSectionDisplay( pdd ); 
	BinID bid;
	if (  menu->getMenuType() != uiMenuHandler::fromScene() ) 
	{
	    HorSampling hrg = pdd->getCubeSampling().hrg;
	    bid = SI().transform((SI().transform(hrg.start)
				 +SI().transform(hrg.stop))/2);
	}
	else bid = SI().transform( pickedpos );

	settingok = viewer->setPosition( bid );
    } 
    else if ( s2d )
    {
	int trcnr;
	if ( menu->getMenuType() != uiMenuHandler::fromScene() )
	    trcnr = s2d->getTraceNrRange().center();
	else
	    trcnr = s2d->getNearestTraceNr( pickedpos );
	
	settingok = viewer->setSeis2DDisplay( s2d, trcnr );
    }
    
    if ( !settingok )
    {
	visserv_->removeObject( viewer, sceneid );
	viewer->unRef();
	return false;
    }

    const int res = pdd ? pdd->getResolution() : s2d->getResolution();
    if ( viewer->flatViewer() )
   	viewer->flatViewer()->setResolution( res );

    //set viewer angle.
    const ui3DViewer*  sovwr = ODMainWin()->sceneMgr().getSoViewer( sceneid );
    const Coord3 campos = sovwr->getCameraPosition();
    const Coord3 displaycampos = 
	viewer->getScene()->getUTM2DisplayTransform()->transformBack( campos );
    const BinID dir0 = SI().transform(displaycampos)-SI().transform(pickedpos);
    const Coord dir( dir0.inl, dir0.crl );
    viewer->displaysOnPositiveSide( viewer->getBaseDirection().dot(dir)>0 );
    
    //Read defaults 
    const Settings& settings = Settings::fetch(uiViewer3DMgr::sSettings3DKey()); 
    bool autoview;
    if ( settings.getYN(visSurvey::PreStackDisplay::sKeyAutoWidth(), autoview) )
	viewer->displaysAutoWidth( autoview );

    float factor;
    if ( settings.get( visSurvey::PreStackDisplay::sKeyFactor(), factor ) )
	viewer->setFactor( factor );
   
    float width; 
    if ( settings.get( visSurvey::PreStackDisplay::sKeyWidth(), width ) )
	viewer->setWidth( width );
    
    IOPar* flatviewpar = settings.subselect( sKeyFlatviewPars() );
    bool showx1 = viewer->flatViewer()->appearance().annot_.x1_.showgridlines_;
    bool showx2 = viewer->flatViewer()->appearance().annot_.x2_.showgridlines_;
    if ( flatviewpar )
    {
	viewer->flatViewer()->appearance().ddpars_.usePar( *flatviewpar );
	flatviewpar->getYN( IOPar::compKey( FlatView::Annotation::sKeyAxes(),
		    FlatView::Annotation::sKeyShwGridLines()), showx1, showx2 );
	viewer->flatViewer()->appearance().annot_.x1_.showgridlines_ = showx1;
 	viewer->flatViewer()->appearance().annot_.x2_.showgridlines_ = showx2;
    }
    
    viewer->flatViewer()->handleChange( FlatView::Viewer::DisplayPars );
    viewer->flatViewer()->turnOnGridLines( showx1, showx2 );

    if ( viewer->getScene() )
	viewer->getScene()->change.notifyIfNotNotified( 
		mCB( this, uiViewer3DMgr, sceneChangeCB ) );

    viewers3d_ += viewer;
    posdialogs_ += 0;
    mkNewPosDialog( menu, *viewer );
    return true;
}


uiViewer3DPositionDlg*
    uiViewer3DMgr::mkNewPosDialog( const uiMenuHandler* menu,
				   visSurvey::PreStackDisplay& vwr )
{
    mDeclareAndTryAlloc( uiViewer3DPositionDlg*, dlg,
	    uiViewer3DPositionDlg( menu->getParent(), vwr ) );
    if ( dlg ) 
    {
	const int newidx = posdialogs_.size() - 1;
	setDlgPos( dlg, newidx );
	posdialogs_.replace( newidx, dlg );
	dlg->show();
    }

    return dlg;
}


#define mErrRes(msg) { uiMSG().error(msg); return 0; }

uiFlatViewMainWin* uiViewer3DMgr::create2DViewer( const BufferString& title, 
						int dpid )
{
    uiFlatViewMainWin* viewwin = new uiFlatViewMainWin( 
	ODMainWin(), uiFlatViewMainWin::Setup(title) );

    viewwin->setWinTitle( title );
    viewwin->setDarkBG( false );

    uiFlatViewer& vwr = viewwin->viewer();
    vwr.appearance().annot_.setAxesAnnot( true );
    vwr.appearance().setGeoDefaults( true );
    vwr.appearance().ddpars_.show( false, true );
    vwr.appearance().ddpars_.wva_.overlap_ = 1;

    DataPack* dp = DPM(DataPackMgr::FlatID()).obtain( dpid );
    if ( !dp )
	return 0;

    mDynamicCastGet( const FlatDataPack*, fdp, dp );
    if ( !fdp )
    {
	DPM(DataPackMgr::FlatID()).release( dp );
	return 0;
    }

    vwr.setPack( false, dpid, false, true );
    int pw = 400 + 5 * fdp->data().info().getSize( 0 );
    if ( pw > 800 ) pw = 800;

    vwr.setInitialSize( uiSize(pw,600) );  
    viewwin->addControl( new uiFlatViewStdControl( vwr,
	uiFlatViewStdControl::Setup().withstates(true) ) );
    viewwin->windowClosed.notify( mCB(this,uiViewer3DMgr,viewer2DClosedCB) );
    //vwr.drawBitMaps();
    //vwr.drawAnnot();
    DPM(DataPackMgr::FlatID()).release( dp );
    return viewwin;
}


uiStoredViewer2DMainWin* uiViewer3DMgr::createMultiGather2DViewer( 
				    const visSurvey::PreStackDisplay& psv )
{
    const MultiID mid = psv.getMultiID();
    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj )
       return 0;

    const bool is2d = !psv.is3DSeis();
    BufferString title = "Gathers from [";
    if ( is2d )
	title += psv.lineName();
    else 
	title += ioobj->name();
    title += "]";

    uiStoredViewer2DMainWin* viewwin =
	new uiStoredViewer2DMainWin( ODMainWin(), title ); 
    viewwin->show();
    const StepInterval<int>& trcrg = psv.getTraceRange( psv.getBinID() );
    viewwin->init( mid, psv.getDataPackID(), psv.isOrientationInline(), trcrg,
			    is2d ? psv.lineName() : 0 );
    viewwin->setDarkBG( false );
    viewwin->seldatacalled_.notify( mCB(this,uiViewer3DMgr,viewer2DSelDataCB) );
    viewwin->windowClosed.notify( mCB(this,uiViewer3DMgr,viewer2DClosedCB) );
    return viewwin;
}


void uiViewer3DMgr::viewer2DSelDataCB( CallBacker* cb )
{
    mDynamicCastGet( uiStoredViewer2DMainWin*, win, cb )
    if ( !win )
	{ pErrMsg( "Can not find viewer" ); return; }

    uiSeisPartServer* seisserv = ODMainWin()->applMgr().seisServer();
    BufferStringSet selgnms, allgnms; TypeSet<MultiID> selids;	
    win->getIDs( selids );
    for( int idx=0; idx<selids.size(); idx++ )
    {
	PtrMan<IOObj> ioobj = IOM().get( selids[idx] );
	if ( ioobj )
	    selgnms.addIfNew( ioobj->name() );
    }

    seisserv->getStoredGathersList(!win->is2D(),allgnms);
    if ( allgnms.isEmpty() )
	return;

    for ( int idx=0; idx<selgnms.size(); idx++ )
    {
	if ( allgnms.isPresent( selgnms.get( idx ).buf() ) ) 
	    allgnms.removeSingle(allgnms.indexOf( selgnms.get( idx ).buf() ) );
    }

    selids.erase(); 
    uiViewer2DSelDataDlg dlg( win, allgnms, selgnms );
    if ( dlg.go() )
    {
	for( int idx=0; idx<selgnms.size(); idx++ )
	{
	    IOObj* ioobj = IOM().getLocal( selgnms[idx]->buf() );
	    if ( ioobj )
		selids += ioobj->key();
	    else
	    { 
		BufferString msg( "Can not find" );
		msg += selgnms[idx]->buf();
		uiMSG().error( msg ); 
	    }
	}
	if ( selids.isEmpty() )
	    { uiMSG().error("No data found"); return; }

	win->setIDs( selids );
    }
}


void uiViewer3DMgr::viewer2DClosedCB( CallBacker* cb )
{
    const int idx = viewers2d_.indexOf( (uiFlatViewMainWin*) cb );
    if ( idx==-1 )
	return;

    viewers2d_[idx]->windowClosed.remove(
	    mCB(this,uiViewer3DMgr,viewer2DClosedCB) );

    viewers2d_.removeSingle( idx );
}


void uiViewer3DMgr::sceneChangeCB( CallBacker* )
{
    for ( int idx = 0; idx<viewers3d_.size(); idx++ )
    {
	visSurvey::PreStackDisplay* psv = viewers3d_[idx];
	visBase::Scene* scene = psv->getScene();	

	int dpid = psv->getDataPackID();
	const visSurvey::PlaneDataDisplay* pdd = psv->getSectionDisplay();
	const visSurvey::Seis2DDisplay*    s2d = psv->getSeis2DDisplay();
	if ( pdd && (!scene || scene->getFirstIdx( pdd )==-1 ) )
	{
	    removeViewWin( dpid );
	    viewers3d_.removeSingle( idx );
	    delete posdialogs_.removeSingle( idx );
	    if ( scene ) visserv_->removeObject( psv, scene->id() );
	    psv->unRef();
	    idx--;
	}
	
	if ( s2d && (!scene || scene->getFirstIdx( s2d )==-1 ) )
	{
	    removeViewWin( dpid );
	    viewers3d_.removeSingle( idx );
	    delete posdialogs_.removeSingle( idx );
	    if ( scene ) visserv_->removeObject( psv, scene->id() );
	    psv->unRef();
	    idx--;
	}
    }
}


void uiViewer3DMgr::removeViewWin( int dpid )
{
    for ( int idx=0; idx<viewers2d_.size(); idx++ )
    {
	if ( viewers2d_[idx]->viewer().packID(false) == dpid )
    	    delete viewers2d_.removeSingle( idx );
    }
}


void uiViewer3DMgr::sessionRestoreCB( CallBacker* )
{
    deepErase( viewers2d_ );

    TypeSet<int> vispsviewids;
    visserv_->findObject( typeid(visSurvey::PreStackDisplay), vispsviewids );

    for ( int idx=0; idx<vispsviewids.size(); idx++ )
    {
	mDynamicCastGet( visSurvey::PreStackDisplay*, psv,
			 visserv_->getObject(vispsviewids[idx]) );
	if ( !psv )
	    continue;

	if ( psv->getScene() )
	    psv->getScene()->change.notifyIfNotNotified( 
		    mCB( this, uiViewer3DMgr, sceneChangeCB ) );
	viewers3d_ += psv;
	posdialogs_ += 0;
	psv->ref();
    }
    
    PtrMan<IOPar> allwindowspar = ODMainWin()->sessionPars().subselect(
	    			  sKey2DViewers() );
    if ( !allwindowspar )
	allwindowspar =
	    ODMainWin()->sessionPars().subselect( "PreStack 2D Viewers" );

    int nrwindows;
    if ( !allwindowspar || !allwindowspar->get(sKeyNrWindows(), nrwindows) )
	return;

    for ( int idx=0; idx<nrwindows; idx++ )
    {
	BufferString key = sKeyViewerPrefix();
	key += idx;
	PtrMan<IOPar> viewerpar = allwindowspar->subselect( key.buf() );
	if ( !viewerpar )
	    continue;

	MultiID mid;
	bool is3d;
	int trcnr;
	BinID bid;
	BufferString name2d;
	if ( !viewerpar->get( sKeyMultiID(), mid ) ||
	     !viewerpar->get( sKeyBinID(), bid ) ||
	     !viewerpar->get( sKeyTraceNr(), trcnr ) ||
	     !viewerpar->get( sKeyLineName(), name2d ) ||
	     !viewerpar->getYN( sKeyIsVolumeData(), is3d ) )
	{
	    if ( !viewerpar->get( "uiFlatViewWin MultiID", mid ) ||
		 !viewerpar->get( "uiFlatViewWin binid", bid ) ||
		 !viewerpar->get( "Seis2D TraceNr", trcnr ) ||
		 !viewerpar->get( "Seis2D Name", name2d ) ||
		 !viewerpar->getYN( "Seis3D display", is3d ) )
		continue;
	}

	PtrMan<IOObj> ioobj = IOM().get( mid );
	if ( !ioobj )
	    continue;

	PreStack::Gather* gather = new PreStack::Gather;
	int dpid;
	if ( is3d && gather->readFrom(mid,bid,0) )
	    dpid = gather->id();
	else if ( gather->readFrom( *ioobj, trcnr, name2d,0 ) )
	    dpid = gather->id();
	else 
	{
	    delete gather;
	    continue;	    
	}

	DPM(DataPackMgr::FlatID()).add( gather );
	DPM(DataPackMgr::FlatID()).obtain( dpid );

	BufferString title;
	if ( is3d )
	    getSeis3DTitle( bid, ioobj->name(), title );
	else
	    getSeis2DTitle( trcnr, name2d, title );
	uiFlatViewMainWin* viewwin = create2DViewer( title, dpid );
	DPM(DataPackMgr::FlatID()).release( gather );
	if ( !viewwin )
	    continue;

	viewers2d_ += viewwin;
	viewwin->start();
    }
    
    if ( preprocmgr_ )
	preprocmgr_->usePar( *allwindowspar );

    for ( int idx=0; idx<viewers3d_.size(); idx++ )
	viewers3d_[idx]->setPreProcessor( preprocmgr_ );
}


void uiViewer3DMgr::getSeis2DTitle( int tracenr, const char* nm,
				    BufferString& title )
{
    title = "Gather from [";
    title += nm;
    title += "] at trace " ;
    title += tracenr;
}


void uiViewer3DMgr::getSeis3DTitle( const BinID& bid, const char* name,
				    BufferString& title )
{
    title = "Gather from [";
    title += name;
    title += "] at ";
    title += bid.inl;
    title += "/";
    title += bid.crl;
}


void uiViewer3DMgr::sessionSaveCB( CallBacker* ) 
{
    IOPar allwindowpar;
    int nrsaved = 0;
    for ( int idx=0; idx<viewers2d_.size(); idx++ )
    {
	const FlatDataPack* dp = viewers2d_[idx]->viewer().pack( false );
	mDynamicCastGet( const PreStack::Gather*, gather, dp );
	if ( !gather )
	    continue;

	IOPar viewerpar;
	viewers2d_[idx]->viewer().fillAppearancePar( viewerpar );
	viewerpar.set( sKeyBinID(), gather->getBinID() );
	viewerpar.set( sKeyMultiID(), gather->getStorageID() );
	viewerpar.set( sKeyTraceNr(), gather->getSeis2DTraceNr() );
	viewerpar.set( sKeyLineName(), gather->getSeis2DName() );
	viewerpar.setYN( sKeyIsVolumeData(), gather->is3D() );

	BufferString key = sKeyViewerPrefix();
	key += nrsaved;
	nrsaved++;

	allwindowpar.mergeComp( viewerpar, key );
    }

    if ( preprocmgr_ )
	preprocmgr_->fillPar( allwindowpar );

    allwindowpar.set( sKeyNrWindows(), nrsaved );
    ODMainWin()->sessionPars().mergeComp( allwindowpar, sKey2DViewers() );
}


void  uiViewer3DMgr::removeAllCB( CallBacker* )
{
    deepErase( posdialogs_ );
    deepErase( viewers2d_ );
    deepUnRef( viewers3d_ );
}    


void uiViewer3DMgr::surveyToBeChangedCB( CallBacker* )
{
    deepErase( posdialogs_ );
    deepErase( viewers2d_ );
    deepUnRef( viewers3d_ );
}

}; // Namespace
