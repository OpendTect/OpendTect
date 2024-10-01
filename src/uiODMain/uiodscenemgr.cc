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
    for ( auto* scene : scenes_ )
	scene->itemmanager_->prepareForShutdown();
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
    if ( scenes_.isEmpty() )
	addScene(true);
}


void uiODSceneMgr::cleanUp( bool startnew )
{
    mdiarea_->closeAll();
    // closeAll() cascades callbacks which remove the scene from set

    visServ().deleteAllObjects();
    vwridx_ = 0;
    if ( startnew )
	addScene( true );
}


uiODSceneMgr::Scene& uiODSceneMgr::mkNewScene()
{
    uiODSceneMgr::Scene& scn = *new uiODSceneMgr::Scene( mdiarea_ );
    mAttachCB( scn.mdiwin_->closed(), uiODSceneMgr::removeSceneCB );
    mAttachCB( scn.mdiwin_->windowShown(), uiODSceneMgr::mdiAreaChanged );
    mAttachCB( scn.mdiwin_->windowHidden(), uiODSceneMgr::mdiAreaChanged );
    scenes_ += &scn;
    vwridx_++;
    BufferString vwrnm( "Viewer Scene ", vwridx_ );
    scn.vwr3d_->setName( vwrnm );
    return scn;
}


SceneID uiODSceneMgr::addScene( bool maximized, ZAxisTransform* zt,
				const uiString& name )
{
    if ( !canAddSceneAtStartup() )
	return SceneID::udf();

    Scene& scn = mkNewScene();
    const SceneID sceneid = visServ().addScene();
    RefMan<visSurvey::Scene> visscene = visServ().getScene( sceneid );
    if ( visscene )
    {
	if ( scn.vwr3d_->getPolygonSelector() )
	    visscene->setPolygonSelector( scn.vwr3d_->getPolygonSelector() );
	if ( scn.vwr3d_->getSceneColTab() )
	    visscene->setSceneColTab( scn.vwr3d_->getSceneColTab() );
	mAttachCB( visscene->sceneboundingboxupdated,
		   uiODSceneMgr::newSceneUpdated );
    }

    scn.vwr3d_->setScene( visscene.ptr() );
    uiString title = uiStrings::phrJoinStrings( uiStrings::sScene(),
					       toUiString(vwridx_) );

    scn.mdiwin_->setTitle( title );
    visServ().setSceneName( sceneid, title );
    scn.vwr3d_->display( true );
    scn.vwr3d_->setAnnotationFont( visscene ? visscene->getAnnotFont()
					    : FontData() );
    mAttachCB( scn.vwr3d_->viewmodechanged, uiODSceneMgr::viewModeChg );
    mAttachCB( scn.vwr3d_->pageupdown, uiODSceneMgr::pageUpDownPressed );
    scn.mdiwin_->display( true, false, maximized );
    actMode(0);
    treeToBeAdded.trigger( sceneid );
    initTree( scn, vwridx_ );
    treeAdded.trigger( sceneid );

    if ( scenes_.size()>1 && scenes_[0] )
    {
	scn.vwr3d_->setStereoType( scenes_[0]->vwr3d_->getStereoType() );
	scn.vwr3d_->setStereoOffset(
		scenes_[0]->vwr3d_->getStereoOffset() );
	scn.vwr3d_->showRotAxis( scenes_[0]->vwr3d_->rotAxisShown() );
	if ( !scenes_[0]->vwr3d_->isCameraPerspective() )
	    scn.vwr3d_->toggleCameraType();
	visServ().displaySceneColorbar( visServ().sceneColorbarDisplayed() );
    }
    else if ( scenes_[0] )
    {
	const bool isperspective = scenes_[0]->vwr3d_->isCameraPerspective();
	if ( appl_.menuMgrAvailable() )
	{
	    appl_.menuMgr().setCameraPixmap( isperspective );
	    appl_.menuMgr().updateAxisMode( true );
	}
	scn.vwr3d_->showRotAxis( true );
    }

    if ( name.isSet() )
	setSceneName( sceneid, name );

    visServ().setZAxisTransform( sceneid, zt, 0 );
    visServ().turnSelectionModeOn( visServ().isSelectionModeOn() );

    scn.vwr3d_->setStartupView();

    return sceneid;
}


