/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodemsurftreeitem.h"

#include "datapointset.h"
#include "datacoldef.h"
#include "emhorizon3d.h"
#include "emhorizonztransform.h"
#include "emioobjinfo.h"
#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "ioman.h"
#include "ioobj.h"
#include "mpeengine.h"
#include "posvecdataset.h"
#include "survinfo.h"
#include "threadwork.h"
#include "timefun.h"

#include "uiattribpartserv.h"
#include "uiempartserv.h"
#include "uiemattribpartserv.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uimpepartserv.h"
#include "uimsg.h"
#include "uinotsaveddlg.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uishortcutsmgr.h"
#include "uistrings.h"
#include "uiviscoltabed.h"
#include "uivisemobj.h"
#include "uivispartserv.h"
#include "visemobjdisplay.h"
#include "vishorizondisplay.h"
#include "vishorizonsection.h"

uiODDataTreeItem* uiODEarthModelSurfaceTreeItem::createAttribItem(
					const Attrib::SelSpec* as ) const
{
    const char* parenttype = typeid(*this).name();
    uiODDataTreeItem* res = as
	? uiODDataTreeItem::factory().create( 0, *as, parenttype, false) : 0;
    if ( !res ) res = new uiODEarthModelSurfaceDataTreeItem( emid_, uivisemobj_,
							     parenttype );
    return res;
}


uiODEarthModelSurfaceTreeItem::uiODEarthModelSurfaceTreeItem(
						const EM::ObjectID& nemid )
    : uiODDisplayTreeItem()
    , emid_(nemid)
    , uivisemobj_(0)
    , createflatscenemnuitem_(tr("Create Flattened Scene"))
    , savemnuitem_(uiStrings::sSave(),-800)
    , saveasmnuitem_(m3Dots(uiStrings::sSaveAs()),-850)
    , enabletrackingmnuitem_(tr("Enable Tracking"))
    , changesetupmnuitem_(m3Dots(tr("Change Settings")))
    , reloadmnuitem_(uiStrings::sReload(),-750)
    , trackmenuitem_(uiStrings::sTracking())
    , starttrackmnuitem_(m3Dots(tr("Start Tracking")))
    , istrackingallowed_(true)
{
    savemnuitem_.iconfnm = "save";
    saveasmnuitem_.iconfnm = "saveas";
    enabletrackingmnuitem_.checkable = true;
    changesetupmnuitem_.iconfnm = "seedpicksettings";
    NotSavedPrompter::NSP().promptSaving.notify(
	    mCB(this,uiODEarthModelSurfaceTreeItem,askSaveCB));
}


uiODEarthModelSurfaceTreeItem::~uiODEarthModelSurfaceTreeItem()
{
    NotSavedPrompter::NSP().promptSaving.remove(
	    mCB(this,uiODEarthModelSurfaceTreeItem,askSaveCB));
    mDynamicCastGet(visSurvey::EMObjectDisplay*,
		    emd,visserv_->getObject(displayid_));
    if ( emd )
    {
	emd->selection()->remove(
		mCB(this,uiODEarthModelSurfaceTreeItem,selChg) );
	emd->deSelection()->remove(
		mCB(this,uiODEarthModelSurfaceTreeItem,selChg) );
	if ( ODMainWin() )
	    ODMainWin()->colTabEd().setColTab( 0, 0, 0 );
    }

    if ( MPE::engine().hasTracker(emid_) )
	MPE::engine().unRefTracker( emid_ );
    delete uivisemobj_;

}


#define mDelRet { delete uivisemobj_; uivisemobj_ = 0; return false; }


bool uiODEarthModelSurfaceTreeItem::init()
{
    if ( !createUiVisObj() )
	return false;

    if ( !uiODDisplayTreeItem::init() )
	return false;

    if ( MPE::engine().hasTracker(emid_) )
	MPE::engine().refTracker( emid_ );

    EM::IOObjInfo eminfo( EM::EMM().getMultiID(emid_) );
    timelastmodified_ = eminfo.timeLastModified( true );
    initNotify();
    return true;
}


