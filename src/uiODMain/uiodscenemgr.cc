/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "od_helpids.h"
#include "uiodscenemgr.h"
#include "scene.xpm"

#include "ui3dviewer.h"
#include "uiaction.h"
#include "uiattribpartserv.h"
#include "uibutton.h"
#include "uibuttongroup.h"
#include "uidockwin.h"
#include "uiempartserv.h"
#include "uifont.h"
#include "uigeninput.h"
#include "uigeninputdlg.h"
#include "uimain.h"
#include "uimdiarea.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodmenumgr.h"
#include "uiodviewer2dmgr.h"
#include "uiosgutil.h"
#include "uipickpartserv.h"
#include "uiprintscenedlg.h"
#include "uiscenepropdlg.h"
#include "uisettings.h"
#include "uistatusbar.h"
#include "uistrings.h"
#include "uitreeitemmanager.h"
#include "uitreeview.h"
#include "uiviscoltabed.h"
#include "uivispartserv.h"
#include "uiwindowgrabber.h"

#include "emfaultstickset.h"
#include "emfault3d.h"
#include "emfaultset3d.h"
#include "emhorizon2d.h"
#include "emmanager.h"
#include "emhorizon3d.h"
#include "emmarchingcubessurface.h"
#include "emrandomposbody.h"
#include "envvars.h"
#include "ioman.h"
#include "ioobj.h"
#include "uiosgutil.h"
#include "pickset.h"
#include "ptrman.h"
#include "sorting.h"
#include "settingsaccess.h"
#include "vissurvscene.h"
#include "vissurvobj.h"
#include "welltransl.h"

// For factories
#include "uiodannottreeitem.h"
#include "uiodbodydisplaytreeitem.h"
#include "uiodemsurftreeitem.h"
#include "uiodfaultsettreeitem.h"
#include "uiodfaulttreeitem.h"
#include "uiodhortreeitem.h"
#include "uiodpicksettreeitem.h"
#include "uiodplanedatatreeitem.h"
#include "uiodpseventstreeitem.h"
#include "uiodrandlinetreeitem.h"
#include "uiodseis2dtreeitem.h"
#include "uiodscenetreeitem.h"
#include "uiodvolrentreeitem.h"
#include "uiodwelltreeitem.h"

#define mPosField	0
#define mValueField	1
#define mNameField	2
#define mStatusField	3

static const int cWSWidth = 600;
static const int cWSHeight = 500;
static const char* sKeyWarnStereo = "Warning.Stereo Viewing";

#define mWSMCB(fn) mCB(this,uiODSceneMgr,fn)
#define mDoAllScenes(memb,fn,arg) \
    for ( int idx=0; idx<scenes_.size(); idx++ ) \
	scenes_[idx]->memb->fn( arg )

uiODSceneMgr::uiODSceneMgr( uiODMain* a )
    : sceneClosed(this)
    , treeToBeAdded(this)
    , treeAdded(this)
    , scenesHidden(this)
    , scenesShown(this)
    , viewModeChanged(this)
    , activeSceneChanged(this)
    , appl_(*a)
    , mdiarea_(new uiMdiArea(a,"OpendTect work space"))
    , vwridx_(0)
    , tifs_(new uiTreeFactorySet)
    , wingrabber_(new uiWindowGrabber(a))
    , tiletimer_(new Timer)
{
    tifs_->addFactory( new uiODInlineTreeItemFactory, 1000,
		       OD::Only3D );
    tifs_->addFactory( new uiODCrosslineTreeItemFactory, 1100,
		       OD::Only3D );
    tifs_->addFactory( new uiODZsliceTreeItemFactory, 1200,
		       OD::Only3D );
    tifs_->addFactory( new uiODVolrenTreeItemFactory, 1500, OD::Only3D );
    tifs_->addFactory( new uiODRandomLineTreeItemFactory, 2000,
		       OD::Only3D );
    tifs_->addFactory( new Line2DTreeItemFactory, 3000, OD::Only2D );
    tifs_->addFactory( new uiODHorizonTreeItemFactory, 4000,
		       OD::Both2DAnd3D );
    tifs_->addFactory( new uiODHorizon2DTreeItemFactory, 4500,
		       OD::Only2D );
    tifs_->addFactory( new uiODFaultTreeItemFactory, 5000 );
    tifs_->addFactory( new uiODFaultSetTreeItemFactory, 5250,
		       OD::Only3D );
    tifs_->addFactory( new uiODFaultStickSetTreeItemFactory, 5500,
		       OD::Both2DAnd3D );
    tifs_->addFactory( new uiODBodyDisplayTreeItemFactory, 6000,
		       OD::Only3D );
    tifs_->addFactory( new uiODWellTreeItemFactory, 7000,
		       OD::Both2DAnd3D );
    tifs_->addFactory( new uiODPickSetTreeItemFactory, 8000,
		       OD::Both2DAnd3D );
    tifs_->addFactory( new uiODPolygonTreeItemFactory, 8500,
		       OD::Both2DAnd3D );
    tifs_->addFactory( new uiODPSEventsTreeItemFactory, 9000,
		       OD::Both2DAnd3D );
    tifs_->addFactory( new uiODAnnotTreeItemFactory, 10000,
		       OD::Both2DAnd3D );

    mdiarea_->setPrefWidth( cWSWidth );
    mdiarea_->setPrefHeight( cWSHeight );

    mAttachCB( mdiarea_->windowActivated, uiODSceneMgr::mdiAreaChanged );
    mAttachCB( tiletimer_->tick, uiODSceneMgr::tileTimerCB );

    uiFont& font3d = FontList().get( FontData::key(FontData::Graphics3D) );
    mAttachCB( font3d.changed, uiODSceneMgr::font3DChanged );

    mAttachCB( appl_.windowShown, uiODSceneMgr::showIfMinimized );
    mAttachCB( appl_.windowShown, uiODSceneMgr::mdiAreaChanged );
    if ( !OD::useQOpenGL() )
	OD::setOSGTimerCallbacks( scenesShown, scenesHidden );
}


uiODSceneMgr::~uiODSceneMgr()
{
    detachAllNotifiers();
    cleanUp( false );
    delete tifs_;
    delete mdiarea_;
    for ( int idx=scenes_.size()-1; idx>=0; idx-- )
    {
	SceneBundle* scene = scenes_.get( idx );
	scene->itemmanager_->prepareForShutdown();
	removeScene( *scene ); // deletes "scene"
    }

    deepErase( scenes_ );
    delete wingrabber_;
    delete tiletimer_;
}


bool uiODSceneMgr::canAddSceneAtStartup()
{
    static bool addscene = !GetEnvVarYN( "OD_NOSCENE_AT_STARTUP" );
    return addscene;
}


void uiODSceneMgr::initMenuMgrDepObjs()
{
    if ( visServ().nrScenes() == 0 )
	addDefaultScene();
}


void uiODSceneMgr::cleanUp( bool startnew )
{
    mdiarea_->closeAll();
    // closeAll() cascades callbacks which remove the scene from set

    visServ().deleteAllObjects();
    vwridx_ = 0;
    if ( startnew )
	addDefaultScene();
}


uiODSceneMgr::SceneBundle& uiODSceneMgr::mkNewScene( bool vsg )
{
    SceneBundle* scn = vsg ? new uiODSceneMgr::VSGSceneBundle( mdiarea_ )
			   : new uiODSceneMgr::OSGSceneBundle( mdiarea_ );
    if ( vsg )
	scn->vwr3d_ = visServ().createVSGViewer( scn->mdiwin_ );

    mAttachCB( scn->mdiwin_->closed(), uiODSceneMgr::removeSceneCB );
    mAttachCB( scn->mdiwin_->windowShown(), uiODSceneMgr::mdiAreaChanged );
    mAttachCB( scn->mdiwin_->windowHidden(), uiODSceneMgr::mdiAreaChanged );
    scenes_ += scn;
    vwridx_++;
    if ( scn->vwr3d_ )
    {
	BufferString vwrnm( "Viewer Scene ", vwridx_ );
	scn->vwr3d_->setName( vwrnm );
    }

    return *scn;
}


