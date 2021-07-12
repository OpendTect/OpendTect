/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Dec 2003
________________________________________________________________________

-*/

#include "od_helpids.h"
#include "uiodscenemgr.h"
#include "scene.xpm"

#include "uiattribpartserv.h"
#include "uiempartserv.h"
#include "uiodapplmgr.h"
#include "uipickpartserv.h"
#include "uivispartserv.h"
#include "uiwellattribpartserv.h"
#include "uisettings.h"
#include "uigeninput.h"

#include "uibuttongroup.h"
#include "uidockwin.h"
#include "uifont.h"
#include "uigeninputdlg.h"
#include "uimdiarea.h"
#include "uimsg.h"
#include "uiodmenumgr.h"
#include "uiodviewer2dmgr.h"
#include "uiprintscenedlg.h"
#include "ui3dviewer.h"
#include "uiscenepropdlg.h"
#include "uistatusbar.h"
#include "uistrings.h"
#include "uitoolbar.h"
#include "uitreeitem.h"
#include "uitreeview.h"
#include "uiviscoltabed.h"
#include "uiwindowgrabber.h"

#include "emfaultstickset.h"
#include "emfault3d.h"
#include "emhorizon2d.h"
#include "emmanager.h"
#include "emhorizon3d.h"
#include "emmarchingcubessurface.h"
#include "emrandomposbody.h"
#include "ioobj.h"
#include "mpeengine.h"
#include "probemanager.h"
#include "probeimpl.h"
#include "randomlineprobe.h"
#include "wellmanager.h"
#include "picksetmanager.h"
#include "ptrman.h"
#include "probeimpl.h"
#include "randomlinegeom.h"
#include "sorting.h"
#include "settingsaccess.h"
#include "survgeom2d.h"
#include "vissurvscene.h"
#include "vissurvobj.h"

// For factories
#include "uiodannottreeitem.h"
#include "uiodbodydisplaytreeitem.h"
#include "uioddatatreeitem.h"
#include "uiodemsurftreeitem.h"
#include "uiodfaulttreeitem.h"
#include "uiodhortreeitem.h"
#include "uiodpicksettreeitem.h"
#include "uiodplanedatatreeitem.h"
#include "uiodpseventstreeitem.h"
#include "uiodrandlinetreeitem.h"
#include "uiodseis2dtreeitem.h"
#include "uiodsceneobjtreeitem.h"
#include "uiodvolumetreeitem.h"
#include "uiodwelltreeitem.h"

#define mPosField	0
#define mValueField	1
#define mNameField	2
#define mStatusField	3

static const int cWSWidth = 600;
static const int cWSHeight = 500;
static const char* sKeyWarnQuadStereo = "Warning.Quad Stereo Viewing";

#define mWSMCB(fn) mCB(this,uiODSceneMgr,fn)
#define mDoAllScenes(memb,fn,arg) \
    for ( int idx=0; idx<viewers_.size(); idx++ ) \
	getSceneByIdx(idx)->memb->fn( arg )


uiODSceneMgr::uiODSceneMgr( uiODMain* a )
    : appl_(*a)
    , mdiarea_(new uiMdiArea(a,"OpendTect work space"))
    , vwridx_(0)
    , tifs_(new uiTreeFactorySet)
    , wingrabber_(new uiWindowGrabber(a))
    , activeSceneChanged(this)
    , sceneClosed(this)
    , treeToBeAdded(this)
    , treeAdded(this)
    , viewModeChanged(this)
    , tiletimer_(new Timer)
{
#   define mAddFact( typ, nr, pol ) \
    tifs_->addFactory( new typ, nr, OD::pol )
    mAddFact( uiODInlineTreeItemFactory,	1000,	Only3D );
    mAddFact( uiODCrosslineTreeItemFactory,	1100,	Only3D );
    mAddFact( uiODZsliceTreeItemFactory,	1200,	Only3D );
    mAddFact( uiODVolumeTreeItemFactory,	1500,	Only3D );
    mAddFact( uiODRandomLineTreeItemFactory,	2000,	Only3D );
    mAddFact( Line2DTreeItemFactory,		3000,	Only2D );
    mAddFact( uiODHorizonTreeItemFactory,	4000,	Both2DAnd3D );
    mAddFact( uiODHorizon2DTreeItemFactory,	4500,	Only2D );
    mAddFact( uiODFaultTreeItemFactory,		5000,	Both2DAnd3D );
    mAddFact( uiODFaultStickSetTreeItemFactory,	5500,	Both2DAnd3D );
    mAddFact( uiODBodyDisplayTreeItemFactory,	6000,	Only3D );
    mAddFact( uiODWellTreeItemFactory,		7000,	Both2DAnd3D );
    mAddFact( uiODPickSetTreeItemFactory,	8000,	Both2DAnd3D );
    mAddFact( uiODPolygonTreeItemFactory,	8500,	Both2DAnd3D );
    mAddFact( uiODPSEventsTreeItemFactory,	9000,	Both2DAnd3D );
    mAddFact( uiODAnnotTreeItemFactory,		10000,	Both2DAnd3D );

    mdiarea_->setPrefWidth( cWSWidth );
    mdiarea_->setPrefHeight( cWSHeight );

    mAttachCB( mdiarea_->windowActivated, uiODSceneMgr::mdiAreaChanged );
    mAttachCB( tiletimer_->tick, uiODSceneMgr::tileTimerCB );

    uiFont& font3d = FontList().get( FontData::key(FontData::Graphics3D) );
    mAttachCB( font3d.changed, uiODSceneMgr::font3DChanged );
}


