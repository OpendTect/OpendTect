/*+
___________________________________________________________________

 CopyRight: 	(C) dGB Beheer B.V.
 Author: 	K. Tingdahl
 Date: 		Jul 2003
 RCS:		$Id: uiodemsurftreeitem.cc,v 1.19 2007-02-22 12:45:37 cvsjaap Exp $
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
#include "uiodscenemgr.h"
#include "uivisemobj.h"
#include "uimpepartserv.h"
#include "visemobjdisplay.h"


uiODDataTreeItem* uiODEarthModelSurfaceTreeItem::createAttribItem(
					const Attrib::SelSpec* as ) const
{
    const char* parenttype = typeid(*this).name();
    uiODDataTreeItem* res = as ? uiODDataTreeItem::create( *as, parenttype) : 0;
    if ( !res ) res = new uiODEarthModelSurfaceDataTreeItem( emid, uivisemobj,
	    						     parenttype );
	    
    return res;
}


uiODEarthModelSurfaceTreeItem::uiODEarthModelSurfaceTreeItem(
						const EM::ObjectID& nemid )
    : uiODDisplayTreeItem()
    , emid(nemid)
    , uivisemobj(0)
    , savemnuitem_("Save")
    , saveasmnuitem_("Save as ...")
    , enabletrackingmnuitem_("Enable tracking")
    , changesetupmnuitem_("Change setup ...")
    , reloadmnuitem_("Reload")
    , relationsmnuitem_("Relations ...")
    , starttrackmnuitem_("Start tracking ...")
{}


uiODEarthModelSurfaceTreeItem::~uiODEarthModelSurfaceTreeItem()
{ 
    delete uivisemobj;
}


#define mDelRet { delete uivisemobj; uivisemobj = 0; return false; }


bool uiODEarthModelSurfaceTreeItem::init()
{
    delete uivisemobj;
    if ( displayid_!=-1 )
    {
	uivisemobj = new uiVisEMObject( getUiParent(), displayid_, visserv );
	if ( !uivisemobj->isOK() )
	    mDelRet;

	emid = uivisemobj->getObjectID();
    }
    else
    {
	uivisemobj = new uiVisEMObject( getUiParent(), emid, sceneID(),
					visserv );
	displayid_ = uivisemobj->id();
	if ( !uivisemobj->isOK() )
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

    const int trackerid = applMgr()->mpeServer()->getTrackerID(emid);
    if ( trackerid==-1 )
    {
	prevtrackstatus = false;
	return;
    }

    if ( isChecked() )
	applMgr()->mpeServer()->enableTracking(trackerid, prevtrackstatus);
    else
    {
	prevtrackstatus = applMgr()->mpeServer()->isTrackingEnabled(trackerid);
	applMgr()->mpeServer()->enableTracking(trackerid,false);
    }
    applMgr()->visServer()->updateMPEToolbar();
}


void uiODEarthModelSurfaceTreeItem::prepareForShutdown()
{
    applMgr()->EMServer()->askUserToSave(emid);
}


uiODEarthModelSurfaceDataTreeItem::uiODEarthModelSurfaceDataTreeItem(
							EM::ObjectID objid,
							uiVisEMObject* uv,
       							const char* parenttype )
    : uiODAttribTreeItem( parenttype )
    , depthattribmnuitem_("Z values")
    , savesurfacedatamnuitem_("Save attribute ...")
    , loadsurfacedatamnuitem_("Surface data ...")
    , emid(objid)
    , uivisemobj(uv)
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
    const int nrsurfdata = uivisemobj->nrSurfaceData();
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
		applMgr()->EMServer()->setAuxData( emid, vals, name_, 1 );
	    applMgr()->EMServer()->storeAuxData( emid, true );
	}
    }
    else if ( mnuid==depthattribmnuitem_.id )
    {
	menu->setIsHandled( true );
	uivisemobj->setDepthAsAttrib( attribNr() );
	updateColumnText( uiODSceneMgr::cNameColumn() );
    }
    else if ( mnuid==loadsurfacedatamnuitem_.id )
    {
	menu->setIsHandled( true );
	if ( !applMgr()->EMServer()->showLoadAuxDataDlg(emid) )
	    return;

	BufferStringSet attrnms;
	ObjectSet<BinIDValueSet> vals;
	applMgr()->EMServer()->getAllAuxData( emid, attrnms, vals );
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
	!strcmp(uivisemobj->getObjectType(displayid_),typestr)

void uiODEarthModelSurfaceTreeItem::createMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::createMenuCB(cb);
    mDynamicCastGet(uiMenuHandler*,menu,cb);

    if ( menu->menuID()!=displayID() )
	return;

    MenuItem* trackmnu = menu->findItem(uiVisEMObject::trackingmenutxt);
    if ( isChecked() && trackmnu )
    {
	EM::SectionID section = -1;
	if ( uivisemobj->nrSections()==1 )
	    section = uivisemobj->getSectionID(0);
	else if ( menu->getPath() )
	    section = uivisemobj->getSectionID( menu->getPath() );

	const bool hastracker = applMgr()->mpeServer()->getTrackerID(emid)>=0;
	if ( !hastracker && !visserv->isLocked(displayid_) )
	{
	    mAddMenuItem( trackmnu, &starttrackmnuitem_, true, false );
	    mResetMenuItem( &changesetupmnuitem_ );
	    mResetMenuItem( &enabletrackingmnuitem_ );
	    mResetMenuItem( &relationsmnuitem_ );
	}
	else if ( hastracker && section!=-1 && !visserv->isLocked(displayid_) )
	{
	    mAddMenuItem( trackmnu, &starttrackmnuitem_, false, false );
	    mAddMenuItem( trackmnu, &changesetupmnuitem_, true, false );
	    mAddMenuItem( trackmnu, &enabletrackingmnuitem_, true,
		   applMgr()->mpeServer()->isTrackingEnabled(
		      applMgr()->mpeServer()->getTrackerID(emid)) );

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
		  applMgr()->EMServer()->isChanged(emid) && 
		  applMgr()->EMServer()->isFullyLoaded(emid) &&
		  !applMgr()->EMServer()->isShifted(emid), false );

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
    if ( uivisemobj->nrSections()==1 )
	sectionid = uivisemobj->getSectionID(0);
    else if ( menu->getPath() )
	sectionid = uivisemobj->getSectionID( menu->getPath() );
    
    uiMPEPartServer* mps = applMgr()->mpeServer();
    mps->setCurrentAttribDescSet( applMgr()->attrServer()->curDescSet(false) );
    mps->setCurrentAttribDescSet( applMgr()->attrServer()->curDescSet(true) );

    if ( mnuid==savemnuitem_.id )
    {
	menu->setIsHandled(true);
	applMgr()->EMServer()->storeObject( emid, false );

	const MultiID mid = applMgr()->EMServer()->getStorageID(emid);
	mps->saveSetup( mid );
    }
    else if ( mnuid==saveasmnuitem_.id )
    {
	menu->setIsHandled(true);
	const MultiID oldmid = applMgr()->EMServer()->getStorageID(emid);
	mps->prepareSaveSetupAs( oldmid );

	applMgr()->EMServer()->storeObject( emid, true );
	applMgr()->visServer()->setObjectName( displayid_,
		(const char*) applMgr()->EMServer()->getName(emid) );

	const MultiID newmid = applMgr()->EMServer()->getStorageID(emid);
	mps->saveSetupAs( newmid );

	updateColumnText( uiODSceneMgr::cNameColumn() );
    }
    else if ( mnuid==starttrackmnuitem_.id )
    {
	menu->setIsHandled(true);
	if ( sectionid < 0 ) return;

	applMgr()->enableMenusAndToolBars( false );
	applMgr()->enableTree( false );

	if ( mps->addTracker(emid,menu->getPickedPos())!=-1 )
	{
	    mps->useSavedSetupDlg( emid, sectionid );
	    uivisemobj->checkTrackingStatus();
	    applMgr()->visServer()->showMPEToolbar();
	    applMgr()->visServer()->triggerTreeUpdate();
	}

	applMgr()->enableMenusAndToolBars( true );
	applMgr()->enableTree( true );
    }
    else if ( mnuid==changesetupmnuitem_.id )
    {
	menu->setIsHandled(true);
	mps->showSetupDlg( emid, sectionid, true );
	applMgr()->visServer()->updateMPEToolbar();
    }
    else if ( mnuid==reloadmnuitem_.id )
    {
	menu->setIsHandled(true);
	uiTreeItem* parent__ = parent_;

	const MultiID mid = applMgr()->EMServer()->getStorageID(emid);

	applMgr()->visServer()->removeObject( displayid_, sceneID() );
	delete uivisemobj; uivisemobj = 0;

	if ( !applMgr()->EMServer()->loadSurface(mid) )
	    return;

	emid = applMgr()->EMServer()->getObjectID(mid);

	uivisemobj = new uiVisEMObject( getUiParent(), emid, sceneID(),visserv);
	displayid_ = uivisemobj->id();
    }
    else if ( mnuid==relationsmnuitem_.id )
    {	
	menu->setIsHandled(true);
	mps->showRelationsDlg( emid, sectionid );
    }
    else if ( mnuid==enabletrackingmnuitem_.id )
    {
	menu->setIsHandled(true);
	const int trackerid = mps->getTrackerID(emid);
	mps->enableTracking( trackerid, !mps->isTrackingEnabled(trackerid) );
	applMgr()->visServer()->updateMPEToolbar();
    }
}