SceneID uiODSceneMgr::addScene( bool maximized, ZAxisTransform* zt,
				 const uiString& name, bool isvsg )
{
    if ( !canAddSceneAtStartup() )
	return SceneID::udf();

    SceneBundle& scn = mkNewScene( isvsg );
    const SceneID sceneid = scn.vwr3d_ ? visServ().addScene( isvsg )
				       : SceneID::udf();
    if ( !sceneid.isValid() )
    {
	removeScene( scn );
	return SceneID::udf();
    }

    RefMan<visSurvey::Scene> visscene = visServ().getScene( sceneid );
    scn.vwr3d_->setScene( visscene.ptr() );

    uiString title = uiStrings::phrJoinStrings( uiStrings::sScene(),
					       toUiString(vwridx_) );
    scn.mdiwin_->setTitle( title );
    visServ().setSceneName( sceneid, title );
    scn.vwr3d_->display( true );
    scn.vwr3d_->setAnnotationFont( visscene ? visscene->getAnnotFont()
					    : FontData() );
    scn.mdiwin_->display( true, false, maximized );
    actMode(0);

    // Tree listeners add objects that read these
    auto* osgvwr = dCast(ui3DViewer*,scn.vwr3d_);
    if ( visscene && osgvwr )
    {
	if ( osgvwr->getPolygonSelector() )
	    visscene->setPolygonSelector( osgvwr->getPolygonSelector() );
	if ( osgvwr->getSceneColTab() )
	    visscene->setSceneColTab( osgvwr->getSceneColTab() );
	mAttachCB( visscene->sceneboundingboxupdated,
		   uiODSceneMgr::newSceneUpdated );
    }

    if ( !isvsg )
	treeToBeAdded.trigger( sceneid );
    initTree( scn, vwridx_ );
    if ( !isvsg )
	treeAdded.trigger( sceneid );

    if ( name.isSet() )
	setSceneName( sceneid, name );

    visServ().turnSelectionModeOn( visServ().isSelectionModeOn() );

    if ( isvsg )
    {
	scn.vwr3d_->setStartupView();
	return sceneid;
    }

    mAttachCB( osgvwr->viewmodechanged, uiODSceneMgr::viewModeChg );
    mAttachCB( osgvwr->pageupdown, uiODSceneMgr::pageUpDownPressed );

    OSGSceneBundle* firstscene = nullptr;
    int nrodscenes = 0;
    for ( auto* bundle : scenes_ )
    {
	if ( bundle->isVSG() )
	    continue;

	if ( !firstscene )
	    firstscene = dCast(OSGSceneBundle*,bundle);
	nrodscenes++;
    }

    if ( nrodscenes>1 && firstscene )
    {
	auto* firstvwr = dCast(ui3DViewer*,firstscene->vwr3d_);
	osgvwr->setStereoType( firstvwr->getStereoType() );
	osgvwr->setStereoOffset( firstvwr->getStereoOffset() );
	scn.vwr3d_->showRotAxis( firstscene->vwr3d_->rotAxisShown() );
	if ( !firstvwr->isCameraPerspective() )
	    osgvwr->toggleCameraType();
	visServ().displaySceneColorbar( visServ().sceneColorbarDisplayed() );
    }
    else if ( firstscene )
    {
	auto* firstvwr = dCast(ui3DViewer*,firstscene->vwr3d_);
	const bool isperspective = firstvwr->isCameraPerspective();
	if ( appl_.menuMgrAvailable() )
	{
	    appl_.menuMgr().setCameraPixmap( isperspective );
	    appl_.menuMgr().updateAxisMode( true );
	}
	scn.vwr3d_->showRotAxis( true );
    }

    visServ().setZAxisTransform( sceneid, zt, nullptr );
    scn.vwr3d_->setStartupView();

    return sceneid;
}

SceneID uiODSceneMgr::addScene( bool maximized, bool isvsg )
{
    return addScene( maximized, nullptr, uiString::empty(), isvsg );
}

uiODSceneMgr::SceneBundle* uiODSceneMgr::getScene( const SceneID& sceneid )
{
    if ( !sceneid.isValid() )
	return nullptr;

    for ( auto* bundle : scenes_ )
    {
	if ( bundle->itemmanager_ && bundle->sceneID() == sceneid )
	    return bundle;
    }

    return nullptr;
}


void uiODSceneMgr::tabifyTree( SceneBundle& tree )
{
    for ( int idx=scenes_.indexOf(&tree)-1; idx>=0; idx-- )
    {
	uiDockWin* other = scenes_[idx]->dw_;
	if ( !other )
	    continue;

	appl_.tabifyDockWindow( *other, *tree.dw_ );
	return;
    }
}


SceneID uiODSceneMgr::addDefaultScene()
{
    if ( !canAddSceneAtStartup() )
	return SceneID::udf();

    if ( useVSGDefaultScene() )
	return addScene( true, true );

    return addScene( true );
}


void uiODSceneMgr::newSceneUpdated( CallBacker* )
{
    OSGSceneBundle* lastscene = nullptr;
    for ( int idx=scenes_.size()-1; idx>=0; idx-- )
    {
	if ( scenes_[idx]->isVSG() )
	    continue;

	lastscene = dCast(OSGSceneBundle*,scenes_[idx]);
	break;
    }

    if ( lastscene && lastscene->vwr3d_ )
    {
	lastscene->vwr3d_->viewAll( false );
	tiletimer_->start( 10,true );

	RefMan<visSurvey::Scene> visscene = visServ().getScene(
				lastscene->vwr3d_->sceneID() );
	if ( visscene )
	    mDetachCB( visscene->sceneboundingboxupdated,
		       uiODSceneMgr::newSceneUpdated );
    }
}


void uiODSceneMgr::tileTimerCB( CallBacker* )
{
    if ( scenes_.size() > 1 )
	tile();
}


void uiODSceneMgr::removeScene( uiODSceneMgr::SceneBundle& scene )
{
    if ( !scene.isVSG() )
	appl_.colTabEd().setColTab( 0, mUdf(int), mUdf(int) );

    appl_.removeDockWindow( scene.dw_ );

    if ( scene.isVSG() )
	visServ().removeScene( scene.sceneID() );
    else if ( scene.itemmanager_ )
    {
	const SceneID sceneid = scene.itemmanager_->sceneID();
	RefMan<visSurvey::Scene> visscene =
			appl_.applMgr().visServer()->getScene( sceneid );
	if ( visscene )
	{
	    mDetachCB( visscene->sceneboundingboxupdated,
		       uiODSceneMgr::newSceneUpdated );
	    scene.itemmanager_->askContinueAndSaveIfNeeded( false );
	    visscene->setMoreObjectsToDoHint( true );
	}

	visscene = nullptr;
	scene.itemmanager_->prepareForShutdown();
	visServ().removeScene( scene.itemmanager_->sceneID() );
	sceneClosed.trigger( scene.itemmanager_->sceneID() );
    }

    scene.mdiwin_->closed().remove( mWSMCB(removeSceneCB) );
    scenes_ -= &scene;
    delete &scene;
}


void uiODSceneMgr::removeSceneCB( CallBacker* cb )
{
    mDynamicCastGet(uiGroupObj*,grp,cb)
    if ( !grp ) return;
    for ( auto* bundle : scenes_ )
    {
	if ( grp != bundle->mdiwin_->mainObject() )
	    continue;

	removeScene( *bundle );
	return;
    }
}


void uiODSceneMgr::setSceneName( const SceneID& sceneid, const uiString& nm )
{
    visServ().setSceneName( sceneid, nm );
    SceneBundle* bundle = getScene( sceneid );
    if ( !bundle )
	return;

    bundle->setName( nm );
    uiTreeItem* itm = findSceneItem( sceneid );
    if ( itm )
	itm->updateColumnText( uiODSceneMgr::cNameColumn() );
}


uiString uiODSceneMgr::getSceneName( const SceneID& sceneid ) const
{
    return getNonConst(*this).visServ().getSceneName( sceneid );
}


void uiODSceneMgr::getScenePars( IOPar& iopar )
{
    int idx = 0;
    for ( auto* bundle : scenes_ )
    {
	if ( bundle->isVSG() )
	    continue;

	auto* scene = dCast(OSGSceneBundle*,bundle);
	IOPar iop;
	dCast(ui3DViewer*,scene->vwr3d_)->fillPar( iop );
	iopar.mergeComp( iop, toString(idx++) );
    }
}