void uiODSceneMgr::newSceneUpdated( CallBacker* )
{
    if ( scenes_.size() >0 && scenes_.last()->vwr3d_ )
    {
	scenes_.last()->vwr3d_->viewAll( false );
	tiletimer_->start( 10,true );

	RefMan<visSurvey::Scene> visscene = visServ().getScene(
				scenes_.last()->vwr3d_->sceneID() );
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


void uiODSceneMgr::removeScene( uiODSceneMgr::Scene& scene )
{
    appl_.colTabEd().setColTab( 0, mUdf(int), mUdf(int) );
    appl_.removeDockWindow( scene.dw_ );

    if ( scene.itemmanager_ )
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
    int idxnr = -1;
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	if ( grp == scenes_[idx]->mdiwin_->mainObject() )
	{
	    idxnr = idx;
	    break;
	}
    }
    if ( idxnr < 0 ) return;

    uiODSceneMgr::Scene* scene = scenes_[idxnr];
    removeScene( *scene );
}


void uiODSceneMgr::setSceneName( const SceneID& sceneid, const uiString& nm )
{
    visServ().setSceneName( sceneid, nm );
    Scene* scene = getScene( sceneid );
    if ( !scene )
	return;

    scene->mdiwin_->setTitle( nm );
    scene->dw_->setDockName( nm );
    uiTreeItem* itm = findSceneItem( sceneid );
    if ( itm )
	itm->updateColumnText( uiODSceneMgr::cNameColumn() );
}


uiString uiODSceneMgr::getSceneName( const SceneID& sceneid ) const
{
    return mSelf().visServ().getSceneName( sceneid );
}


void uiODSceneMgr::getScenePars( IOPar& iopar )
{
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	IOPar iop;
	scenes_[idx]->vwr3d_->fillPar( iop );
	iopar.mergeComp( iop, toString(idx) );
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

	Scene& scn = mkNewScene();
	if ( !scn.vwr3d_->usePar(*scenepar) )
	{
	    removeScene( scn );
	    continue;
	}

	const SceneID sceneid = scn.vwr3d_->sceneID();
	RefMan<visSurvey::Scene> visscene = visServ().getScene( sceneid );
	if ( visscene )
	{
	    if ( scn.vwr3d_->getPolygonSelector() )
		visscene->setPolygonSelector(scn.vwr3d_->getPolygonSelector());
	    if ( scn.vwr3d_->getSceneColTab() )
		visscene->setSceneColTab( scn.vwr3d_->getSceneColTab() );
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

	scn.mdiwin_->setTitle( scenenm );

	scn.vwr3d_->display( true );
	scn.vwr3d_->showRotAxis( true );
	mAttachCB( scn.vwr3d_->viewmodechanged, uiODSceneMgr::viewModeChg );
	mAttachCB( scn.vwr3d_->pageupdown, uiODSceneMgr::pageUpDownPressed );
	scn.mdiwin_->display( true, false );

	treeToBeAdded.trigger( sceneid );
	initTree( scn, vwridx_ );
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
    if ( scenes_.isEmpty() ) return;

    mDynamicCastGet(ui3DViewer*,vwr3d_,cb)
    if ( vwr3d_ ) setToViewMode( vwr3d_->isViewMode() );
}


void uiODSceneMgr::setToViewMode( bool yn )
{
    mDoAllScenes(vwr3d_,setViewMode,yn);
    visServ().setViewMode( yn , false );
    if ( appl_.menuMgrAvailable() )
	appl_.menuMgr().updateViewMode( yn );

    updateStatusBar();
    viewModeChanged.trigger();
}


bool uiODSceneMgr::inViewMode() const
{ return scenes_.isEmpty() ? false : scenes_[0]->vwr3d_->isViewMode(); }


void uiODSceneMgr::setToWorkMode( uiVisPartServer::WorkMode wm )
{
    bool yn = ( wm == uiVisPartServer::View ) ? true : false;

    mDoAllScenes(vwr3d_,setViewMode,yn);
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
	const BinID bid( SI().transform( Coord(xytpos.x,xytpos.y) ) );
	const float zfact = mCast(float,visServ().zFactor());
	const float zval = (float) (zfact * xytpos.z);
	const int nrdec = SI().nrZDecimals()+1; // get from settings
	const BufferString zvalstr = toString( zval, nrdec );
	msg = toUiString("%1    (%2, %3, %4)")
	    .arg( bid.toString() )
	    .arg( mNINT32(xytpos.x) )
	    .arg( mNINT32(xytpos.y) )
	    .arg( zvalstr );
    }

    appl_.statusBar()->message( msg, mPosField );

    const BufferString valstr = visServ().getMousePosVal();
    if ( haspos )
    {
	msg = valstr.isEmpty()
		? uiString::emptyString()
		: tr("Value = %1").arg( valstr );
    }
    else
	msg.setEmpty();

    appl_.statusBar()->message( msg, mValueField );

    msg = haspos
	    ? mToUiStringTodo(visServ().getMousePosString())
	    : uiString::emptyString();
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
    return scenes_.size() ? (int)scenes_[0]->vwr3d_->getStereoType() : 0;
}


void uiODSceneMgr::setStereoType( int type )
{
    if ( scenes_.isEmpty() ) return;

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
    const float stereooffset = scenes_[0]->vwr3d_->getStereoOffset();
    for ( int ids=0; ids<scenes_.size(); ids++ )
    {
	ui3DViewer& vwr3d_ = *scenes_[ids]->vwr3d_;
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


void uiODSceneMgr::setViewSelectMode( SceneID sceneid,
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
    mDoAllScenes(vwr3d_,showRotAxis,act?act->isChecked():false);
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	const OD::Color& col = applMgr().visServer()->getSceneAnnotCol( idx );
	scenes_[idx]->vwr3d_->setAnnotationColor( col );
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
    : uiDialog( p, uiDialog::Setup(tr("Specify snapshot"),
		   tr("Select area to take snapshot"),
				   mODHelpKey(mSnapshotDlgHelpID) ) )
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
    for ( int idx=0; idx<scenes_.size(); idx++ )
	dispids += scenes_[idx]->itemmanager_->getDisplayIds( selectedid,
							      !issolomodeon );

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
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	SceneID sceneid = scenes_[idx]->itemmanager_->sceneID();
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
    for ( int idx=0; idx<scenes_.size(); idx++ )
	vwrs += scenes_[idx]->vwr3d_;
}


const ui3DViewer* uiODSceneMgr::get3DViewer( const SceneID& sceneid ) const
{
    const Scene* scene = getScene( sceneid );
    return scene ? scene->vwr3d_ : 0;
}


ui3DViewer* uiODSceneMgr::get3DViewer( const SceneID& sceneid )
{
    const Scene* scene = getScene( sceneid );
    return scene ? scene->vwr3d_ : 0;
}


uiODTreeTop* uiODSceneMgr::getTreeItemMgr( const uiTreeView* lv ) const
{
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	if ( scenes_[idx]->lv_ == lv )
	    return scenes_[idx]->itemmanager_;
    }

    return nullptr;
}


uiODTreeTop* uiODSceneMgr::getTreeItemMgr( const SceneID& sceneid ) const
{
    const Scene* scene = getScene( sceneid );
    return scene ? scene->itemmanager_ : nullptr;
}


void uiODSceneMgr::translateText()
{
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	scenes_[idx]->itemmanager_->translateText();
    }
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
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	if ( !scenes_[idx] || !scenes_[idx]->itemmanager_ )
	    continue;

	if ( scenenm ==
	   getSceneName(scenes_[idx]->itemmanager_->sceneID()).getFullString() )
	    return scenes_[idx]->itemmanager_->sceneID();
    }

    return SceneID::udf();
}