uiODSceneMgr::~uiODSceneMgr()
{
    detachAllNotifiers();
    cleanUp( false );
    delete tifs_;
    delete mdiarea_;
    deepErase( viewers_ );
    delete wingrabber_;
    delete tiletimer_;
}


void uiODSceneMgr::initMenuMgrDepObjs()
{
    if ( viewers_.isEmpty() )
	addScene(true);
}


void uiODSceneMgr::cleanUp( bool startnew )
{
    mdiarea_->closeAll();
    // closeAll() cascades callbacks which remove the scene from set

    visServ().deleteAllObjects();
    vwridx_ = 0;
    if ( startnew ) addScene(true);
}


uiODScene& uiODSceneMgr::mkNewScene()
{
    uiODScene& scn = *new uiODScene( mdiarea_ );
    scn.mdiwin_->closed().notify( mWSMCB(removeSceneCB) );
    viewers_ += &scn;
    vwridx_++;
    BufferString vwrnm( "Viewer Scene ", vwridx_ );
    scn.vwr3d_->setName( vwrnm );
    return scn;
}


int uiODSceneMgr::addScene( bool maximized, ZAxisTransform* zt,
		const uiString& name )
{
    uiODScene& scn = mkNewScene();
    scn.setZAxisTransform( zt );
    const int sceneid = visServ().addScene();
    scn.setViewerObjID( ViewerObjID::get(sceneid) );
    mDynamicCastGet(visSurvey::Scene*,visscene,visServ().getObject(sceneid));
    if ( visscene && scn.vwr3d_->getPolygonSelector() )
	visscene->setPolygonSelector( scn.vwr3d_->getPolygonSelector() );
    if ( visscene && scn.vwr3d_->getSceneColTab() )
	visscene->setSceneColTab( scn.vwr3d_->getSceneColTab() );
    if ( visscene )
	mAttachCB(
	visscene->sceneboundingboxupdated,uiODSceneMgr::newSceneUpdated );

    scn.vwr3d_->setSceneID( sceneid );
    uiString title =uiStrings::sSceneWithNr(vwridx_);

    scn.mdiwin_->setTitle( title );
    visServ().setUiObjectName( sceneid, title );
    scn.vwr3d_->display( true );
    scn.vwr3d_->setAnnotationFont( visscene ? visscene->getAnnotFont()
					    : FontData() );
    scn.vwr3d_->viewmodechanged.notify( mWSMCB(viewModeChg) );
    scn.vwr3d_->pageupdown.notify( mCB(this,uiODSceneMgr,pageUpDownPressed) );
    scn.mdiwin_->display( true, false, maximized );
    actMode( 0 );
    treeToBeAdded.trigger( sceneid );
    initTree( scn, vwridx_ );
    treeAdded.trigger( sceneid );

    if ( viewers_.size()>1 && viewers_[0] )
    {
	scn.vwr3d_->setStereoType( getSceneByIdx(0)->vwr3d_->getStereoType() );
	scn.vwr3d_->setStereoOffset(
		getSceneByIdx(0)->vwr3d_->getStereoOffset() );
	scn.vwr3d_->showRotAxis( getSceneByIdx(0)->vwr3d_->rotAxisShown() );
	if ( !getSceneByIdx(0)->vwr3d_->isCameraPerspective() )
	    scn.vwr3d_->toggleCameraType();
	visServ().displaySceneColorbar( visServ().sceneColorbarDisplayed() );
    }
    else if ( viewers_[0] )
    {
	const bool isperspective =
	    getSceneByIdx(0)->vwr3d_->isCameraPerspective();
	if ( appl_.menuMgrAvailable() )
	{
	    appl_.menuMgr().setCameraPixmap( isperspective );
	    appl_.menuMgr().updateAxisMode( true );
	}
	scn.vwr3d_->showRotAxis( true );
    }

    if ( !name.isEmpty() )
	setSceneName( sceneid, name );

    visServ().setZAxisTransform( sceneid, zt, 0 );
    scn.vwr3d_->updateZDomainInfo();

    visServ().turnSelectionModeOn( visServ().isSelectionModeOn() );
    return sceneid;
}


void uiODSceneMgr::newSceneUpdated( CallBacker* cb )
{
    if ( viewers_.size() >0 && getSceneByIdx(viewers_.size()-1)->vwr3d_ )
    {
	getSceneByIdx(viewers_.size()-1)->vwr3d_->viewAll( false );
	tiletimer_->start( 10,true );

	visBase::DataObject* obj = visBase::DM().getObject(
		getSceneByIdx(viewers_.size()-1)->vwr3d_->sceneID() );

	mDynamicCastGet( visSurvey::Scene*,visscene,obj );

	mDetachCB(
	    visscene->sceneboundingboxupdated,uiODSceneMgr::newSceneUpdated );
    }
}


void uiODSceneMgr::tileTimerCB( CallBacker* )
{
    if ( viewers_.size() > 1 )
	tile();
}