void uiODSceneMgr::useScenePars( const IOPar& sessionpar )
{
    for ( int idx=0; ; idx++ )
    {
	PtrMan<IOPar> scenepar = sessionpar.subselect( toString(idx) );
	if ( !scenepar || !scenepar->size() )
	{
	    if ( !idx ) continue;
	    break;
	}

	auto* scn = dCast(OSGSceneBundle*,&mkNewScene());
	if ( !scn )
	    continue;

	auto* vwr3d = dCast(ui3DViewer*,scn->vwr3d_);
	if ( !vwr3d->usePar(*scenepar) )
	{
	    removeScene( *scn );
	    continue;
	}

	const SceneID sceneid = scn->vwr3d_->sceneID();
	RefMan<visSurvey::Scene> visscene = visServ().getScene( sceneid );
	if ( visscene )
	{
	    if ( vwr3d->getPolygonSelector() )
		visscene->setPolygonSelector( vwr3d->getPolygonSelector() );
	    if ( vwr3d->getSceneColTab() )
		visscene->setSceneColTab( vwr3d->getSceneColTab() );
	}

	visServ().displaySceneColorbar( visServ().sceneColorbarDisplayed() );
	visServ().turnSelectionModeOn( false );

	uiString scenenm = visscene->uiName();
	if ( scenenm.isEmpty() )
	{
	    scenenm = uiStrings::phrJoinStrings(
				    uiStrings::sScene(), toUiString(vwridx_) );
	    visServ().setSceneName( sceneid, scenenm );
	}

	scn->mdiwin_->setTitle( scenenm );

	scn->vwr3d_->display( true );
	scn->vwr3d_->showRotAxis( true );
	mAttachCB( vwr3d->viewmodechanged, uiODSceneMgr::viewModeChg );
	mAttachCB( vwr3d->pageupdown, uiODSceneMgr::pageUpDownPressed );
	scn->mdiwin_->display( true, false );

	treeToBeAdded.trigger( sceneid );
	initTree( *scn, vwridx_ );
	treeAdded.trigger( sceneid );
    }

    ObjectSet<ui3DViewer> vwrs;
    get3DViewers( vwrs );
    if ( appl_.menuMgrAvailable() && !vwrs.isEmpty() && vwrs[0] )
    {
	const bool isperspective = vwrs[0]->isCameraPerspective();
	appl_.menuMgr().setCameraPixmap( isperspective );
	appl_.menuMgr().updateAxisMode( true );
    }

    rebuildTrees();

}


void uiODSceneMgr::setSceneProperties()
{
    ObjectSet<ui3DViewer> vwrs;
    get3DViewers( vwrs );
    if ( vwrs.isEmpty() )
    {
	uiMSG().error( tr("No scenes available") );
	return;
    }

    int curvwridx = 0;
    if ( vwrs.size() > 1 )
    {
	const SceneID sceneid = askSelectScene();
	const ui3DViewer* vwr = get3DViewer( sceneid );
	if ( !vwr ) return;

	curvwridx = vwrs.indexOf( vwr );
    }

    uiScenePropertyDlg dlg( &appl_, vwrs, curvwridx );
    dlg.go();
}


void uiODSceneMgr::viewModeChg( CallBacker* cb )
{
    mDynamicCastGet(ui3DViewer*,vwr3d_,cb)
    if ( vwr3d_ ) setToViewMode( vwr3d_->isViewMode() );
}


void uiODSceneMgr::setToViewMode( bool yn )
{
    for ( auto* bundle : scenes_ )
	if ( bundle->vwr3d_ )
	    bundle->vwr3d_->setViewMode( yn );
    visServ().setViewMode( yn , false );
    if ( appl_.menuMgrAvailable() )
	appl_.menuMgr().updateViewMode( yn );

    updateStatusBar();
    viewModeChanged.trigger();
}


bool uiODSceneMgr::inViewMode() const
{
    for ( auto* bundle : scenes_ )
	if ( bundle->vwr3d_ )
	    return bundle->vwr3d_->isViewMode();

    return false;
}


void uiODSceneMgr::setToWorkMode( uiVisPartServer::WorkMode wm )
{
    bool yn = ( wm == uiVisPartServer::View ) ? true : false;

    for ( auto* bundle : scenes_ )
	if ( bundle->vwr3d_ )
	    bundle->vwr3d_->setViewMode( yn );
    if ( appl_.menuMgrAvailable() )
	appl_.menuMgr().updateViewMode( yn );

    visServ().setWorkMode( wm , false );
    updateStatusBar();
}


void uiODSceneMgr::actMode( CallBacker* )
{
    setToWorkMode( uiVisPartServer::Interactive );
}


void uiODSceneMgr::viewMode( CallBacker* )
{
    setToWorkMode( uiVisPartServer::View );
}


void uiODSceneMgr::pageUpDownPressed( CallBacker* cb )
{
    mCBCapsuleUnpack(bool,up,cb);
    applMgr().pageUpDownPressed( up );
}


void uiODSceneMgr::resetStatusBar( const VisID& id )
{
    appl_.statusBar()->message( uiString::emptyString(), mPosField );
    appl_.statusBar()->message( uiString::emptyString(), mValueField );
    appl_.statusBar()->message(mToUiStringTodo(visServ().getInteractionMsg(id)),
			       mNameField );
    appl_.statusBar()->message( uiString::emptyString(), mStatusField );
    appl_.statusBar()->setBGColor( mStatusField,
				   appl_.statusBar()->getBGColor(mPosField) );
}


void uiODSceneMgr::updateStatusBar()
{
    if ( visServ().isViewMode() )
	resetStatusBar();

    const Coord3 xytpos = visServ().getMousePos();
    const bool haspos = xytpos.isDefined();

    uiString msg;
    if ( haspos  )
    {
	const SceneID sceneid = visServ().getMouseSceneID();
	const ZDomain::Info* datazdom = visServ().zDomainInfo( sceneid );
	if ( !datazdom )
	    datazdom = &SI().zDomainInfo();

	const ZDomain::Info& displayzdom = datazdom->isDepth()
				    ? ZDomain::DefaultDepth( true ) : *datazdom;
	const double zfact = FlatView::Viewer::userFactor( *datazdom,
							   &displayzdom );
	const Coord3 crd( xytpos, xytpos.z_ * zfact );
	const int nrxydec = SI().nrXYDecimals();
	const int nrzdec = displayzdom.nrDecimals( mUdf(float) );
        const BinID bid( SI().transform( Coord(xytpos.x_,xytpos.y_) ) );
	msg = toUiString("%1	%2")
			    .arg( bid )
			    .arg( toUiString(crd,nrxydec,nrzdec) );
    }

    appl_.statusBar()->message( msg, mPosField );

    const BufferString valstr = visServ().getMousePosVal();
    if ( haspos )
    {
	if ( valstr.isEmpty() )
	    msg.setEmpty();
	else
	    msg = toUiString( "%1 = %2" ).arg( uiStrings::sValue() )
					 .arg( valstr );
    }
    else
	msg.setEmpty();

    appl_.statusBar()->message( msg, mValueField );

    if ( haspos )
	msg = mToUiStringTodo(visServ().getMousePosString() );
    else
	msg.setEmpty();

    if ( msg.isEmpty() )
    {
	const VisID selid = visServ().getSelObjectId();
	msg = mToUiStringTodo(visServ().getInteractionMsg( selid ) );
    }

    appl_.statusBar()->message( msg, mNameField );

    BufferString bsmsg;
    visServ().getPickingMessage( bsmsg );
    appl_.statusBar()->message( mToUiStringTodo(bsmsg), mStatusField );

    appl_.statusBar()->setBGColor( mStatusField, visServ().isPicking() ?
	    OD::Color(255,0,0) : appl_.statusBar()->getBGColor(mPosField) );
}


int uiODSceneMgr::getStereoType() const
{
    for ( auto* bundle : scenes_ )
    {
	if ( !bundle->isVSG() )
	    return (int)dCast(ui3DViewer*,bundle->vwr3d_)->getStereoType();
    }

    return 0;
}


void uiODSceneMgr::setStereoType( int type )
{
    OSGSceneBundle* firstscene = nullptr;
    for ( auto* bundle : scenes_ )
    {
	if ( bundle->isVSG() )
	    continue;

	firstscene = dCast(OSGSceneBundle*,bundle);
	break;
    }

    if ( !firstscene ) return;

    if ( type )
    {
	if ( !Settings::common().isFalse(sKeyWarnStereo) )
	{
	    bool wantmsg = uiMSG().showMsgNextTime(
		tr("Stereo viewing is not officially supported."
		"\nIt may not work well for your particular graphics setup.") );
	    if ( !wantmsg )
	    {
		Settings::common().setYN( sKeyWarnStereo, false );
		Settings::common().write();
	    }
	}
    }

    OD::StereoType stereotype = (OD::StereoType)type;
    auto* firstvwr = dCast(ui3DViewer*,firstscene->vwr3d_);
    const float stereooffset = firstvwr->getStereoOffset();
    for ( auto* bundle : scenes_ )
    {
	if ( bundle->isVSG() )
	    continue;

	ui3DViewer& vwr3d_ = *dCast(ui3DViewer*,bundle->vwr3d_);
	if ( !vwr3d_.setStereoType(stereotype) )
	{
	    uiMSG().error( tr("No support for this type of stereo rendering") );
	    return;
	}
	if ( type )
	    vwr3d_.setStereoOffset( stereooffset );
    }

    if ( type>0 )
	applMgr().setStereoOffset();
}