bool uiODEarthModelSurfaceTreeItem::createUiVisObj()
{
    if ( uivisemobj_ )
	return true;

    if ( displayid_.isValid() )
    {
	uivisemobj_ = new uiVisEMObject( ODMainWin(), displayid_, visserv_ );
	if ( !uivisemobj_->isOK() )
	    mDelRet;

	emid_ = uivisemobj_->getObjectID();
    }
    else
    {
	uivisemobj_ = new uiVisEMObject( ODMainWin(), emid_, sceneID(),
					visserv_ );
	displayid_ = uivisemobj_->id();
	if ( !uivisemobj_->isOK() )
	    mDelRet;
    }

    mDynamicCastGet(visSurvey::EMObjectDisplay*,
		    emd,visserv_->getObject(displayid_))
    if ( emd )
    {
	emd->selection()->notify(
		mCB(this,uiODEarthModelSurfaceTreeItem,selChg) );
	emd->deSelection()->notify(
		mCB(this,uiODEarthModelSurfaceTreeItem,selChg) );
    }

    return true;
}


void uiODEarthModelSurfaceTreeItem::setOnlyAtSectionsDisplay( bool yn )
{ uiODDisplayTreeItem::setOnlyAtSectionsDisplay( yn ); }

bool uiODEarthModelSurfaceTreeItem::isOnlyAtSections() const
{ return displayedOnlyAtSections(); }


void uiODEarthModelSurfaceTreeItem::selChg( CallBacker* )
{
    updateTrackingState();
}


void uiODEarthModelSurfaceTreeItem::checkCB( CallBacker* cb )
{
    uiODDisplayTreeItem::checkCB(cb);
    updateTrackingState();
}


void uiODEarthModelSurfaceTreeItem::keyPressCB( CallBacker* cb )
{
    mCBCapsuleUnpack(uiKeyDesc,kd,cb);

    if ( kd.key()==OD::KB_H && kd.state()==OD::NoButton )
    {
	uiMPEPartServer* mps = applMgr()->mpeServer();
	const bool hastracker = mps->getTrackerID(emid_)>=0;
	if ( !uivisemobj_ || !hastracker )
	    return;

	mps->showSetupDlg( emid_ );
	if ( cbcaps )
	    cbcaps->data.setKey( 0 );
    }

    uiODDisplayTreeItem::keyPressCB( cb );
}


void uiODEarthModelSurfaceTreeItem::updateTrackingState()
{
    uiMPEPartServer* mps = applMgr()->mpeServer();
    const int trackerid = mps->getTrackerID( emid_ );
    if ( trackerid == -1 )
	return;

    mDynamicCastGet( visSurvey::EMObjectDisplay*,
		     emod, visserv_->getObject(displayid_) );

    const bool enabletracking = istrackingallowed_ && isChecked() &&
				emod && emod->isSelected();

    if ( mps->isTrackingEnabled(trackerid) != enabletracking )
	mps->enableTracking( trackerid, enabletracking );
}


void uiODEarthModelSurfaceTreeItem::prepareForShutdown()
{
    uiODDisplayTreeItem::prepareForShutdown();
}


void uiODEarthModelSurfaceTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODDisplayTreeItem::createMenu( menu, istb );
    if ( !menu || !isDisplayID(menu->menuID()) )
	return;

    mDynamicCastGet(visSurvey::Scene*,scene,
		    ODMainWin()->applMgr().visServer()->getObject(sceneID()));
    const bool hastransform = scene && scene->getZAxisTransform();

    uiMPEPartServer* mps = applMgr()->mpeServer();
    const bool hastracker = mps->getTrackerID(emid_)>=0;
    if ( !hastracker && !visserv_->isLocked(displayid_) && !hastransform )
    {
	mAddMenuItem( &trackmenuitem_, &starttrackmnuitem_, true, false );
	mResetMenuItem( &changesetupmnuitem_ );
	mResetMenuItem( &enabletrackingmnuitem_ );
    }
    else if ( hastracker && !visserv_->isLocked(displayid_) &&
	      !hastransform )
    {
	mAddMenuItem( &trackmenuitem_, &starttrackmnuitem_, false, false );
	mAddMenuOrTBItem( istb, menu, &trackmenuitem_, &changesetupmnuitem_,
			  true, false );
	mAddMenuItem( &trackmenuitem_, &enabletrackingmnuitem_, true,
		      istrackingallowed_ );
    }
    else
    {
	mResetMenuItem( &starttrackmnuitem_ );
	mResetMenuItem( &changesetupmnuitem_ );
	mResetMenuItem( &enabletrackingmnuitem_ );
    }

    const bool isshifted =
		!mIsZero( visserv_->getTranslation(displayID()).z, 1e-5 );
    const MultiID mid = EM::EMM().getMultiID( emid_ );
    const bool enab = trackmenuitem_.nrItems() && !isshifted && isChecked()
			&& EM::canOverwrite( mid );
    mAddMenuOrTBItem( istb, 0, menu, &trackmenuitem_, enab, false );

    const EM::IOObjInfo eminfo( mid );
    const BufferString curtime = eminfo.timeLastModified( true );
    const bool isnewer = Time::isEarlier( timelastmodified_, curtime, 0 );
    const bool allowreload = !hastracker && isnewer;
    mAddMenuOrTBItem( istb, 0, menu, &reloadmnuitem_, allowreload, false );

    mAddMenuOrTBItem( istb, menu, menu, &savemnuitem_,
		  applMgr()->EMServer()->isChanged(emid_) &&
		  applMgr()->EMServer()->isFullyLoaded(emid_) &&
		  EM::canOverwrite( mid ) &&
		  !isshifted, false );

    const bool istransformedandshifted = hastransform && isshifted;
    mAddMenuOrTBItem( istb, 0, menu, &saveasmnuitem_,
		      !istransformedandshifted, false );
    mResetMenuItem( &createflatscenemnuitem_ );
}


VisID uiODEarthModelSurfaceTreeItem::reloadEMObject()
{
    uiEMPartServer* ems = applMgr()->EMServer();
    const MultiID mid = ems->getStorageID( emid_ );

    applMgr()->visServer()->removeObject( displayid_, sceneID() );
    delete uivisemobj_; uivisemobj_ = 0;

    if ( !ems->loadSurface(mid) )
	return VisID::udf();

    const EM::IOObjInfo eminfo( mid );
    timelastmodified_ = eminfo.timeLastModified( true );
    emid_ = applMgr()->EMServer()->getObjectID(mid);
    uivisemobj_ = new uiVisEMObject( ODMainWin(), emid_, sceneID(), visserv_ );
    return uivisemobj_->id();
}


void uiODEarthModelSurfaceTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
    mDynamicCastGet(uiMenuHandler*,uimenu,caller);
    if ( menu->isHandled() || !isDisplayID(menu->menuID()) || mnuid==-1 )
	return;

    uiMPEPartServer* mps = applMgr()->mpeServer();
    uiEMPartServer* ems = applMgr()->EMServer();
    mps->setCurrentAttribDescSet( applMgr()->attrServer()->curDescSet(false) );
    mps->setCurrentAttribDescSet( applMgr()->attrServer()->curDescSet(true) );

    if ( mnuid==savemnuitem_.id )
    {
	menu->setIsHandled( true );
	saveCB( 0 );
    }
    else if ( mnuid==saveasmnuitem_.id )
    {
	menu->setIsHandled(true);
	const MultiID oldmid = ems->getStorageID(emid_);
	mps->prepareSaveSetupAs( oldmid );

	MultiID storedmid;
	ems->storeObject( emid_, true, storedmid,
		(float) visserv_->getTranslation(displayID()).z);
	applMgr()->visServer()->setUiObjectName( displayid_,
						 ems->getUiName(emid_) );

	const MultiID midintree = ems->getStorageID(emid_);
	EM::EMM().getObject(emid_)->setMultiID( storedmid );
	mps->saveSetupAs( storedmid );
	EM::EMM().getObject(emid_)->setMultiID( midintree );

	updateColumnText( uiODSceneMgr::cNameColumn() );
    }
    else if ( mnuid==starttrackmnuitem_.id )
    {
	menu->setIsHandled(true);

	applMgr()->enableMenusAndToolBars( false );
	applMgr()->enableTree( false );

	const Coord3 pickedpos =
	    uimenu ? uimenu->getPickedPos() : Coord3::udf();
	if ( mps->addTracker(emid_,pickedpos) != -1 )
	{
	    mps->useSavedSetupDlg( emid_ );
	    uivisemobj_->checkTrackingStatus();
	    addAuxDataItems();
	    applMgr()->visServer()->triggerTreeUpdate();
	    applMgr()->visServer()->turnSeedPickingOn( true );
	}

	applMgr()->enableMenusAndToolBars( true );
	applMgr()->enableTree( true );
    }
    else if ( mnuid==changesetupmnuitem_.id )
    {
	menu->setIsHandled(true);
	mps->showSetupDlg( emid_ );
    }
    else if ( mnuid==reloadmnuitem_.id )
    {
	menu->setIsHandled(true);
	reloadEMObject();
    }
    else if ( mnuid==enabletrackingmnuitem_.id )
    {
	menu->setIsHandled(true);
	istrackingallowed_ = !istrackingallowed_;
	updateTrackingState();
    }
    else if ( mnuid==createflatscenemnuitem_.id )
    {
	mDynamicCastGet(visSurvey::EMObjectDisplay*,
			emd,visserv_->getObject(displayid_));
	const EM::ObjectID objectid = emd->getObjectID();
	mDynamicCastGet(const EM::Horizon*,horizon,
			EM::EMM().getObject( objectid ) );

	if ( !horizon ) return;

	const uiString scenenm = tr("Flattened on '%1'").arg( horizon->name() );
	RefMan<EM::HorizonZTransform> transform = new EM::HorizonZTransform;
	transform->setHorizon( *horizon );
	ODMainWin()->sceneMgr().tile();
	ODMainWin()->sceneMgr().addScene( true, transform, scenenm );
    }
}