void uiODSceneMgr::mdiAreaChanged( CallBacker* )
{
    bool visible = false;
    if ( !appl_.isMinimized() )
    {
	for ( int idx=0; idx<scenes_.size(); idx++ )
	{
	    if ( !scenes_[idx]->mdiwin_->isMinimized() )
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
}


void uiODSceneMgr::sceneChanged( SceneID sceneid )
{
    uiODSceneMgr::Scene* scene = getScene( sceneid );
    if ( scene )
    {
	scene->dw_->raise();
	if ( scene->vwr3d_ && appl_.menuMgrAvailable() )
	    appl_.menuMgr().setCameraPixmap(
			scene->vwr3d_->isCameraPerspective() );
    }

    activeSceneChanged.trigger();
}


void uiODSceneMgr::setActiveScene( int idx )
{
    uiODSceneMgr::Scene* scene = scenes_.validIdx(idx) ? scenes_[idx] : nullptr;
    if ( scene )
	setActiveScene( scene->vwr3d_->sceneID() );
}


void uiODSceneMgr::setActiveScene( SceneID sceneid )
{
    const BufferString scenenm = getSceneName( sceneid ).getFullString();
    mdiarea_->setActiveWin( scenenm );
    sceneChanged( sceneid );
}


void uiODSceneMgr::initTree( Scene& scn, int vwridx )
{
    const uiString capt = tr( "Tree scene %1" ).arg( vwridx );
    scn.dw_ = new uiDockWin( &appl_, capt );
    scn.dw_->setMinimumWidth( 200 );
    scn.lv_ = new uiTreeView( scn.dw_, capt.getFullString() );
    scn.dw_->setObject( scn.lv_ );
    uiStringSet labels;
    labels.add( sElements() );
    labels.add( uiStrings::sColor() );
    scn.lv_->addColumns( labels );
    scn.lv_->setFixedColumnWidth( cColorColumn(), 40 );

    scn.itemmanager_ = new uiODTreeTop( scn.vwr3d_, scn.lv_, &applMgr(), tifs_);
    uiODSceneTreeItem* sceneitm =
	new uiODSceneTreeItem( scn.mdiwin_->getTitle(),
			       scn.vwr3d_->sceneID() );
    scn.itemmanager_->addChild( sceneitm, false );

    TypeSet<int> idxs;
    TypeSet<int> placeidxs;

    for ( int idx=0; idx<tifs_->nrFactories(); idx++ )
    {
	OD::Pol2D3D pol2d = tifs_->getPol2D3D( idx );
	if ( SI().survDataType() == OD::Both2DAnd3D ||
	     pol2d == OD::Both2DAnd3D ||
	     pol2d == SI().survDataType() )
	{
	    idxs += idx;
	    placeidxs += tifs_->getPlacementIdx( idx );
	}
    }

    sort_coupled( placeidxs.arr(), idxs.arr(), idxs.size() );

    for ( int idx=0; idx<idxs.size(); idx++ )
    {
	const int fidx = idxs[idx];
	scn.itemmanager_->addChild(
		tifs_->getFactory(fidx)->create(), true );
    }

    scn.lv_->display( true );
    appl_.addDockWindow( *scn.dw_, uiMainWin::Left );
    if ( scenes_.size() > 1 )
    {
	const int sceneidx = scenes_.indexOf( &scn );
	Scene* prevscene = scenes_.validIdx(sceneidx-1) ? scenes_[sceneidx-1]
							: nullptr;
	if ( prevscene && prevscene->dw_ )
	    appl_.tabifyDockWindow( *prevscene->dw_, *scn.dw_ );
    }

    scn.dw_->setVisible( treeShown() );
}


void uiODSceneMgr::updateTrees()
{
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	Scene& scene = *scenes_[idx];
	scene.itemmanager_->updateColumnText( cNameColumn() );
	scene.itemmanager_->updateColumnText( cColorColumn() );
	scene.itemmanager_->updateCheckStatus();
    }
}


void uiODSceneMgr::rebuildTrees()
{
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	Scene& scene = *scenes_[idx];
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
    Scene* scene = getScene( sceneid );
    return scene ? scene->lv_ : nullptr;
}


void uiODSceneMgr::showTree( bool yn )
{
    for ( int idx=0; idx<scenes_.size(); idx++ )
	scenes_[idx]->dw_->setVisible( yn );
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
    const VisID id = visServ().getSelObjectId();
    updateItemToolbar( id );

    if ( id.isValid() )
    {
	resetStatusBar( id );
	//applMgr().modifyColorTable( id );
	if ( !visServ().isOn(id) )
	    visServ().turnOn(id, true, true);
	else if ( scenes_.size() != 1 && visServ().isSoloMode() )
	    visServ().updateDisplay( true, id );
    }

    mDoAllScenes(itemmanager_,updateSelection,id.asInt());
    mDoAllScenes(itemmanager_,updateSelTreeColumnText,cNameColumn());
    mDoAllScenes(itemmanager_,updateSelTreeColumnText,cColorColumn());

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
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	const uiTreeItem* itm = scenes_[idx]->itemmanager_->findChild( str );
	if ( itm )
	    return VisID( itm->selectionKey() );
    }

    return VisID::udf();
}