void uiODSceneMgr::tile()		{ mdiarea_->tile(); }
void uiODSceneMgr::tileHorizontal()	{ mdiarea_->tileHorizontal(); }
void uiODSceneMgr::tileVertical()	{ mdiarea_->tileVertical(); }
void uiODSceneMgr::cascade()		{ mdiarea_->cascade(); }


void uiODSceneMgr::layoutScenes()
{
    const int nrgrps = scenes_.size();
    if ( nrgrps == 1 && scenes_[0] )
	scenes_[0]->mdiwin_->display( true, false, true );
    else if ( nrgrps>1 && scenes_[0] )
	tile();
}


void uiODSceneMgr::toHomePos( CallBacker* )
{ mDoAllScenes(vwr3d_,toHomePos,); }
void uiODSceneMgr::saveHomePos( CallBacker* )
{ mDoAllScenes(vwr3d_,saveHomePos,); }

void uiODSceneMgr::resetHomePos( CallBacker* )
{
    mDoAllScenes(vwr3d_,resetHomePos,);
    mDoAllScenes(vwr3d_,setStartupView,);
}

void uiODSceneMgr::viewAll( CallBacker* )
{ mDoAllScenes(vwr3d_,viewAll,); }
void uiODSceneMgr::align( CallBacker* )
{ mDoAllScenes(vwr3d_,align,); }

void uiODSceneMgr::viewX( CallBacker* )
{ mDoAllScenes(vwr3d_,viewPlane,ui3DViewer::X); }
void uiODSceneMgr::viewY( CallBacker* )
{ mDoAllScenes(vwr3d_,viewPlane,ui3DViewer::Y); }
void uiODSceneMgr::viewZ( CallBacker* )
{ mDoAllScenes(vwr3d_,viewPlane,ui3DViewer::Z); }
void uiODSceneMgr::viewInl( CallBacker* )
{ mDoAllScenes(vwr3d_,viewPlane,ui3DViewer::Inl); }
void uiODSceneMgr::viewCrl( CallBacker* )
{ mDoAllScenes(vwr3d_,viewPlane,ui3DViewer::Crl); }

void uiODSceneMgr::setViewSelectMode( int md )
{
    mDoAllScenes(vwr3d_,viewPlane, sCast(ui3DViewer::PlaneType,md) );
}


void uiODSceneMgr::setViewSelectMode( const SceneID& sceneid,
				      ui3DViewer::PlaneType type )
{
    ui3DViewer* vwr = get3DViewer( sceneid );
    if ( !vwr )
	return;

    vwr->viewPlane( type );
}


void uiODSceneMgr::showRotAxis( CallBacker* cb )
{
    mDynamicCastGet(const uiAction*,act,cb)
    const bool show = act ? act->isChecked() : false;
    for ( auto* bundle : scenes_ )
	if ( bundle->vwr3d_ )
	    bundle->vwr3d_->showRotAxis( show );
    int idx = 0;
    for ( auto* bundle : scenes_ )
    {
	if ( bundle->isVSG() )
	    continue;

	const OD::Color& col = applMgr().visServer()->getSceneAnnotCol( idx++ );
	dCast(OSGSceneBundle*,bundle)->vwr3d_->setAnnotationColor( col );
    }
}


class uiSnapshotDlg : public uiDialog
{ mODTextTranslationClass(uiSnapshotDlg);
public:
			uiSnapshotDlg(uiParent*);

    enum		SnapshotType { Scene=0, Window, Desktop };
    SnapshotType	getSnapshotType() const;

protected:
    uiButtonGroup*	butgrp_;
};


uiSnapshotDlg::uiSnapshotDlg( uiParent* p )
    : uiDialog( p, Setup(tr("Specify snapshot"),
			 tr("Select area to take snapshot"),
			 mODHelpKey(mSnapshotDlgHelpID)) )
{
    butgrp_ = new uiButtonGroup( this, "Area type", OD::Vertical );
    butgrp_->setExclusive( true );
    new uiRadioButton( butgrp_, uiStrings::sScene() );
    new uiRadioButton( butgrp_, tr("Window") );
    new uiRadioButton( butgrp_, tr("Desktop") );
    butgrp_->selectButton( 0 );
}


uiSnapshotDlg::SnapshotType uiSnapshotDlg::getSnapshotType() const
{ return (uiSnapshotDlg::SnapshotType) butgrp_->selectedId(); }


void uiODSceneMgr::mkSnapshot( CallBacker* )
{
    uiSnapshotDlg snapdlg( &appl_ );
    if ( !snapdlg.go() )
	return;

    if ( snapdlg.getSnapshotType() == uiSnapshotDlg::Scene )
    {

	ObjectSet<ui3DViewer> viewers;
	get3DViewers( viewers );
	if ( viewers.size() == 0 ) return;

	uiPrintSceneDlg printdlg( &appl_, viewers );
	printdlg.go();
    }
    else
    {
	const bool desktop = snapdlg.getSnapshotType()==uiSnapshotDlg::Desktop;
	wingrabber_->grabDesktop( desktop );
	wingrabber_->go();
    }
}


void uiODSceneMgr::soloMode( CallBacker* )
{
    if ( !appl_.menuMgrAvailable() )
	return;

    TypeSet< TypeSet<VisID> > dispids;
    VisID selectedid;

    const bool issolomodeon = appl_.menuMgr().isSoloModeOn();
    for ( auto* bundle : scenes_ )
    {
	if ( bundle->isVSG() )
	    continue;

	auto* scene = dCast(OSGSceneBundle*,bundle);
	dispids += scene->itemmanager_->getDisplayIds( selectedid,
						    !issolomodeon );
    }

    visServ().setSoloMode( issolomodeon, dispids, selectedid );
    updateSelectedTreeItem();
}


void uiODSceneMgr::switchCameraType( CallBacker* )
{
    ObjectSet<ui3DViewer> vwrs;
    get3DViewers( vwrs );
    if ( vwrs.isEmpty() )
	return;

    mDoAllScenes(vwr3d_,toggleCameraType,);
    const bool isperspective = vwrs[0]->isCameraPerspective();
    if ( appl_.menuMgrAvailable() )
	appl_.menuMgr().setCameraPixmap( isperspective );
}


SceneID uiODSceneMgr::askSelectScene() const
{
    uiStringSet scenenms;
    TypeSet<SceneID> sceneids;
    for ( auto* bundle : scenes_ )
    {
	if ( bundle->isVSG() )
	    continue;

	auto* scene = dCast(OSGSceneBundle*,bundle);
	SceneID sceneid = scene->itemmanager_->sceneID();
	sceneids += sceneid;
	scenenms.add( getSceneName(sceneid) );
    }

    if ( sceneids.size() < 2 )
	return sceneids.isEmpty() ? SceneID::udf() : sceneids[0];

    StringListInpSpec* inpspec = new StringListInpSpec( scenenms );
    uiGenInputDlg dlg( &appl_, tr("Choose scene"), mNoDlgTitle, inpspec );
    const int selidx = dlg.go() ? dlg.getIntValue() : -1;
    return sceneids.validIdx(selidx) ? sceneids[selidx] : SceneID::udf();
}


void uiODSceneMgr::get3DViewers( ObjectSet<ui3DViewer>& vwrs )
{
    vwrs.erase();
    for ( auto* bundle : scenes_ )
    {
	if ( !bundle->isVSG() )
	    vwrs += dCast(ui3DViewer*,bundle->vwr3d_);
    }
}


const ui3DViewer* uiODSceneMgr::get3DViewer( const SceneID& sceneid ) const
{
    const SceneBundle* scene = getScene( sceneid );
    return scene ? dCast(ui3DViewer*,scene->vwr3d_) : 0;
}


ui3DViewer* uiODSceneMgr::get3DViewer( const SceneID& sceneid )
{
    const SceneBundle* scene = getScene( sceneid );
    return scene ? dCast(ui3DViewer*,scene->vwr3d_) : 0;
}