bool uiODEarthModelSurfaceTreeItem::askSave()
{
    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( !emobj )
	return false;

    const MultiID key = emobj->multiID();
    if ( !IOM().isUsable(key) || emobj->isChanged() )
    {
	const uiString msg = tr("To continue, "
	    "%1 '%2' has to be saved.\n\nDo you want to save it?")
	    .arg(emobj->getTypeStr()).arg(emobj->name());
	const int ret = uiMSG().askSave( msg, false );
	if ( ret!=1 )
	    return false;

	return doSave();
    }

    return true;
}


void uiODEarthModelSurfaceTreeItem::askSaveCB( CallBacker* )
{
    uiEMPartServer* ems = applMgr()->EMServer();
    if ( !ems->isChanged( emid_ ) )
	return;

    bool savewithname = EM::EMM().getMultiID( emid_ ).isUdf();
    if ( !savewithname )
    {
	PtrMan<IOObj> ioobj = IOM().get( EM::EMM().getMultiID(emid_) );
	savewithname = !ioobj;
    }

    const uiString obj = toUiString("%1 \"%2\"")
	.arg( ems->getType( emid_ ) ).arg(ems->getUiName(emid_));
    NotSavedPrompter::NSP().addObject(	obj,
		mCB(this,uiODEarthModelSurfaceTreeItem,saveCB),
		savewithname, 0 );

    Threads::WorkManager::twm().addWork(
	    Threads::Work( *new uiTreeItemRemover( parent_, this ), true ), 0,
	    NotSavedPrompter::NSP().queueID(), false );
}


void uiODEarthModelSurfaceTreeItem::saveCB( CallBacker* cb )
{
    const bool issaved = doSave();
    if ( issaved && cb )
	NotSavedPrompter::NSP().reportSuccessfullSave();
}