void uiODSceneMgr::disabRightClick( bool yn )
{
    mDoAllScenes(itemmanager_,disabRightClick,yn);
}


void uiODSceneMgr::disabTrees( bool yn )
{
    const bool wasparalysed = mdiarea_->paralyse( true );

    for ( int idx=0; idx<scenes_.size(); idx++ )
	scenes_[idx]->lv_->setSensitive( !yn );

    mdiarea_->paralyse( wasparalysed );
}


#define mGetOrAskForScene \
    Scene* scene = getScene( scnid ); \
    const SceneID sceneid = scene ? scnid : askSelectScene(); \
    if ( !scene ) \
	scene = getScene( sceneid ); \
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
	const Scene* scene = getScene( sceneid );
	if ( !scene ) return;

	gtLoadedPickSetIDs( *scene, picks, poly );
	return;
    }

    for ( int idx=0; idx<scenes_.size(); idx++ )
	gtLoadedPickSetIDs( *scenes_[idx], picks, poly );

}


void uiODSceneMgr::gtLoadedPickSetIDs( const Scene& scene,
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
	const Scene* scene = getScene( sceneid );
	if ( !scene ) return;
	gtLoadedEMIDs( scene, emids, type );
	return;
    }

    for ( int idx=0; idx<scenes_.size(); idx++ )
	gtLoadedEMIDs( scenes_[idx], emids, type );
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