uiODTreeTop* uiODSceneMgr::getTreeItemMgr( const uiTreeView* lv ) const
{
    for ( auto* bundle : scenes_ )
    {
	if ( !bundle->isVSG() && bundle->lv_ == lv )
	    return bundle->itemmanager_;
    }

    return nullptr;
}


uiODTreeTop* uiODSceneMgr::getTreeItemMgr( const SceneID& sceneid ) const
{
    const SceneBundle* scene = getScene( sceneid );
    return scene ? scene->itemmanager_ : nullptr;
}


void uiODSceneMgr::translateText()
{
    for ( auto* bundle : scenes_ )
	bundle->itemmanager_->translateText();
}


void uiODSceneMgr::getSceneNames( uiStringSet& nms, int& active ) const
{
    mdiarea_->getWindowNames( nms );
    active = -1;

    const char* activenm = mdiarea_->getActiveWin();

    for ( int idx=0; idx<nms.size(); idx++ )
    {
	if ( nms[idx].getFullString()==activenm )
	{
	    active = idx;
	    break;
	}
    }
}


void uiODSceneMgr::getActiveSceneName( BufferString& nm ) const
{ nm = mdiarea_->getActiveWin(); }


SceneID uiODSceneMgr::getActiveSceneID() const
{
    const BufferString scenenm = mdiarea_->getActiveWin();
    for ( auto* bundle : scenes_ )
    {
	if ( bundle->isVSG() || !bundle->itemmanager_ )
	    continue;

	auto* scene = dCast(OSGSceneBundle*,bundle);
	if ( scenenm ==
	   getSceneName(scene->itemmanager_->sceneID()).getFullString() )
	    return scene->itemmanager_->sceneID();
    }

    return SceneID::udf();
}


void uiODSceneMgr::mdiAreaChanged( CallBacker* )
{
    bool visible = false;
    if ( !appl_.isMinimized() )
    {
	for ( auto* bundle : scenes_ )
	{
	    if ( !bundle->mdiwin_->isMinimized() )
	    {
		scenesShown.trigger();
		visible = true;
		break;
	    }
	}
    }
    if ( !visible )
	scenesHidden.trigger();

//    const bool wasparalysed = mdiarea_->paralyse( true );
    if ( appl_.menuMgrAvailable() )
	appl_.menuMgr().updateSceneMenu();
//    mdiarea_->paralyse( wasparalysed );

    const SceneID sceneid = getActiveSceneID();
    sceneChanged( sceneid );

    const BufferString activenm = mdiarea_->getActiveWin();
    for ( auto* bundle : scenes_ )
    {
	if ( bundle->isVSG() &&
	     activenm == bundle->mdiwin_->getTitle().getFullString() )
	{
	    bundle->raiseTree();
	    break;
	}
    }
}


void uiODSceneMgr::sceneChanged( SceneID sceneid )
{
    SceneBundle* scene = getScene( sceneid );
    if ( scene )
    {
	scene->raiseTree();
	if ( scene->vwr3d_ && appl_.menuMgrAvailable() )
	    appl_.menuMgr().setCameraPixmap(
			scene->vwr3d_->isCameraPerspective() );
    }

    activeSceneChanged.trigger();
}


void uiODSceneMgr::setActiveScene( int idx )
{
    SceneBundle* scene = scenes_.validIdx(idx) ? scenes_[idx] : nullptr;
    if ( scene )
	setActiveScene( scene->vwr3d_->sceneID() );
}


void uiODSceneMgr::setActiveScene( const SceneID& sceneid )
{
    const BufferString scenenm = getSceneName( sceneid ).getFullString();
    mdiarea_->setActiveWin( scenenm );
    sceneChanged( sceneid );
}


void uiODSceneMgr::createTreeDock( SceneBundle& bundle,
				   const uiString& caption )
{
    bundle.dw_ = new uiDockWin( &appl_, caption );
    bundle.dw_->setMinimumWidth( 200 );
    bundle.lv_ = new uiTreeView( bundle.dw_, caption.getFullString() );
    bundle.dw_->setObject( bundle.lv_ );
    uiStringSet labels;
    labels.add( sElements() );
    labels.add( uiStrings::sColor() );
    bundle.lv_->addColumns( labels );
    bundle.lv_->setFixedColumnWidth( cColorColumn(), 40 );
}


void uiODSceneMgr::getFactoryIdxs( TypeSet<int>& idxs ) const
{
    TypeSet<int> placeidxs;
    for ( int idx=0; idx<tifs_->nrFactories(); idx++ )
    {
	const OD::Pol2D3D pol2d = tifs_->getPol2D3D( idx );
	if ( SI().survDataType() == OD::Both2DAnd3D ||
	     pol2d == OD::Both2DAnd3D ||
	     pol2d == SI().survDataType() )
	{
	    idxs += idx;
	    placeidxs += tifs_->getPlacementIdx( idx );
	}
    }

    sort_coupled( placeidxs.arr(), idxs.arr(), idxs.size() );
}


void uiODSceneMgr::initTree( SceneBundle& scn, int vwridx )
{
    const uiString capt = scn.isVSG() ? scn.mdiwin_->getTitle()
				      : tr( "Tree scene %1" ).arg( vwridx );
    createTreeDock( scn, capt );

    scn.itemmanager_ = new uiODTreeTop( scn.vwr3d_, scn.lv_, &applMgr(), tifs_);
    uiODSceneTreeItem* sceneitm =
	new uiODSceneTreeItem( scn.mdiwin_->getTitle(),
			       scn.vwr3d_->sceneID() );
    scn.itemmanager_->addChild( sceneitm, false );

    TypeSet<int> idxs;
    getFactoryIdxs( idxs );

    for ( int idx=0; idx<idxs.size(); idx++ )
    {
	const int fidx = idxs[idx];
	scn.itemmanager_->addChild(
		tifs_->getFactory(fidx)->create(), true );
    }

    if ( scn.isVSG() )
    {
	// The OSG items are only shown: they would add OSG objects
	scn.itemmanager_->disabRightClick( true );
	scn.itemmanager_->disabAnyClick( true );
    }

    scn.lv_->display( true );
    appl_.addDockWindow( *scn.dw_, uiMainWin::Left );
    tabifyTree( scn );

    scn.dw_->setVisible( treeShown() );
    if ( scn.isVSG() )
	scn.dw_->raise();
    else
	sceneitm->select();
}


void uiODSceneMgr::updateTrees()
{
    for ( auto* bundle : scenes_ )
    {
	bundle->itemmanager_->updateColumnText( cNameColumn() );
	bundle->itemmanager_->updateColumnText( cColorColumn() );
	bundle->itemmanager_->updateCheckStatus();
    }
}


void uiODSceneMgr::rebuildTrees()
{
    for ( auto* bundle : scenes_ )
    {
	if ( bundle->isVSG() )
	    continue;

	auto& scene = *dCast(OSGSceneBundle*,bundle);
	const SceneID sceneid = scene.vwr3d_->sceneID();
	TypeSet<VisID> visids;
	visServ().getSceneChildIds( sceneid, visids );

	for ( int idy=0; idy<visids.size(); idy++ )
	{
	    mDynamicCastGet( const visSurvey::SurveyObject*, surobj,
		visServ().getObject(visids[idy]) );

	    if ( surobj && surobj->getSaveInSessionsFlag() == false )
		continue;

	    uiODDisplayTreeItem::create( scene.itemmanager_, &applMgr(),
					 visids[idy] );
	}
    }
    updateSelectedTreeItem();
}


uiTreeView* uiODSceneMgr::getTree( const SceneID& sceneid )
{
    SceneBundle* scene = getScene( sceneid );
    return scene ? scene->lv_ : nullptr;
}


void uiODSceneMgr::showTree( bool yn )
{
    for ( auto* bundle : scenes_ )
	bundle->showTree( yn );
}


bool uiODSceneMgr::treeShown() const
{
    return scenes_.size()>1 ? scenes_[0]->dw_->isVisible() : true;
}


void uiODSceneMgr::setItemInfo( const VisID& id )
{
    mDoAllScenes(itemmanager_,updateColumnText,cNameColumn());
    mDoAllScenes(itemmanager_,updateColumnText,cColorColumn());
    resetStatusBar( id );
}


void uiODSceneMgr::updateItemToolbar( const VisID& id )
{
    visServ().getToolBarHandler()->setMenuID( id.asInt() );
    visServ().getToolBarHandler()->executeMenu(); // addButtons
}


