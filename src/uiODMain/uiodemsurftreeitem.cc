/*+
___________________________________________________________________

 CopyRight: 	(C) dGB Beheer B.V.
 Author: 	K. Tingdahl
 Date: 		Jul 2003
 RCS:		$Id: uiodemsurftreeitem.cc,v 1.25 2007-09-17 12:44:31 cvskris Exp $
___________________________________________________________________

-*/

#include "uiodemsurftreeitem.h"

#include "uimenu.h"
#include "uiodapplmgr.h"
#include "uivispartserv.h"
#include "uilistview.h"
#include "uimenuhandler.h"

#include "binidvalset.h"

#include "uiattribpartserv.h"
#include "uiempartserv.h"
#include "uimpepartserv.h"
#include "uimsg.h"
#include "uiodscenemgr.h"
#include "uivisemobj.h"
#include "visemobjdisplay.h"


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
    , savemnuitem_("Save")
    , saveasmnuitem_("Save as ...")
    , enabletrackingmnuitem_("Enable tracking")
    , changesetupmnuitem_("Change setup ...")
    , reloadmnuitem_("Reload")
    , relationsmnuitem_("Relations ...")
    , starttrackmnuitem_("Start tracking ...")
{
    enabletrackingmnuitem_.checkable = true;
}


uiODEarthModelSurfaceTreeItem::~uiODEarthModelSurfaceTreeItem()
{ 
    delete uivisemobj_;
}


#define mDelRet { delete uivisemobj_; uivisemobj_ = 0; return false; }


bool uiODEarthModelSurfaceTreeItem::init()
{
    delete uivisemobj_;
    if ( displayid_!=-1 )
    {
	uivisemobj_ = new uiVisEMObject( getUiParent(), displayid_, visserv_ );
	if ( !uivisemobj_->isOK() )
	    mDelRet;

	emid_ = uivisemobj_->getObjectID();
    }
    else
    {
	uivisemobj_ = new uiVisEMObject( getUiParent(), emid_, sceneID(),
					visserv_ );
	displayid_ = uivisemobj_->id();
	if ( !uivisemobj_->isOK() )
	    mDelRet;
    }

    if ( !uiODDisplayTreeItem::init() )
	return false;

    initNotify();

    return true;
}


void uiODEarthModelSurfaceTreeItem::checkCB( CallBacker* cb )
{
    uiODDisplayTreeItem::checkCB(cb);

    const int trackerid = applMgr()->mpeServer()->getTrackerID(emid_);
    if ( trackerid==-1 )
    {
	prevtrackstatus_ = false;
	return;
    }

    if ( isChecked() )
	applMgr()->mpeServer()->enableTracking(trackerid, prevtrackstatus_);
    else
    {
	prevtrackstatus_ = applMgr()->mpeServer()->isTrackingEnabled(trackerid);
	applMgr()->mpeServer()->enableTracking(trackerid,false);
    }
    applMgr()->visServer()->updateMPEToolbar();
}


void uiODEarthModelSurfaceTreeItem::prepareForShutdown()
{
    applMgr()->EMServer()->askUserToSave(emid_);
}


uiODEarthModelSurfaceDataTreeItem::uiODEarthModelSurfaceDataTreeItem(
							EM::ObjectID objid,
							uiVisEMObject* uv,
       							const char* parenttype )
    : uiODAttribTreeItem( parenttype )
    , depthattribmnuitem_("Z values")
    , savesurfacedatamnuitem_("Save attribute ...")
    , loadsurfacedatamnuitem_("Surface data ...")
    , emid_(objid)
    , uivisemobj_(uv)
{
}


void uiODEarthModelSurfaceDataTreeItem::createMenuCB( CallBacker* cb )
{
    uiODAttribTreeItem::createMenuCB( cb );
    mDynamicCastGet(uiMenuHandler*,menu,cb);

    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    const Attrib::SelSpec* as = visserv->getSelSpec( displayID(),
	    					     attribNr() );

    const bool islocked = visserv->isLocked( displayID() );
    const int nrsurfdata = applMgr()->EMServer()->nrAttributes( emid_ );
    BufferString itmtxt = "Surface data ("; itmtxt += nrsurfdata;
    itmtxt += ") ...";
    loadsurfacedatamnuitem_.text = itmtxt;
    mAddMenuItem( &selattrmnuitem_, &loadsurfacedatamnuitem_,
		  !islocked && nrsurfdata>0, false );
    mAddMenuItem( &selattrmnuitem_, &depthattribmnuitem_, !islocked,
		  as->id()==Attrib::SelSpec::cNoAttrib() );

    mAddMenuItem( menu, &savesurfacedatamnuitem_, as && as->id() >= 0, false );
}


void uiODEarthModelSurfaceDataTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODAttribTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(uiMenuHandler*,menu,caller);
    if ( mnuid==-1 || menu->isHandled() )
	return;

    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    if ( mnuid==savesurfacedatamnuitem_.id )
    {
	menu->setIsHandled( true );
	ObjectSet<const BinIDValueSet> vals;
	visserv->getRandomPosCache( displayID(), attribNr(), vals );
	if ( vals.size() && vals[0]->nrVals()>=2 )
	{
	    const int auxnr =
		applMgr()->EMServer()->setAuxData( emid_, vals, name_, 1 );
	    applMgr()->EMServer()->storeAuxData( emid_, true );
	}
    }
    else if ( mnuid==depthattribmnuitem_.id )
    {
	menu->setIsHandled( true );
	uivisemobj_->setDepthAsAttrib( attribNr() );
	updateColumnText( uiODSceneMgr::cNameColumn() );
    }
    else if ( mnuid==loadsurfacedatamnuitem_.id )
    {
	menu->setIsHandled( true );
	if ( !applMgr()->EMServer()->showLoadAuxDataDlg(emid_) )
	    return;

	BufferStringSet attrnms;
	ObjectSet<BinIDValueSet> vals;
	applMgr()->EMServer()->getAllAuxData( emid_, attrnms, vals );
	BufferString attrnm = attrnms.size() ? attrnms.get(0) : "";
	visserv->setSelSpec( displayID(), attribNr(),
		Attrib::SelSpec(attrnm,Attrib::SelSpec::cOtherAttrib()) );
	visserv->setRandomPosData( displayID(), attribNr(), &vals );
	updateColumnText( uiODSceneMgr::cNameColumn() );
    }
}


BufferString uiODEarthModelSurfaceDataTreeItem::createDisplayName() const
{
    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    const Attrib::SelSpec* as = visserv->getSelSpec( displayID(),
	    					     attribNr() );

    if ( as->id()==Attrib::SelSpec::cNoAttrib() )
	return BufferString("Z values");

    return uiODAttribTreeItem::createDisplayName();
}


#define mIsObject(typestr) \
	!strcmp(uivisemobj_->getObjectType(displayid_),typestr)

void uiODEarthModelSurfaceTreeItem::createMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::createMenuCB(cb);
    mDynamicCastGet(uiMenuHandler*,menu,cb);

    if ( menu->menuID()!=displayID() )
	return;

    mDynamicCastGet(visSurvey::Scene*,scene,
	    	    ODMainWin()->applMgr().visServer()->getObject(sceneID()));
    const bool hastransform = scene && scene->getDataTransform();
    MenuItem* trackmnu = menu->findItem(uiVisEMObject::trackingmenutxt);

    if ( isChecked() && trackmnu )
    {
	EM::SectionID section = -1;
	if ( uivisemobj_->nrSections()==1 )
	    section = uivisemobj_->getSectionID(0);
	else if ( menu->getPath() )
	    section = uivisemobj_->getSectionID( menu->getPath() );

	uiMPEPartServer* mps = applMgr()->mpeServer();
	const bool hastracker = mps->getTrackerID(emid_)>=0;
	if ( !hastracker && !visserv_->isLocked(displayid_) && !hastransform )
	{
	    mAddMenuItem( trackmnu, &starttrackmnuitem_, true, false );
	    mResetMenuItem( &changesetupmnuitem_ );
	    mResetMenuItem( &enabletrackingmnuitem_ );
	    mResetMenuItem( &relationsmnuitem_ );
	}
	else if ( hastracker && section!=-1 && !visserv_->isLocked(displayid_)&&
		  !hastransform )
	{
	    mAddMenuItem( trackmnu, &starttrackmnuitem_, false, false );
	    mAddMenuItem( trackmnu, &changesetupmnuitem_, true, false );
	    mAddMenuItem( trackmnu, &enabletrackingmnuitem_, true,
			  mps->isTrackingEnabled(mps->getTrackerID(emid_)) );
	    mResetMenuItem( &relationsmnuitem_ );
	    //mAddMenuItem( trackmnu, &relationsmnuitem_,
	//	    mIsObject(EM::Horizon::typeStr()), false );
	}

    }
    else
    {
	mResetMenuItem( &starttrackmnuitem_ );
	mResetMenuItem( &changesetupmnuitem_ );
	mResetMenuItem( &enabletrackingmnuitem_ );
	mResetMenuItem( &relationsmnuitem_ );
    }

    mAddMenuItem( menu, &savemnuitem_,
		  applMgr()->EMServer()->isChanged(emid_) && 
		  applMgr()->EMServer()->isFullyLoaded(emid_) &&
		  !applMgr()->EMServer()->isShifted(emid_), false );

    mAddMenuItem( menu, &saveasmnuitem_, true, false );