void uiODSceneMgr::gtLoadedEMIDs( const Scene* scene,
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
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	Scene& scene = *scenes_[idx];
	uiTreeItem* itm = scene.itemmanager_->findChild( displayid.asInt() );
	if ( itm )
	{
	    itm->prepareForShutdown();
	    scene.itemmanager_->removeChild( itm );
	}
    }
}


uiTreeItem* uiODSceneMgr::findSceneItem( const SceneID& sceneid )
{
    for ( const auto* scene : scenes_ )
    {
	uiTreeItem* itm = scene->itemmanager_->findChild( sceneid.asInt() );
	if ( itm )
	    return itm;
    }

    return nullptr;
}


uiTreeItem* uiODSceneMgr::findItem( const VisID& displayid )
{
    for ( const auto* scene : scenes_ )
    {
	uiTreeItem* itm = scene->itemmanager_->findChild( displayid.asInt() );
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
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	Scene& scene = *scenes_[idx];
	if ( !sceneid.isValid() || scene.itemmanager_->sceneID() == sceneid )
	    scene.itemmanager_->findChildren( nm, items );
    }
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


uiODSceneMgr::Scene* uiODSceneMgr::getScene( const SceneID& sceneid )
{
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	uiODSceneMgr::Scene* scn = scenes_[idx];
	if ( scn && scn->itemmanager_ &&
		scn->itemmanager_->sceneID() == sceneid )
	    return scenes_[idx];
    }

    return nullptr;
}


const uiODSceneMgr::Scene* uiODSceneMgr::getScene(
						const SceneID& sceneid ) const
{ return mSelf().getScene( sceneid ); }


void uiODSceneMgr::font3DChanged( CallBacker* )
{
    uiFont& font3d = FontList().get( FontData::key(FontData::Graphics3D) );
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	if ( !scenes_[idx]->vwr3d_ ) continue;

	scenes_[idx]->vwr3d_->setAnnotationFont( font3d.fontData() );

	const SceneID sceneid = scenes_[idx]->vwr3d_->sceneID();
	RefMan<visSurvey::Scene> visscene = visServ().getScene( sceneid );
	if ( visscene )
	    visscene->setAnnotFont( font3d.fontData() );
    }
}


void uiODSceneMgr::showIfMinimized( CallBacker* )
{
    if ( appl_.isMinimized() )
	appl_.show();
}

// uiODSceneMgr::Scene
uiODSceneMgr::Scene::Scene( uiMdiArea* mdiarea )
    : dw_(nullptr)
    , lv_(nullptr)
    , mdiwin_(nullptr)
    , vwr3d_(nullptr)
    , itemmanager_(nullptr)
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


uiODSceneMgr::Scene::~Scene()
{
    delete vwr3d_;
    delete mdiwin_;
    delete itemmanager_;
    delete dw_;
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