void uiODSceneMgr::updateSelectedTreeItem()
{
    int nrodscenes = 0;
    for ( const auto* bundle : scenes_ )
    {
	if ( !bundle->isVSG() )
	    nrodscenes++;
    }

    const VisID id = visServ().getSelObjectId();
    if ( id.isValid() )
    {
	resetStatusBar( id );
	//applMgr().modifyColorTable( id );
	if ( !visServ().isOn(id) )
	    visServ().turnOn(id, true, true);
	else if ( nrodscenes != 1 && visServ().isSoloMode() )
	    visServ().updateDisplay( true, id );
    }

    const int visidasint = id.asInt();
    for ( auto* bundle : scenes_ )
    {
	auto* itmmgr = bundle->itemmanager_;
	itmmgr->updateSelection( visidasint );
	itmmgr->updateSelTreeColumnText( cNameColumn() );
	itmmgr->updateSelTreeColumnText( cColorColumn() );
    }

    updateItemToolbar( id );
    applMgr().updateColorTable( id, visServ().getSelAttribNr() );

    if ( !applMgr().attrServer() )
	return;

    bool found = applMgr().attrServer()->attrSetEditorActive();
    bool gotoact = false;
    if ( !found )
    {
	mDynamicCastGet(const uiODDisplayTreeItem*,treeitem,findItem(id))
	if ( treeitem )
	    gotoact = treeitem->actModeWhenSelected();
    }

    if ( gotoact && !applMgr().attrServer()->attrSetEditorActive() )
	actMode( 0 );
}


VisID uiODSceneMgr::getIDFromName( const char* str ) const
{
    for ( const auto* bundle : scenes_ )
    {
	const uiTreeItem* itm = bundle->itemmanager_->findChild( str );
	if ( itm )
	    return VisID( itm->selectionKey() );
    }

    return VisID::udf();
}


void uiODSceneMgr::disabRightClick( bool yn )
{
    for ( auto* bundle : scenes_ )
    {
	if ( !bundle->isVSG() ) // VSG trees keep their right-click off
	    bundle->itemmanager_->disabRightClick( yn );
    }
}


void uiODSceneMgr::disabTrees( bool yn )
{
    const bool wasparalysed = mdiarea_->paralyse( true );

    for ( auto* bundle : scenes_ )
	bundle->setTreeSensitive( !yn );

    mdiarea_->paralyse( wasparalysed );
}


#define mGetOrAskForScene \
    auto* scene = dCast(OSGSceneBundle*,getScene(scnid)); \
    const SceneID sceneid = scene ? scnid : askSelectScene(); \
    if ( !scene ) \
	scene = dCast(OSGSceneBundle*,getScene(sceneid)); \
    \
    if ( !scene ) \
	return VisID::udf();

VisID uiODSceneMgr::addWellItem( const MultiID& mid, const SceneID& scnid )
{
    mGetOrAskForScene

    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !scene || !ioobj ) return VisID::udf();

    if ( ioobj->group() != mTranslGroupName(Well) )
	return VisID::udf();

    auto* itm = new uiODWellTreeItem( mid );
    scene->itemmanager_->addChild( itm, false );
    return itm->displayID();
}


void uiODSceneMgr::addWellItems( const TypeSet<MultiID>& mids,
				 const SceneID& scnid )
{
    if ( mids.isEmpty() )
	return;

    auto* scene = dCast(OSGSceneBundle*,getScene(scnid));
    const SceneID sceneid = scene ? scnid : askSelectScene();
    if ( !scene )
	scene = dCast(OSGSceneBundle*,getScene(sceneid));

    if ( !scene )
	return;

    uiUserShowWait usw( &appl_, tr("Adding well(s) to 3D scene") );
    for ( const auto& mid : mids )
	addWellItem( mid, sceneid );
}


VisID uiODSceneMgr::addDisplayTreeItem( uiODDisplayTreeItem* itm,
					const SceneID& scnid )
{
    mGetOrAskForScene

    scene->itemmanager_->addChild( itm, false );
    return itm->displayID();
}


void uiODSceneMgr::getLoadedPickSetIDs( TypeSet<MultiID>& picks, bool poly,
					const SceneID& sceneid ) const
{
    if ( sceneid.isValid() )
    {
	const SceneBundle* scene = getScene( sceneid );
	if ( !scene ) return;

	gtLoadedPickSetIDs( *scene, picks, poly );
	return;
    }

    for ( const auto* bundle : scenes_ )
	gtLoadedPickSetIDs( *bundle, picks, poly );

}


void uiODSceneMgr::gtLoadedPickSetIDs( const SceneBundle& scene,
	TypeSet<MultiID>& picks, bool poly ) const
{
    for ( int chidx=0; chidx<scene.itemmanager_->nrChildren(); chidx++ )
    {
	const uiTreeItem* chlditm = scene.itemmanager_->getChild( chidx );
	if ( !chlditm )
	    continue;

	gtLoadedPickSetIDs( *chlditm, picks, poly );
    }
}


void uiODSceneMgr::gtLoadedPickSetIDs( const uiTreeItem& topitm,
	TypeSet<MultiID>& picks, bool poly ) const
{
    for ( int chidx=0; chidx<topitm.nrChildren(); chidx++ )
    {
	ConstRefMan<Pick::Set> ps = nullptr;
	const uiTreeItem* chlditm = topitm.getChild( chidx );
	if ( poly )
	{
	    mDynamicCastGet(const uiODPolygonTreeItem*,polyitem,chlditm)
	    if ( polyitem )
		ps = polyitem->getSet();
	}
	else
	{
	    mDynamicCastGet(const uiODPickSetTreeItem*,pickitem,chlditm)
	    if ( pickitem )
		ps = pickitem->getSet();
	}

	if ( ps )
	{
	    const MultiID& mid = Pick::Mgr().get( *ps );
	    picks.addIfNew( mid );
	}
    }
}


void uiODSceneMgr::getLoadedEMIDs( TypeSet<EM::ObjectID>& emids,
				   const char* type,
				   const SceneID& sceneid ) const
{
    if ( sceneid.isValid() )
    {
	const SceneBundle* scene = getScene( sceneid );
	if ( !scene ) return;
	gtLoadedEMIDs( scene, emids, type );
	return;
    }

    for ( const auto* bundle : scenes_ )
	gtLoadedEMIDs( bundle, emids, type );
}


void uiODSceneMgr::gtLoadedEMIDs( const uiTreeItem* topitm,
				  TypeSet<EM::ObjectID>& emids,
				  const char* type ) const
{
    for ( int chidx=0; chidx<topitm->nrChildren(); chidx++ )
    {
	const uiTreeItem* chlditm = topitm->getChild( chidx );
	mDynamicCastGet(const uiODEarthModelSurfaceTreeItem*,emtreeitem,chlditm)
	mDynamicCastGet(const uiODFaultTreeItem*,flttreeitem,chlditm)
	mDynamicCastGet(const uiODFaultStickSetTreeItem*,fsstreeitem,chlditm)
	mDynamicCastGet(const uiODFaultSetTreeItem*,fltsettreeitem,chlditm)
	if ( !emtreeitem && !flttreeitem && !fsstreeitem && !fltsettreeitem )
	    continue;

	if ( !type || EM::Horizon3D::typeStr()==type )
	{
	    mDynamicCastGet(const uiODHorizonTreeItem*,hor3dtreeitm,chlditm)
	    if ( hor3dtreeitm )
		emids.addIfNew( hor3dtreeitm->emObjectID() );
	}
	else if ( !type || EM::Horizon2D::typeStr()==type )
	{
	    mDynamicCastGet(const uiODHorizon2DTreeItem*,hor2dtreeitm,chlditm)
	    if ( hor2dtreeitm )
		emids.addIfNew( hor2dtreeitm->emObjectID() );
	}
	else if ( !type || EM::Fault3D::typeStr()==type )
	{
	    if ( flttreeitem )
		emids.addIfNew( flttreeitem->emObjectID() );
	}
	else if ( !type || EM::FaultStickSet::typeStr()==type )
	{
	    if ( fsstreeitem )
		emids.addIfNew( fsstreeitem->emObjectID() );
	}
	else if ( !type || EM::FaultSet3D::typeStr()==type )
	{
	    if ( fltsettreeitem )
		emids.addIfNew( fltsettreeitem->emObjectID() );
	}
    }
}