void uiODSceneMgr::removeScene( uiODScene& scene )
{
    appl_.removeDockWindow( scene.dw_ );

    if ( scene.itemmanager_ )
    {
	visBase::DataObject* obj =
	    visBase::DM().getObject( scene.itemmanager_->sceneID() );
	mDynamicCastGet( visSurvey::Scene*,visscene,obj );
	mDetachCB(
	    visscene->sceneboundingboxupdated,uiODSceneMgr::newSceneUpdated );
	scene.itemmanager_->askContinueAndSaveIfNeeded( false );
	visscene->setMoreObjectsToDoHint( true );
	scene.itemmanager_->prepareForShutdown();
	visServ().removeScene( scene.itemmanager_->sceneID() );
	sceneClosed.trigger( scene.itemmanager_->sceneID() );
    }

    scene.mdiwin_->closed().remove( mWSMCB(removeSceneCB) );
    viewers_ -= &scene;
    delete &scene;
}


void uiODSceneMgr::removeSceneCB( CallBacker* cb )
{
    mDynamicCastGet(uiGroupObj*,grp,cb)
    if ( !grp ) return;
    int idxnr = -1;
    for ( int idx=0; idx<viewers_.size(); idx++ )
    {
	if ( grp == getSceneByIdx(idx)->mdiwin_->mainObject() )
	{
	    idxnr = idx;
	    break;
	}
    }
    if ( idxnr < 0 ) return;

    uiODScene* scene = getSceneByIdx(idxnr);
    removeScene( *scene );
}


void uiODSceneMgr::setSceneName( int sceneid, const uiString& nm )
{
    visServ().setUiObjectName( sceneid, nm );
    uiODScene* scene = getScene( sceneid );
    if ( !scene ) return;

    scene->mdiwin_->setTitle( nm );
    scene->dw_->setDockName( nm );
    uiTreeItem* itm = scene->itemmanager_->findChild( sceneid );
    if ( itm )
	itm->updateColumnText( uiODSceneMgr::cNameColumn() );
}


uiString uiODSceneMgr::getSceneName( int sceneid ) const
{
    return toUiString( const_cast<uiODSceneMgr*>(this)->visServ()
			.getObjectName(sceneid) ); }

const ZDomain::Info* uiODSceneMgr::zDomainInfo( int sceneid ) const
{ return const_cast<uiODSceneMgr*>(this)->visServ().zDomainInfo( sceneid ); }


void uiODSceneMgr::getScenePars( IOPar& iopar )
{
    for ( int idx=0; idx<viewers_.size(); idx++ )
    {
	IOPar iop;
	getSceneByIdx(idx)->vwr3d_->fillPar( iop );
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

	uiODScene& scn = mkNewScene();
	if ( !scn.vwr3d_->usePar(*scenepar) )
	{
	    removeScene( scn );
	    continue;
	}

	const int sceneid = scn.vwr3d_->sceneID();
	visBase::DataObject* obj =
	    visBase::DM().getObject( sceneid );
	mDynamicCastGet( visSurvey::Scene*,visscene,obj );

	if ( visscene )
	{
	    if ( scn.vwr3d_->getPolygonSelector() )
		visscene->setPolygonSelector(scn.vwr3d_->getPolygonSelector());
	    if ( scn.vwr3d_->getSceneColTab() )
		visscene->setSceneColTab( scn.vwr3d_->getSceneColTab() );
	}

	visServ().displaySceneColorbar( visServ().sceneColorbarDisplayed() );
	visServ().turnSelectionModeOn( false );

	const uiString title = uiStrings::sSceneWithNr(vwridx_);
	scn.mdiwin_->setTitle( title );
	visServ().setUiObjectName( sceneid, title );

	scn.vwr3d_->display( true );
	scn.vwr3d_->showRotAxis( true );
	scn.vwr3d_->viewmodechanged.notify( mWSMCB(viewModeChg) );
	scn.vwr3d_->pageupdown.notify(mCB(this,uiODSceneMgr,pageUpDownPressed));
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
	gUiMsg().error( tr("No scenes available") );
	return;
    }

    int curvwridx = 0;
    if ( vwrs.size() > 1 )
    {
	const int sceneid = askSelectScene();
	const ui3DViewer* vwr = get3DViewer( sceneid );
	if ( !vwr ) return;

	curvwridx = vwrs.indexOf( vwr );
    }

    uiScenePropertyDlg dlg( &appl_, vwrs, curvwridx );
    dlg.go();
}


void uiODSceneMgr::viewModeChg( CallBacker* cb )
{
    if ( viewers_.isEmpty() ) return;

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
{ return viewers_.isEmpty() ? false : getSceneByIdx(0)->vwr3d_->isViewMode(); }


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


void uiODSceneMgr::resetStatusBar( int id )
{
    appl_.statusBar()->message( uiString::empty(), mPosField );
    appl_.statusBar()->message( uiString::empty(), mValueField );
    appl_.statusBar()->message(	toUiString(visServ().getInteractionMsg(id)),
				mNameField );
    appl_.statusBar()->message( uiString::empty(), mStatusField );
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
    const BinID bid( SI().transform( xytpos.getXY() ) );
    const float zfact = mCast(float,visServ().zFactor());
    float zval = (float) (zfact * xytpos.z_);
    if ( zfact>100 || zval>10 ) zval = mCast( float, mNINT32(zval) );
    msg = toUiString("%1    (%2, %3, %4)")
	.arg( bid.toString() )
	.arg( mNINT32(xytpos.x_) )
	.arg( mNINT32(xytpos.y_) )
	.arg( zval );
    }

    appl_.statusBar()->message( msg, mPosField );

    const BufferString valstr = visServ().getMousePosVal();
    if ( haspos )
    {
    msg = valstr.isEmpty()
	    ? uiString::empty()
	    : tr("Value = %1").arg( valstr );
    }
    else
    msg.setEmpty();

    appl_.statusBar()->message( msg, mValueField );

    msg = haspos
	    ? toUiString(visServ().getMousePosString())
	    : uiString::empty();
    if ( msg.isEmpty() )
    {
	const int selid = visServ().getSelObjectId();
    msg = toUiString(visServ().getInteractionMsg( selid ) );
    }
    appl_.statusBar()->message( msg, mNameField );

    uiString bsmsg;
    visServ().getPickingMessage( bsmsg );
    appl_.statusBar()->message( bsmsg, mStatusField );

    appl_.statusBar()->setBGColor( mStatusField, visServ().isPicking() ?
	    Color(255,0,0) : appl_.statusBar()->getBGColor(mPosField) );
}