bool uiODEarthModelSurfaceTreeItem::doSave()
{
    uiMPEPartServer* mps = applMgr()->mpeServer();
    uiEMPartServer* ems = applMgr()->EMServer();
    mps->setCurrentAttribDescSet( applMgr()->attrServer()->curDescSet(false) );
    mps->setCurrentAttribDescSet( applMgr()->attrServer()->curDescSet(true) );
    const bool hastracker = MPE::engine().hasTracker( emid_ );

    if ( !hastracker && ems->isGeometryChanged(emid_)
		     && ems->nrAttributes(emid_)>0 )
    {
	const bool res = uiMSG().askSave(
		tr("Geometry has been changed. Saved 'Horizon Data' is\n"
		   "not valid anymore and will be removed now.\n"
		   "Continue saving?") );
	if ( !res )
	    return false;
    }

    bool savewithname = EM::EMM().getMultiID( emid_ ).isUdf();
    if ( !savewithname )
    {
	PtrMan<IOObj> ioobj = IOM().get( EM::EMM().getMultiID(emid_) );
	savewithname = !ioobj;
    }

    const bool stored =
	applMgr()->EMServer()->storeObject( emid_, savewithname );
    if ( !stored )
	return false;

    applMgr()->visServer()->setUiObjectName( displayid_,
					     ems->getUiName(emid_) );
    const MultiID mid = ems->getStorageID( emid_ );
    mps->saveSetup( mid );
    updateColumnText( uiODSceneMgr::cNameColumn() );
    return true;
}


void uiODEarthModelSurfaceTreeItem::addAuxDataItems()
{
    mDynamicCastGet(const EM::Horizon3D*,hor3d,EM::EMM().getObject(emid_))
    if ( !hor3d ) return;

    BufferStringSet attrnms;
    for ( int idx=0; idx<hor3d->auxdata.nrAuxData(); idx++ )
	attrnms.add( hor3d->auxdata.auxDataName(idx) );

    applMgr()->EMServer()->loadAuxData( emid_, attrnms, true );

    for ( int idx=0; idx<hor3d->auxdata.nrAuxData(); idx++ )
    {
	DataPointSet dps( false, true );
	float shift;
	applMgr()->EMServer()->getAuxData( emid_, idx, dps, shift );
	uiODDataTreeItem* itm = addAttribItem();
	mDynamicCastGet(uiODEarthModelSurfaceDataTreeItem*,dataitm,itm);
	if ( !dataitm ) continue;

	dataitm->setDataPointSet( dps );
	dataitm->setChecked( false, true );
    }
}


// uiODEarthModelSurfaceDataTreeItem
uiODEarthModelSurfaceDataTreeItem::uiODEarthModelSurfaceDataTreeItem(
							EM::ObjectID objid,
							uiVisEMObject* uv,
							const char* parenttype )
    : uiODAttribTreeItem( parenttype )
    , depthattribmnuitem_(uiStrings::sZValue(mPlural))
    , savesurfacedatamnuitem_(m3Dots(tr("Save as Horizon Data")))
    , loadsurfacedatamnuitem_(m3Dots(tr("Horizon Data")))
    , algomnuitem_(uiStrings::sTools())
    , filtermnuitem_(m3Dots(uiStrings::sFiltering()))
    , fillholesmnuitem_(m3Dots(uiStrings::sGridding()))
    , horvariogrammnuitem_(m3Dots(tr("Variogram")))
    , attr2geommnuitm_(m3Dots(tr("Set Z values")))
    , changed_(false)
    , emid_(objid)
    , uivisemobj_(uv)
{
    horvariogrammnuitem_.iconfnm = "variogram";
}


uiODEarthModelSurfaceDataTreeItem::~uiODEarthModelSurfaceDataTreeItem()
{}