void uiODSceneMgr::gtLoadedEMIDs( const SceneBundle* scene,
				  TypeSet<EM::ObjectID>& emids,
				  const char* type ) const
{
    for ( int chidx=0; chidx<scene->itemmanager_->nrChildren(); chidx++ )
    {
	const uiTreeItem* chlditm = scene->itemmanager_->getChild( chidx );
	gtLoadedEMIDs( chlditm, emids, type );
    }
}


VisID uiODSceneMgr::addEMItem( const EM::ObjectID& emid, const SceneID& scnid )
{
    mGetOrAskForScene;

    RefMan<EM::EMObject> obj = EM::EMM().getObject( emid );
    if ( !obj )
	return VisID::udf();

    StringView type = obj->getTypeStr();
    uiODDisplayTreeItem* itm;
    if ( type==EM::Horizon3D::typeStr() )
	itm = new uiODHorizonTreeItem(emid,false,false);
    else if ( type==EM::Horizon2D::typeStr() )
	itm = new uiODHorizon2DTreeItem(emid);
    else if ( type==EM::Fault3D::typeStr() )
	itm = new uiODFaultTreeItem(emid);
    else if ( type==EM::FaultStickSet::typeStr() )
	itm = new uiODFaultStickSetTreeItem(emid);
    else if ( type==EM::FaultSet3D::typeStr() )
	itm = new uiODFaultSetTreeItem(emid);
    else if ( type==EM::RandomPosBody::typeStr() )
	itm = new uiODBodyDisplayTreeItem(emid);
    else if ( type==EM::MarchingCubesSurface::typeStr() )
	itm = new uiODBodyDisplayTreeItem(emid);
    else
	return VisID::udf();

    scene->itemmanager_->addChild( itm, false );
    return itm->displayID();
}


VisID uiODSceneMgr::addPickSetItem( const MultiID& mid, const SceneID& sceneid )
{
    RefMan<Pick::Set> ps = applMgr().pickServer()->loadSet( mid );
    if ( !ps )
	ps = new Pick::Set( mid.toString() );

    return addPickSetItem( *ps, sceneid );
}


VisID uiODSceneMgr::addPickSetItem( Pick::Set& ps, const SceneID& scnid )
{
    mGetOrAskForScene

    uiODDisplayTreeItem* itm;
    if ( ps.isPolygon() )
	itm = new uiODPolygonTreeItem( VisID::udf(), ps );
    else
	itm = new uiODPickSetTreeItem( VisID::udf(), ps );

    scene->itemmanager_->addChild( itm, false );
    return itm->displayID();
}


VisID uiODSceneMgr::addRandomLineItem( const RandomLineID& rlid,
				       const SceneID& scnid )
{
    mGetOrAskForScene

    auto* itm = new uiODRandomLineTreeItem( VisID::udf(),
					uiODRandomLineTreeItem::Empty, rlid );
    scene->itemmanager_->addChild( itm, false );
    itm->displayDefaultData();
    return itm->displayID();
}


VisID uiODSceneMgr::add2DLineItem( const Pos::GeomID& geomid,
				   const SceneID& scnid, bool withdata )
{
    mGetOrAskForScene

    auto* itm = new uiOD2DLineTreeItem( geomid );
    scene->itemmanager_->addChild( itm, false );
    if ( withdata )
	itm->displayDefaultData();

    return itm->displayID();
}


VisID uiODSceneMgr::add2DLineItem( const Pos::GeomID& geomid,
				   const SceneID& sceneid )
{
    return add2DLineItem( geomid, sceneid, false );
}


VisID uiODSceneMgr::add2DLineItem( const MultiID& mid , const SceneID& scnid )
{
    mGetOrAskForScene
    ConstRefMan<Survey::Geometry> geom = Survey::GM().getGeometry( mid );
    if ( !geom )
	return VisID::udf();

    const Pos::GeomID geomid = geom->getID();
    auto* itm = new uiOD2DLineTreeItem( geomid );
    scene->itemmanager_->addChild( itm, false );
    return itm->displayID();
}


VisID uiODSceneMgr::addInlCrlItem( OD::SliceType st, int nr,
				   const SceneID& scnid )
{
    mGetOrAskForScene
    uiODPlaneDataTreeItem* itm = nullptr;
    TrcKeyZSampling tkzs = SI().sampling(true);
    if ( st == OD::SliceType::Inline )
    {
	itm = new uiODInlineTreeItem(
				VisID::udf(), uiODPlaneDataTreeItem::Empty );
	tkzs.hsamp_.setInlRange( Interval<int>(nr,nr) );
    }
    else if ( st == OD::SliceType::Crossline )
    {
	itm = new uiODCrosslineTreeItem(
				VisID::udf(), uiODPlaneDataTreeItem::Empty );
	tkzs.hsamp_.setCrlRange( Interval<int>(nr,nr) );
    }
    else
	return VisID::udf();

    if ( !scene->itemmanager_->addChild(itm,false) )
	return VisID::udf();

    itm->setTrcKeyZSampling( tkzs );
    itm->displayDefaultData();
    return itm->displayID();
}


VisID uiODSceneMgr::addZSliceItem( const TrcKeyZSampling& tkzs,
				   const SceneID& scnid )
{
    mGetOrAskForScene
    auto* itm =
	new uiODZsliceTreeItem( VisID::udf(), uiODPlaneDataTreeItem::Empty );

    if ( !scene->itemmanager_->addChild(itm,false) )
	return VisID::udf();

    itm->setTrcKeyZSampling( tkzs );
    itm->displayDefaultData();
    return itm->displayID();
}


VisID uiODSceneMgr::addZSliceItem( const TrcKeyZSampling& tkzs,
				   const Attrib::SelSpec& sp,
				   const SceneID& scnid )
{
    mGetOrAskForScene
    auto* itm =
	new uiODZsliceTreeItem( VisID::udf(), uiODPlaneDataTreeItem::Empty );

    if ( !scene->itemmanager_->addChild(itm,false) )
	return VisID::udf();

    itm->setTrcKeyZSampling( tkzs );
    const Attrib::DescID id = sp.id();
    itm->displayDataFromDesc( id, false );
    return itm->displayID();
}


VisID uiODSceneMgr::addZSliceItem( RegularSeisDataPack& dp,
				   const Attrib::SelSpec& sp,
				   const FlatView::DataDispPars::VD& ddp,
				   const SceneID& scnid )
{
    mGetOrAskForScene
    auto* itm =
	new uiODZsliceTreeItem( VisID::udf(), uiODPlaneDataTreeItem::Empty );

    if ( !scene->itemmanager_->addChild(itm,false) )
	return VisID::udf();

    itm->displayDataFromDataPack( dp, sp, ddp );
    return itm->displayID();
}


void uiODSceneMgr::removeTreeItem( const VisID& displayid )
{
    for ( auto* bundle : scenes_ )
    {
	uiTreeItem* itm = bundle->itemmanager_->findChild( displayid.asInt() );
	if ( itm )
	{
	    itm->prepareForShutdown();
	    bundle->itemmanager_->removeChild( itm );
	}
    }
}


uiTreeItem* uiODSceneMgr::findSceneItem( const SceneID& sceneid )
{
    for ( const auto* bundle : scenes_ )
    {
	uiTreeItem* itm = bundle->itemmanager_->findChild( sceneid.asInt() );
	if ( itm )
	    return itm;
    }

    return nullptr;
}


uiTreeItem* uiODSceneMgr::findItem( const VisID& displayid )
{
    for ( const auto* bundle : scenes_ )
    {
	uiTreeItem* itm = bundle->itemmanager_->findChild( displayid.asInt() );
	if ( itm )
	    return itm;
    }

    return nullptr;
}


void uiODSceneMgr::findItems( const char* nm, ObjectSet<uiTreeItem>& items )
{
    findItems( nm, items, SceneID::udf() );
}


void uiODSceneMgr::findItems( const char* nm, ObjectSet<uiTreeItem>& items,
			      const SceneID& sceneid )
{
    deepErase( items );
    for ( auto* bundle : scenes_ )
    {
	if ( !sceneid.isValid() || bundle->sceneID() == sceneid )
	    bundle->itemmanager_->findChildren( nm, items );
    }
}


uiTreeItem* uiODSceneMgr::findItem( const VisID& displayid, int attrib )
{
    uiTreeItem* dispitem = findItem( displayid );
    if ( !dispitem )
	return nullptr;

    for ( int idx=0; idx<dispitem->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODDataTreeItem*,dataitem,dispitem->getChild(idx))
	if ( dataitem && dataitem->attribNr()==attrib )
	    return dataitem;
    }

    return nullptr;
}