int uiODSceneMgr::getStereoType() const
{
    return viewers_.size() ? (int)getSceneByIdx(0)->vwr3d_->getStereoType() : 0;
}


void uiODSceneMgr::setStereoType( int type )
{
    if ( viewers_.isEmpty() ) return;

    if ( type == 2 )
    {
	bool dontwarn = Settings::common().isFalse( sKeyWarnQuadStereo );
	if ( !dontwarn )
	{
	    const bool wantgoon = gUiMsg().askGoOn(
		tr("Please note that Quad buffered stereo viewing requires "
		    "suitable, well set up hardware."
		    "\nWe do not have such hardware, so we can only hope the "
		    "OpenSceneGraph people got it right. If so ... please "
		    "share your experiences ..."
		    "\n\nDo you wish to continue?"), true, &dontwarn );
	    if ( dontwarn )
	    {
		Settings::common().setYN( sKeyWarnQuadStereo, false );
		Settings::common().write();
	    }
	    if ( !wantgoon )
		return;
	}
    }

    ui3DViewer::StereoType stereotype = (ui3DViewer::StereoType)type;
    const float stereooffset = getSceneByIdx(0)->vwr3d_->getStereoOffset();
    for ( int ids=0; ids<viewers_.size(); ids++ )
    {
	ui3DViewer& vwr3d_ = *getSceneByIdx(ids)->vwr3d_;
	if ( !vwr3d_.setStereoType(stereotype) )
	{
	    gUiMsg().error( tr("No support for this type of stereo rendering"));
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
    const int nrgrps = viewers_.size();
    if ( nrgrps == 1 && viewers_[0] )
	getSceneByIdx(0)->mdiwin_->display( true, false, true );
    else if ( nrgrps>1 && viewers_[0] )
	tile();
}


void uiODSceneMgr::toHomePos( CallBacker* )
{ mDoAllScenes(vwr3d_,toHomePos,); }
void uiODSceneMgr::saveHomePos( CallBacker* )
{ mDoAllScenes(vwr3d_,saveHomePos,); }
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
    mDoAllScenes(vwr3d_,viewPlane,(ui3DViewer::PlaneType)md);
}


void uiODSceneMgr::showRotAxis( CallBacker* cb )
{
    mDynamicCastGet(const uiAction*,act,cb)
    mDoAllScenes(vwr3d_,showRotAxis,act?act->isChecked():false);
    for ( int idx=0; idx<viewers_.size(); idx++ )
    {
	const Color& col = applMgr().visServer()->getSceneAnnotCol( idx );
	getSceneByIdx(idx)->vwr3d_->setAnnotationColor( col );
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
    new uiRadioButton( butgrp_, uiStrings::sWindow() );
    new uiRadioButton( butgrp_, uiStrings::sDesktop() );
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

    TypeSet< TypeSet<int> > dispids;
    int selectedid = -1;

    const bool issolomodeon = appl_.menuMgr().isSoloModeOn();
    for ( int idx=0; idx<viewers_.size(); idx++ )
	dispids += getSceneByIdx(idx)->itemmanager_->getDisplayIds( selectedid,
							      !issolomodeon );

    visServ().setSoloMode( issolomodeon, dispids, selectedid );
    updateSelectedTreeItem();
}


void uiODSceneMgr::switchCameraType( CallBacker* )
{
    ObjectSet<ui3DViewer> vwrs;
    get3DViewers( vwrs );
    if ( vwrs.isEmpty() ) return;
    mDoAllScenes(vwr3d_,toggleCameraType,);
    const bool isperspective = vwrs[0]->isCameraPerspective();
    if ( appl_.menuMgrAvailable() )
	appl_.menuMgr().setCameraPixmap( isperspective );
}


int uiODSceneMgr::askSelectScene( const char* zdomkeyfilter ) const
{
    uiStringSet scenenms; TypeSet<int> sceneids;
    const FixedString zdomkey( zdomkeyfilter );
    for ( int idx=0; idx<viewers_.size(); idx++ )
    {
	int sceneid = getSceneByIdx(idx)->itemmanager_->sceneID();
	if ( !zdomkey.isEmpty() )
	{
	    const ZDomain::Info* zinfo = zDomainInfo( sceneid );
	    if ( !zinfo || zdomkey != zinfo->key() )
		continue;
	}

	sceneids += sceneid;
	scenenms.add( getSceneName(sceneid) );
    }

    if ( sceneids.size() < 2 )
	return sceneids.isEmpty() ? -1 : sceneids[0];

    StringListInpSpec* inpspec = new StringListInpSpec( scenenms );
    uiGenInputDlg dlg( &appl_, tr("Choose scene"), mNoDlgTitle, inpspec );
    const int selidx = dlg.go() ? dlg.getIntValue() : -1;
    return sceneids.validIdx(selidx) ? sceneids[selidx] : -1;
}


void uiODSceneMgr::get3DViewers( ObjectSet<ui3DViewer>& vwrs )
{
    vwrs.erase();
    for ( int idx=0; idx<viewers_.size(); idx++ )
	vwrs += getSceneByIdx(idx)->vwr3d_;
}


const ui3DViewer* uiODSceneMgr::get3DViewer( int sceneid ) const
{
    const uiODScene* scene = getScene( sceneid );
    return scene ? scene->vwr3d_ : 0;
}


ui3DViewer* uiODSceneMgr::get3DViewer( int sceneid )
{
    const uiODScene* scene = getScene( sceneid );
    return scene ? scene->vwr3d_ : 0;
}


uiODSceneTreeTop* uiODSceneMgr::getTreeItemMgr( const uiTreeView* lv ) const
{
    for ( int idx=0; idx<viewers_.size(); idx++ )
    {
	if ( getSceneByIdx(idx)->lv_ == lv )
	    return getSceneByIdx(idx)->itemmanager_;
    }

    return 0;
}


void uiODSceneMgr::translateText()
{
    for ( int idx=0; idx<viewers_.size(); idx++ )
    {
	getSceneByIdx(idx)->itemmanager_->translateText();
    }
}


void uiODSceneMgr::getSceneNames( uiStringSet& nms, int& active ) const
{
    mdiarea_->getWindowNames( nms );
    const BufferString activenm = mdiarea_->getActiveWin();
    active = -1;
    for ( int idx=0; idx<nms.size(); idx++ )
    {
	if ( activenm == toString(nms[idx]) )
	    { active = idx; break; }
    }
}


void uiODSceneMgr::getActiveSceneName( BufferString& nm ) const
{
    nm = mdiarea_->getActiveWin();
}


int uiODSceneMgr::getActiveSceneID() const
{
    const BufferString scenenm = mdiarea_->getActiveWin();
    for ( int idx=0; idx<viewers_.size(); idx++ )
    {
	const uiODScene* idxscene = getSceneByIdx( idx );
	if ( !viewers_[idx] || !idxscene->itemmanager_ )
	    continue;

	if ( scenenm ==
	       toString(getSceneName(idxscene->itemmanager_->sceneID())) )
	    return idxscene->itemmanager_->sceneID();
    }

    return -1;
}


void uiODSceneMgr::mdiAreaChanged( CallBacker* )
{
//    const bool wasparalysed = mdiarea_->paralyse( true );
    if ( appl_.menuMgrAvailable() )
	appl_.menuMgr().updateSceneMenu();
//    mdiarea_->paralyse( wasparalysed );
    activeSceneChanged.trigger();
}


void uiODSceneMgr::setActiveScene( int idx )
{
    uiStringSet nms;
    int act;
    getSceneNames( nms, act );

    mdiarea_->setActiveWin( toString(nms[idx]) );
    activeSceneChanged.trigger();
}



void uiODSceneMgr::initTree( uiODScene& scn, int vwridx )
{
    const uiString capt = tr("Tree scene").withNumber( vwridx );
    scn.dw_ = new uiDockWin( &appl_, capt );
    scn.dw_->setMinimumWidth( 200 );
    scn.lv_ = new uiTreeView( scn.dw_, toString(capt) );
    scn.dw_->setObject( scn.lv_ );
    uiStringSet labels;
    labels.add( sElements() );
    labels.add( uiStrings::sColor() );
    scn.lv_->addColumns( labels );
    scn.lv_->setFixedColumnWidth( cColorColumn(), 40 );

    scn.itemmanager_ =
	new uiODSceneTreeTop( scn.lv_, tifs_, scn.vwr3d_->sceneID() );
    uiODSceneObjTreeItem* sceneitm =
	new uiODSceneObjTreeItem( scn.mdiwin_->getTitle(),
				  scn.vwr3d_->sceneID() );
    scn.itemmanager_->addChild( sceneitm, false );

    TypeSet<int> idxs;
    TypeSet<int> placeidxs;

    for ( int idx=0; idx<tifs_->nrFactories(); idx++ )
    {
	const OD::Pol2D3D pol2d3d = (OD::Pol2D3D)tifs_->getPol2D3D( idx );
	if ( SI().survDataType() == OD::Both2DAnd3D ||
	     pol2d3d == OD::Both2DAnd3D ||
	     pol2d3d == SI().survDataType() )
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
    scn.dw_->display( true );
}


void uiODSceneMgr::updateTrees()
{
    for ( int idx=0; idx<viewers_.size(); idx++ )
    {
	uiODScene& scene = *getSceneByIdx( idx );
	scene.itemmanager_->updateColumnText( cNameColumn() );
	scene.itemmanager_->updateColumnText( cColorColumn() );
	scene.itemmanager_->updateCheckStatus();
    }
}


void uiODSceneMgr::rebuildTrees()
{
    for ( int idx=0; idx<viewers_.size(); idx++ )
    {
	uiODScene& scene = *getSceneByIdx( idx );
	const int sceneid = scene.vwr3d_->sceneID();
	TypeSet<int> visids; visServ().getChildIds( sceneid, visids );

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


uiTreeView* uiODSceneMgr::getTree( int sceneid )
{
    uiODScene* scene = getScene( sceneid );
    return scene ? scene->lv_ : 0;
}


void uiODSceneMgr::setItemInfo( int id )
{
    mDoAllScenes(itemmanager_,updateColumnText,cNameColumn());
    mDoAllScenes(itemmanager_,updateColumnText,cColorColumn());
    resetStatusBar( id );
}


void uiODSceneMgr::updateItemToolbar( int id )
{
    visServ().getToolBarHandler()->setMenuID( id );
    visServ().getToolBarHandler()->executeMenu(); // addButtons
}


void uiODSceneMgr::updateSelectedTreeItem()
{
    const int id = visServ().getSelObjectId();
    updateItemToolbar( id );

    if ( id != -1 )
    {
	resetStatusBar( id );
	if ( !visServ().isOn(id) )
	    visServ().turnOn( id, true, true );
	else if ( viewers_.size() != 1 && visServ().isSoloMode() )
	    visServ().updateDisplay( true, id );
    }

    mDoAllScenes(itemmanager_,updateSelection,id);
    mDoAllScenes(itemmanager_,updateSelTreeColumnText,cNameColumn());
    mDoAllScenes(itemmanager_,updateSelTreeColumnText,cColorColumn());

    if ( !applMgr().attrServer() )
	return;

    bool found = applMgr().attrServer()->attrSetEditorActive();
    bool gotoact = false;
    for ( int idx=0; idx<viewers_.size(); idx++ )
    {
	uiODScene& scene = *getSceneByIdx( idx );
	if ( !found )
	{
	    mDynamicCastGet( const uiODDisplayTreeItem*, treeitem,
			     scene.itemmanager_->findChild(id) );
	    if ( treeitem )
	    {
		gotoact = treeitem->actModeWhenSelected();
		found = true;
	    }
	}
    }

    if ( gotoact && !applMgr().attrServer()->attrSetEditorActive() )
	actMode( 0 );
}


int uiODSceneMgr::getIDFromName( const char* str ) const
{
    for ( int idx=0; idx<viewers_.size(); idx++ )
    {
	const uiTreeItem* itm =
	    getSceneByIdx(idx)->itemmanager_->findChild( str );
	if ( itm ) return itm->selectionKey();
    }

    return -1;
}


void uiODSceneMgr::disabRightClick( bool yn )
{
    mDoAllScenes(itemmanager_,disabRightClick,yn);
}


void uiODSceneMgr::disabTrees( bool yn )
{
    const bool wasparalysed = mdiarea_->paralyse( true );

    for ( int idx=0; idx<viewers_.size(); idx++ )
	getSceneByIdx(idx)->lv_->setSensitive( !yn );

    mdiarea_->paralyse( wasparalysed );
}


#define mGetOrAskForScene \
    uiODScene* scene = getScene( sceneid ); \
    if ( !scene ) \
    { \
	sceneid = askSelectScene(); \
	scene = getScene( sceneid ); \
    } \
    if ( !scene ) return -1;

int uiODSceneMgr::addWellItem( const DBKey& dbky, int sceneid )
{
    mGetOrAskForScene

    if ( !scene || !Well::MGR().isValidID(dbky) )
	return -1;

    uiODDisplayTreeItem* itm = new uiODWellTreeItem( dbky );
    scene->itemmanager_->addChild( itm, false );
    return itm->displayID();
}


void uiODSceneMgr::getLoadedPickSetIDs( DBKeySet& picks, bool poly,
					int sceneid ) const
{
    if ( sceneid>=0 )
    {
	const uiODScene* scene = getScene( sceneid );
	if ( !scene ) return;

	gtLoadedPickSetIDs( *scene, picks, poly );
	return;
    }

    for ( int idx=0; idx<viewers_.size(); idx++ )
	gtLoadedPickSetIDs( *getSceneByIdx(idx), picks, poly );

}


void uiODSceneMgr::gtLoadedPickSetIDs( const uiODScene& scene,
	DBKeySet& picks, bool poly ) const
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
	DBKeySet& picks, bool poly ) const
{
    for ( int chidx=0; chidx<topitm.nrChildren(); chidx++ )
    {
	const uiTreeItem* chlditm = topitm.getChild( chidx );
	DBKey setid;
	if ( poly )
	{
	    mDynamicCastGet(const uiODPolygonTreeItem*,polyitem,chlditm)
	    if ( !polyitem )
		continue;
	    setid = Pick::SetMGR().getID( polyitem->getSet() );
	}
	else
	{
	    mDynamicCastGet(const uiODPickSetTreeItem*,pickitem,chlditm)
	    if ( !pickitem )
		continue;
	    setid = Pick::SetMGR().getID( pickitem->getSet() );
	}
	if ( setid.isValid() )
	    picks.addIfNew( setid );
    }
}


void uiODSceneMgr::getLoadedEMIDs( DBKeySet& emids, const char* type,
				   int sceneid ) const
{
    if ( sceneid>=0 )
    {
	const uiODScene* scene = getScene( sceneid );
	if ( !scene ) return;
	gtLoadedEMIDs( scene, emids, type );
	return;
    }

    for ( int idx=0; idx<viewers_.size(); idx++ )
	gtLoadedEMIDs( getSceneByIdx(idx), emids, type );
}


void uiODSceneMgr::gtLoadedEMIDs( const uiTreeItem* topitm, DBKeySet& emids,
				  const char* type ) const
{
    for ( int chidx=0; chidx<topitm->nrChildren(); chidx++ )
    {
	const uiTreeItem* chlditm = topitm->getChild( chidx );
	mDynamicCastGet(const uiODEarthModelSurfaceTreeItem*,emtreeitem,chlditm)
	mDynamicCastGet(const uiODFaultTreeItem*,flttreeitem,chlditm)
	mDynamicCastGet(const uiODFaultStickSetTreeItem*,fsstreeitem,chlditm)
	if ( !emtreeitem && !flttreeitem && !fsstreeitem )
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
    }
}


void uiODSceneMgr::gtLoadedEMIDs( const uiODScene* scene, DBKeySet& emids,
				  const char* type ) const
{
    for ( int chidx=0; chidx<scene->itemmanager_->nrChildren(); chidx++ )
    {
	const uiTreeItem* chlditm = scene->itemmanager_->getChild( chidx );
	gtLoadedEMIDs( chlditm, emids, type );
    }
}


int uiODSceneMgr::addEMItem( const DBKey& emid, int sceneid )
{
    mGetOrAskForScene;

    RefMan<EM::Object> obj = EM::MGR().getObject( emid );
    if ( !obj ) return -1;

    FixedString type = obj->getTypeStr();
    uiODDisplayTreeItem* itm;
    if ( type==EM::Horizon3D::typeStr() )
	itm = new uiODHorizonTreeItem(emid,false,false);
    else if ( type==EM::Horizon2D::typeStr() )
	itm = new uiODHorizon2DTreeItem(emid);
    else if ( type==EM::Fault3D::typeStr() )
	itm = new uiODFaultTreeItem(emid);
    else if ( type==EM::FaultStickSet::typeStr() )
	itm = new uiODFaultStickSetTreeItem(emid);
    else if ( type==EM::RandomPosBody::typeStr() )
	itm = new uiODBodyDisplayTreeItem(emid);
    else if ( type==EM::MarchingCubesSurface::typeStr() )
	itm = new uiODBodyDisplayTreeItem(emid);
    else
	return -1;

    scene->itemmanager_->addChild( itm, false );
    return itm->displayID();
}


int uiODSceneMgr::addPickSetItem( const DBKey& setid, int sceneid )
{
    RefMan<Pick::Set> ps = Pick::SetMGR().fetchForEdit( setid );
    if ( !ps )
	return -1;

    return addPickSetItem( *ps, sceneid );
}


int uiODSceneMgr::addPickSetItem( Pick::Set& ps, int sceneid )
{
    mGetOrAskForScene

    uiODDisplayTreeItem* itm = 0;
    if ( ps.isPolygon() )
	itm = new uiODPolygonTreeItem( -1, ps );
    else
	itm = new uiODPickSetTreeItem( -1, ps );

    scene->itemmanager_->addChild( itm, false );
    return itm->displayID();
}


int uiODSceneMgr::addRandomLineItem( int rlid, int sceneid )
{
    mGetOrAskForScene

    RandomLineProbe* rdlprobe = new RandomLineProbe( rlid );
    SilentTaskRunnerProvider trprov;
    if ( !ProbeMGR().store(*rdlprobe,trprov).isOK() )
	return -1;

    uiODRandomLineTreeItem* itm = new uiODRandomLineTreeItem( *rdlprobe );
    scene->itemmanager_->addChild( itm, false );
    return itm->displayID();
}


int uiODSceneMgr::add2DLineItem( Pos::GeomID geomid, int sceneid )
{
    mGetOrAskForScene

    Line2DProbe* line2dprobe = new Line2DProbe( geomid );
    SilentTaskRunnerProvider trprov;
    if ( !ProbeMGR().store(*line2dprobe, trprov).isOK() )
	return -1;

    uiOD2DLineTreeItem* itm = new uiOD2DLineTreeItem( *line2dprobe );
    scene->itemmanager_->addChild( itm, false );
    return itm->displayID();
}


int uiODSceneMgr::add2DLineItem( const DBKey& dbky , int sceneid )
{
    mGetOrAskForScene

    const auto geomid = geomIDOf( dbky );
    const auto& geom2d = SurvGeom::get2D( geomid );
    if ( geom2d.isEmpty() )
	return -1;

    Line2DProbe* line2dprobe = new Line2DProbe( geomid );
    SilentTaskRunnerProvider trprov;
    if ( !ProbeMGR().store(*line2dprobe,trprov).isOK() )
	return -1;

    uiOD2DLineTreeItem* itm = new uiOD2DLineTreeItem( *line2dprobe );
    scene->itemmanager_->addChild( itm, false );
    return itm->displayID();
}


int uiODSceneMgr::addInlCrlItem( OD::SliceType st, int nr, int sceneid )
{
    mGetOrAskForScene
    uiODPlaneDataTreeItem* itm = 0;
    TrcKeyZSampling tkzs( OD::UsrWork );
    SilentTaskRunnerProvider trprov;
    if ( st == OD::InlineSlice )
    {
	InlineProbe* newprobe = new InlineProbe();
	tkzs.hsamp_.setInlRange( Interval<int>(nr,nr) );
	newprobe->setPos( tkzs );
	if ( !ProbeMGR().store(*newprobe,trprov).isOK() )
	    return -1;

	itm = new uiODInlineTreeItem( *newprobe );
    }
    else if ( st == OD::CrosslineSlice )
    {
	CrosslineProbe* newprobe = new CrosslineProbe();
	tkzs.hsamp_.setCrlRange( Interval<int>(nr,nr) );
	newprobe->setPos( tkzs );
	if ( !ProbeMGR().store(*newprobe,trprov).isOK() )
	    return -1;

	itm = new uiODCrosslineTreeItem( *newprobe );
    }
    else
	return -1;

    if ( !scene->itemmanager_->addChild(itm,false) )
	return -1;

    return itm->displayID();
}


int uiODSceneMgr::addZSliceItem( float z, int sceneid )
{
    mGetOrAskForScene

    ZSliceProbe* newprobe = new ZSliceProbe();
    TrcKeyZSampling tkzs( OD::UsrWork );
    tkzs.zsamp_.set( z, z, SI().zStep() );
    newprobe->setPos( tkzs );
    SilentTaskRunnerProvider trprov;
    if ( !ProbeMGR().store(*newprobe,trprov).isOK() )
	return -1;

    uiODZsliceTreeItem* itm = new uiODZsliceTreeItem( *newprobe );
    if ( !scene->itemmanager_->addChild(itm,false) )
	return -1;

    return itm->displayID();
}


int uiODSceneMgr::addDisplayTreeItem( uiODDisplayTreeItem* itm, int sceneid )
{
    mGetOrAskForScene
    if ( !scene ) return -1;
    scene->itemmanager_->addChild( itm, false );
    return itm->displayID();
}


void uiODSceneMgr::removeTreeItem( int displayid )
{
    for ( int idx=0; idx<viewers_.size(); idx++ )
    {
	uiODScene& scene = *getSceneByIdx( idx );
	uiTreeItem* itm = scene.itemmanager_->findChild( displayid );
	if ( itm )
	{
	    itm->prepareForShutdown();
	    scene.itemmanager_->removeChild( itm );
	}
    }
}


uiTreeItem* uiODSceneMgr::findItem( int displayid )
{
    for ( int idx=0; idx<viewers_.size(); idx++ )
    {
	uiODScene& scene = *getSceneByIdx( idx );
	uiTreeItem* itm = scene.itemmanager_->findChild( displayid );
	if ( itm ) return itm;
    }

    return 0;
}


void uiODSceneMgr::findItems( const char* nm, ObjectSet<uiTreeItem>& items )
{
    deepErase( items );
    for ( int idx=0; idx<viewers_.size(); idx++ )
    {
	uiODScene& scene = *getSceneByIdx( idx );
	scene.itemmanager_->findChildren( nm, items );
    }
}


void uiODSceneMgr::doDirectionalLight(CallBacker*)
{
    visServ().setDirectionalLight();
}


uiODScene* uiODSceneMgr::getSceneByIdx( int idx )
{
    if ( !viewers_.validIdx(idx) )
	return 0;

    mDynamicCastGet(uiODScene*,scene,viewers_[idx]);
    return scene;
}


const uiODScene* uiODSceneMgr::getSceneByIdx( int idx ) const
{ return const_cast<uiODSceneMgr*>(this)->getSceneByIdx( idx ); }


uiODScene* uiODSceneMgr::getScene( int sceneid )
{
    for ( int idx=0; idx<viewers_.size(); idx++ )
    {
	uiODScene* scn = getSceneByIdx( idx );
	if ( scn && scn->itemmanager_ &&
		scn->itemmanager_->sceneID() == sceneid )
	    return scn;
    }

    return 0;
}


const uiODScene* uiODSceneMgr::getScene( int sceneid ) const
{ return const_cast<uiODSceneMgr*>(this)->getScene( sceneid ); }


void uiODSceneMgr::font3DChanged( CallBacker* )
{
    uiFont& font3d = FontList().get( FontData::key(FontData::Graphics3D) );
    for ( int idx=0; idx<viewers_.size(); idx++ )
    {
	uiODScene* uiscene = getSceneByIdx( idx );
	if ( !uiscene || !uiscene->vwr3d_ ) continue;

	uiscene->vwr3d_->setAnnotationFont( font3d.fontData() );

	const int sceneid = uiscene->vwr3d_->sceneID();
	mDynamicCastGet(visSurvey::Scene*,visscene,
			visBase::DM().getObject(sceneid))
	if ( !visscene ) continue;

	visscene->setAnnotFont( font3d.fontData() );
    }
}


// uiODScene
uiODScene::uiODScene( uiMdiArea* mdiarea )
	: lv_(0)
	, dw_(0)
	, mdiwin_(0)
	, vwr3d_(0)
	, itemmanager_(0)
{
    if ( !mdiarea ) return;

    mdiwin_ = new uiMdiAreaWindow( *mdiarea, toUiString("MDI Area Window") );
    mdiwin_->setIcon( scene_xpm_data );
    vwr3d_ = new ui3DViewer( mdiwin_, true );
    vwr3d_->setPrefWidth( 400 );
    vwr3d_->setPrefHeight( 400 );
    mdiarea->addWindow( mdiwin_ );
    viewerobjid_ = ViewerObjID::get( vwr3d_->sceneID() );
}


uiODScene::~uiODScene()
{
    delete vwr3d_;
    delete mdiwin_;
    delete itemmanager_;
    delete dw_;
}