#ifdef __debug__
    mAddMenuItem( menu, &reloadmnuitem_, true, false );
#else
    mResetMenuItem( &reloadmnuitem_ );
#endif
}


void uiODEarthModelSurfaceTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(uiMenuHandler*,menu,caller);
    if ( menu->menuID()!=displayID() || mnuid==-1 || menu->isHandled() )
	return;

    EM::SectionID sectionid = -1;
    if ( uivisemobj_->nrSections()==1 )
	sectionid = uivisemobj_->getSectionID(0);
    else if ( menu->getPath() )
	sectionid = uivisemobj_->getSectionID( menu->getPath() );
    
    uiMPEPartServer* mps = applMgr()->mpeServer();
    uiEMPartServer* ems = applMgr()->EMServer();
    mps->setCurrentAttribDescSet( applMgr()->attrServer()->curDescSet(false) );
    mps->setCurrentAttribDescSet( applMgr()->attrServer()->curDescSet(true) );

    if ( mnuid==savemnuitem_.id )
    {
	menu->setIsHandled( true );
	if ( ems->isGeometryChanged(emid_) && ems->nrAttributes(emid_)>0 )
	{
	    const bool res = uiMSG().askGoOn(
		    "Geometry has been changed. Saved 'Surface Data' is\n"
		    "not valid anymore and will be removed now.\n"
		    "Continue saving?" );
	    if ( !res )
		return;
	}

	applMgr()->EMServer()->storeObject( emid_, false );
	const MultiID mid = ems->getStorageID(emid_);
	mps->saveSetup( mid );
    }
    else if ( mnuid==saveasmnuitem_.id )
    {
	menu->setIsHandled(true);
	const MultiID oldmid = ems->getStorageID(emid_);
	mps->prepareSaveSetupAs( oldmid );

	ems->storeObject( emid_, true );
	applMgr()->visServer()->setObjectName( displayid_,
		(const char*) ems->getName(emid_) );

	const MultiID newmid = ems->getStorageID(emid_);
	mps->saveSetupAs( newmid );

	updateColumnText( uiODSceneMgr::cNameColumn() );
    }
    else if ( mnuid==starttrackmnuitem_.id )
    {
	menu->setIsHandled(true);
	if ( sectionid < 0 ) return;

	applMgr()->enableMenusAndToolBars( false );
	applMgr()->enableTree( false );

	if ( mps->addTracker(emid_,menu->getPickedPos())!=-1 )
	{
	    mps->useSavedSetupDlg( emid_, sectionid );
	    uivisemobj_->checkTrackingStatus();
	    applMgr()->visServer()->triggerTreeUpdate();
	    applMgr()->visServer()->introduceMPEDisplay();
	    applMgr()->visServer()->showMPEToolbar();
	}

	applMgr()->enableMenusAndToolBars( true );
	applMgr()->enableTree( true );
    }
    else if ( mnuid==changesetupmnuitem_.id )
    {
	menu->setIsHandled(true);
	mps->showSetupDlg( emid_, sectionid, true );
	applMgr()->visServer()->updateMPEToolbar();
    }
    else if ( mnuid==reloadmnuitem_.id )
    {
	menu->setIsHandled(true);
	uiTreeItem* parent__ = parent_;

	const MultiID mid = ems->getStorageID(emid_);

	applMgr()->visServer()->removeObject( displayid_, sceneID() );
	delete uivisemobj_; uivisemobj_ = 0;

	if ( !ems->loadSurface(mid) )
	    return;

	emid_ = applMgr()->EMServer()->getObjectID(mid);

	uivisemobj_ = new uiVisEMObject(getUiParent(),emid_,sceneID(),visserv_);
	displayid_ = uivisemobj_->id();
    }
    else if ( mnuid==relationsmnuitem_.id )
    {	
	menu->setIsHandled(true);
	mps->showRelationsDlg( emid_, sectionid );
    }
    else if ( mnuid==enabletrackingmnuitem_.id )
    {
	menu->setIsHandled(true);
	const int trackerid = mps->getTrackerID(emid_);
	mps->enableTracking( trackerid, !mps->isTrackingEnabled(trackerid) );
	applMgr()->visServer()->updateMPEToolbar();
    }
}