void uiODSceneMgr::displayIn2DViewer( const VisID& visid, int attribid,
				      bool dowva )
{
    const FlatView::Viewer::VwrDest dest = FlatView::Viewer::getDest( dowva,
								      !dowva );
    appl_.viewer2DMgr().displayIn2DViewer( visid, attribid, dest );
}


void uiODSceneMgr::remove2DViewer( const VisID& visid )
{
    appl_.viewer2DMgr().remove2DViewer( visid );
}


void uiODSceneMgr::doDirectionalLight(CallBacker*)
{
    visServ().setDirectionalLight();
}


const uiODSceneMgr::SceneBundle* uiODSceneMgr::getScene(
						const SceneID& sceneid ) const
{ return getNonConst(*this).getScene( sceneid ); }


void uiODSceneMgr::font3DChanged( CallBacker* )
{
    uiFont& font3d = FontList().get( FontData::key(FontData::Graphics3D) );
    for ( auto* bundle : scenes_ )
    {
	if ( bundle->vwr3d_ )
	    bundle->vwr3d_->setAnnotationFont( font3d.fontData() );

	RefMan<visSurvey::Scene> visscene =
				visServ().getScene( bundle->sceneID() );
	if ( visscene )
	    visscene->setAnnotFont( font3d.fontData() );
    }
}


void uiODSceneMgr::showIfMinimized( CallBacker* )
{
    if ( appl_.isMinimized() )
	appl_.show();
}

// uiODSceneMgr::SceneBundle
uiODSceneMgr::SceneBundle::~SceneBundle()
{
    delete vwr3d_;
    delete mdiwin_;
    delete itemmanager_;
    delete dw_;
}


SceneID uiODSceneMgr::SceneBundle::sceneID() const
{
    return vwr3d_ ? vwr3d_->sceneID() : SceneID::udf();
}


void uiODSceneMgr::SceneBundle::setName( const uiString& nm )
{
    if ( mdiwin_ )
	mdiwin_->setTitle( nm );

    if ( dw_ )
	dw_->setDockName( nm );
}


void uiODSceneMgr::SceneBundle::raiseTree()
{
    if ( dw_ )
	dw_->raise();
}


void uiODSceneMgr::SceneBundle::showTree( bool yn )
{
    if ( dw_ )
	dw_->setVisible( yn );
}


void uiODSceneMgr::SceneBundle::setTreeSensitive( bool yn )
{
    if ( lv_ )
	lv_->setSensitive( yn );
}


// uiODSceneMgr::VSGSceneBundle
uiODSceneMgr::VSGSceneBundle::VSGSceneBundle( uiMdiArea* mdiarea )
{
    if ( !mdiarea )
	return;

    mdiwin_ = new uiMdiAreaWindow( *mdiarea, toUiString("MDI Area Window") );
    mdiarea->addWindow( mdiwin_ );
}


// uiODSceneMgr::OSGSceneBundle
uiODSceneMgr::OSGSceneBundle::OSGSceneBundle( uiMdiArea* mdiarea )
{
    if ( !mdiarea )
	return;

    mdiwin_ = new uiMdiAreaWindow( *mdiarea, toUiString("MDI Area Window") );
    mdiwin_->setIcon( scene_xpm_data );
    vwr3d_ = new ui3DViewer( mdiwin_ );
    vwr3d_->setPrefWidth( 400 );
    vwr3d_->setPrefHeight( 400 );
    mdiarea->addWindow( mdiwin_ );
}



// uiKeyBindingSettingsGroup
uiKeyBindingSettingsGroup::uiKeyBindingSettingsGroup( uiParent* p, Settings& s )
    : uiSettingsGroup( p, tr("Mouse interaction"), s )
    , keybindingfld_(nullptr)
    , wheeldirectionfld_(nullptr)
    , trackpadzoomspeedfld_(nullptr)
    , initialzoomfactor_(0)
    , initialmousewheelreversal_(false)
{
    TypeSet<SceneID> sceneids;
    if ( ODMainWin()->applMgr().visServer() )
	ODMainWin()->applMgr().visServer()->getSceneIds( sceneids );

    const ui3DViewer* viewer = sceneids.size()
	? ODMainWin()->sceneMgr().get3DViewer( sceneids[0] )
	: nullptr;

    if ( viewer )
    {
	BufferStringSet keyset;
	viewer->getAllKeyBindings( keyset );

	keybindingfld_ = new uiGenInput( this, tr("3D Mouse Controls"),
					 StringListInpSpec( keyset ) );

	setts_.get( ui3DViewer::sKeyBindingSettingsKey(), initialkeybinding_ );
	keybindingfld_->setText( viewer->getCurrentKeyBindings() );

	setts_.getYN( SettingsAccess::sKeyMouseWheelReversal(),
		     initialmousewheelreversal_ );

	wheeldirectionfld_ = new uiGenInput( this,
	    tr("Mouse wheel direction"),
	    BoolInpSpec( !viewer->getReversedMouseWheelDirection(),
			uiStrings::sNormal(),
			uiStrings::sReversed()) );
	wheeldirectionfld_->attach( alignedBelow, keybindingfld_ );

#ifdef __mac__

	initialzoomfactor_ = viewer->getMouseWheelZoomFactor();
	const bool istrackpad =
	  fabs(initialzoomfactor_-MouseEvent::getDefaultTrackpadZoomFactor())<
	  fabs(initialzoomfactor_-MouseEvent::getDefaultMouseWheelZoomFactor());

	trackpadzoomspeedfld_ = new uiGenInput( this,
	       tr("Optimize zoom speed for"),
	       BoolInpSpec( istrackpad, tr("Trackpad"), uiStrings::sMouse()) );
	trackpadzoomspeedfld_->attach( alignedBelow, wheeldirectionfld_ );
#endif
    }
}


uiKeyBindingSettingsGroup::~uiKeyBindingSettingsGroup()
{}


HelpKey uiKeyBindingSettingsGroup::helpKey() const
{
    return mODHelpKey(mODSceneMgrsetKeyBindingsHelpID);
}


bool uiKeyBindingSettingsGroup::acceptOK()
{
    if ( !keybindingfld_ )
	return true;

    TypeSet<SceneID> sceneids;
    if ( ODMainWin()->applMgr().visServer() )
	ODMainWin()->applMgr().visServer()->getSceneIds( sceneids );

    const BufferString keybinding = keybindingfld_->text();
    const bool reversedwheel = !wheeldirectionfld_->getBoolValue();


    for ( int idx=0; idx<sceneids.size(); idx++ )
    {
	ui3DViewer* viewer = ODMainWin()->sceneMgr().get3DViewer(sceneids[idx]);
	viewer->setKeyBindings( keybinding );

	viewer->setReversedMouseWheelDirection( reversedwheel );
    }

    const ObjectSet<uiGraphicsViewBase>& allviewers =
					uiGraphicsViewBase::allInstances();

    for ( int idx=0; idx<allviewers.size(); idx++ )
    {
	const_cast<uiGraphicsViewBase*>(allviewers[idx])
		->setMouseWheelReversal( reversedwheel );
    }

    if ( trackpadzoomspeedfld_ )
    {
	const float zoomfactor = trackpadzoomspeedfld_->getBoolValue()
	    ? MouseEvent::getDefaultTrackpadZoomFactor()
	    : MouseEvent::getDefaultMouseWheelZoomFactor();

	for ( int idx=0; idx<sceneids.size(); idx++ )
	{
	    ui3DViewer* viewer =
		ODMainWin()->sceneMgr().get3DViewer(sceneids[idx]);
	    viewer->setMouseWheelZoomFactor(zoomfactor);
	}
	/*TODO: It was not easy to find a good handling of the mouse events,
	 as it is done in many places, and differently.
	for ( int idx=0; idx<allviewers.size(); idx++ )
	{
	    const_cast<uiGraphicsViewBase*>(allviewers[idx])
		->setMouseWheelZoomFactor( zoomfactor );
	}
	 */

	updateSettings( initialzoomfactor_, zoomfactor,
			SettingsAccess::sKeyMouseWheelZoomFactor() );
    }



    updateSettings( initialkeybinding_, keybinding,
		   ui3DViewer::sKeyBindingSettingsKey() );

    updateSettings( initialmousewheelreversal_, reversedwheel,
		   SettingsAccess::sKeyMouseWheelReversal() );

    return true;
}