void uiODEarthModelSurfaceDataTreeItem::createMenu( MenuHandler* menu,
						    bool istb )
{
    uiODAttribTreeItem::createMenu( menu, istb );
    if ( istb ) return;

    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    const Attrib::SelSpec* as = visserv->getSelSpec( displayID(),
						     attribNr() );

    const bool islocked = visserv->isLocked( displayID() );
    const int nrsurfdata = applMgr()->EMServer()->nrAttributes( emid_ );
    uiString itmtxt = m3Dots(tr("Horizon Data (%1)").arg( nrsurfdata ));
    loadsurfacedatamnuitem_.text = itmtxt;
    mAddMenuItem( &selattrmnuitem_, &loadsurfacedatamnuitem_,
		  !islocked && nrsurfdata>0, false );
    if ( uivisemobj_ )
    {
	mAddMenuItem( &selattrmnuitem_, &depthattribmnuitem_, !islocked,
		  as->id().asInt()==Attrib::SelSpec::cNoAttrib().asInt() );
    }
    else
    {
	mResetMenuItem( &depthattribmnuitem_ );
    }

    mDynamicCastGet(visSurvey::Scene*,scene,
		    ODMainWin()->applMgr().visServer()->getObject(sceneID()));
    bool isdttransform = false;
    if ( scene && scene->getZAxisTransform() )
    {
	const BufferString zaxistrstr = scene->getZAxisTransform()
							    ->toZDomainKey();
	isdttransform = zaxistrstr == ZDomain::sKeyDepth() ||
				    zaxistrstr == ZDomain::sKeyTime();
    }
    const bool enabsave = changed_ ||
	(as && as->id()!=Attrib::SelSpec::cNoAttrib() &&
	 as->id()!=Attrib::SelSpec::cAttribNotSel()) || isdttransform ;

    mAddMenuItem( menu, &savesurfacedatamnuitem_, enabsave, false );
    const bool enabletool = !MPE::engine().trackingInProgress();
    mAddMenuItem( menu, &algomnuitem_, enabletool, false );
    mAddMenuItem( &algomnuitem_, &filtermnuitem_, enabletool, false );
    mAddMenuItem( &algomnuitem_, &fillholesmnuitem_, enabletool, false );
    mAddMenuItem( &algomnuitem_, &horvariogrammnuitem_, enabletool, false );
    mAddMenuItem( &algomnuitem_, &attr2geommnuitm_, enabletool, false );
}


void uiODEarthModelSurfaceDataTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODAttribTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( mnuid==-1 || menu->isHandled() )
	return;

    const VisID visid = displayID();
    const int attribnr = attribNr();

    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    const Attrib::SelSpec* as = visserv->getSelSpec( visid, attribnr );
    if ( mnuid==savesurfacedatamnuitem_.id )
    {
	menu->setIsHandled( true );
	DataPointSet vals( false, true );
	vals.bivSet().setNrVals( 3 );
	visserv->getRandomPosCache( visid, attribnr, vals );
	mDynamicCastGet(visSurvey::Scene*,scene, visserv->getObject(sceneID()));
	bool isdttransform = false;
	BufferString zaxstrstr;
	if ( scene && scene->getZAxisTransform() )
	{
	    zaxstrstr = scene->getZAxisTransform()->toZDomainKey();
	    isdttransform = ( zaxstrstr == ZDomain::sKeyDepth() )  ||
					   ( zaxstrstr == ZDomain::sKeyTime() );
	}
	if ( vals.size() )
	{
	    BufferString auxdatanm;  auxdatanm = name_.getFullString();
	    if ( auxdatanm == sKey::ZValue() && isdttransform )
	    {
		auxdatanm = zaxstrstr; auxdatanm += " Value";
	    }

	    const float shift = (float) visserv->getTranslation( visid ).z;
	    const int validx = visserv->selectedTexture( visid, attribnr ) + 2;
	    const int auxnr = applMgr()->EMServer()->setAuxData( emid_, vals,
		    auxdatanm, validx, shift );
	    if ( auxnr<0 )
	    {
		pErrMsg( "Cannot find Horizon Data." );
		return;
	    }

	    const bool saved =
		applMgr()->EMServer()->storeAuxData( emid_, auxdatanm, true );
	    if ( saved )
	    {
		const Attrib::SelSpec newas( auxdatanm,
			Attrib::SelSpec::cOtherAttrib() );
		visserv->setSelSpec( visid, attribnr, newas );
		BufferStringSet* userrefs = new BufferStringSet;
		userrefs->add( "Section ID" );
		userrefs->add( auxdatanm );
		visserv->setUserRefs( visid, attribnr, userrefs );
		updateColumnText( uiODSceneMgr::cNameColumn() );
	    }
	    changed_ = !saved;
	}
    }
    else if ( mnuid==depthattribmnuitem_.id )
    {
	menu->setIsHandled( true );
	uivisemobj_->setDepthAsAttrib( attribnr );
	updateColumnText( uiODSceneMgr::cNameColumn() );
	updateColumnText( uiODSceneMgr::cColorColumn() );
	applMgr()->updateColorTable( displayID(), attribnr );
	changed_ = false;
    }
    else if ( mnuid==loadsurfacedatamnuitem_.id )
    {
	menu->setIsHandled( true );
	selectAndLoadAuxData();
	changed_ = false;
    }
    else if ( mnuid==fillholesmnuitem_.id || mnuid==filtermnuitem_.id
	   || mnuid==attr2geommnuitm_.id || mnuid==horvariogrammnuitem_.id )
    {
	menu->setIsHandled( true );

	if ( !as || as->id().asInt()!=Attrib::SelSpec::cOtherAttrib().asInt() )
	{
	   uiMSG().error(tr("This algorithm can only be applied on "
			    "'Horizon Data'.\nPlease save attribute first"));
	    return;
	}

	bool res = false;
	DataPointSet vals( false, true );
	visserv->getRandomPosCache( visid, attribnr, vals );
	if ( mnuid==filtermnuitem_.id )
	    res = applMgr()->EMServer()->filterAuxData( emid_,
				    name_.getFullString(), vals);
	else if ( mnuid==fillholesmnuitem_.id )
	    res = applMgr()->EMServer()->
			interpolateAuxData( emid_,name_.getFullString(), vals);
	else if ( mnuid==horvariogrammnuitem_.id )
	    res = applMgr()->EMServer()->
		computeVariogramAuxData( emid_, name_.getFullString(), vals );
	else if ( mnuid==attr2geommnuitm_.id )
	    res = applMgr()->EMServer()->
		attr2Geom( emid_, name_.getFullString(), vals );

	if ( !res || mnuid==horvariogrammnuitem_.id )
	    return;

	visserv->setSelSpec( visid, attribnr,
		Attrib::SelSpec(name_.getFullString(),
				Attrib::SelSpec::cOtherAttrib()) );
	visserv->setRandomPosData( visid, attribnr, &vals );
	changed_ = true;
    }
}


void uiODEarthModelSurfaceDataTreeItem::selectAndLoadAuxData()
{
    if ( !applMgr()->EMServer()->showLoadAuxDataDlg(emid_) )
	return;

    mDynamicCastGet(visSurvey::HorizonDisplay*,vishor,
		    applMgr()->visServer()->getObject(displayID()) );
    if ( !vishor )
	return;

    const StepInterval<int>& grrg = vishor->geometryRowRange();
    const StepInterval<int>& gcrg = vishor->geometryColRange();
    const visBase::HorizonSection* horsect = vishor->getSection( 0 );
    StepInterval<int> loadrrg = horsect ? horsect->displayedRowRange()
					: grrg;
    StepInterval<int> loadcrg = horsect ? horsect->displayedColRange()
					: gcrg;
    TrcKeyZSampling cs( true );
    cs.hsamp_.set( loadrrg, loadcrg );

    TypeSet<float> shifts;
    DataPointSet vals( false, true );
    applMgr()->EMServer()->getAllAuxData( emid_, vals, &shifts, &cs );
    setDataPointSet( vals );
    vishor->setAttribShift( attribNr(), shifts );

    updateColumnText( uiODSceneMgr::cNameColumn() );
}


void uiODEarthModelSurfaceDataTreeItem::setDataPointSet(
						const DataPointSet& vals )
{
    const VisID visid = displayID();
    const int attribnr = attribNr();
    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    TypeSet<Attrib::SelSpec> specs;
    for ( int idx=1; idx<vals.nrCols(); idx++ ) // Skip SectionID
    {
	const char* attrnm = vals.colName( idx );
	specs += Attrib::SelSpec( attrnm, Attrib::SelSpec::cOtherAttrib() );
    }

    visserv->setSelSpecs( visid, attribnr, specs );
    visserv->setRandomPosData( visid, attribnr, &vals );
    visserv->selectTexture( visid, attribnr, 0 );
    updateColumnText( uiODSceneMgr::cNameColumn() );
}


uiString uiODEarthModelSurfaceDataTreeItem::createDisplayName() const
{
    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    const Attrib::SelSpec* as = visserv->getSelSpec( displayID(), attribNr() );

    if ( as && as->id().asInt()==Attrib::SelSpec::cNoAttrib().asInt() )
	return uiStrings::sZValue(mPlural);

    return uiODAttribTreeItem::createDisplayName();
}
