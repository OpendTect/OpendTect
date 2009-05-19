/*+
___________________________________________________________________

 CopyRight: 	(C) dGB Beheer B.V.
 Author: 	K. Tingdahl
 Date: 		Jul 2003
___________________________________________________________________

-*/
static const char* rcsID = "$Id: uiodemsurftreeitem.cc,v 1.50 2009-05-19 05:47:08 cvsnanne Exp $";

#include "uiodemsurftreeitem.h"

#include "datapointset.h"
#include "emhorizon.h"
#include "emhorizonztransform.h"
#include "emmanager.h"
#include "ioman.h"
#include "ioobj.h"

#include "uiattribpartserv.h"
#include "uiempartserv.h"
#include "uiemattribpartserv.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uimpepartserv.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uivisemobj.h"
#include "uivispartserv.h"
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
    , createflatscenemnuitem_("&Create flattened scene")
    , savemnuitem_("Save")
    , saveasmnuitem_("Save as ...")
    , enabletrackingmnuitem_("Enable tracking")
    , changesetupmnuitem_("Change setup ...")
    , reloadmnuitem_("Reload")
    , starttrackmnuitem_("Start tracking ...")
    , treeitemwasenabled_(true)
    , prevtrackstatus_(true)
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
    treeitemwasenabled_ = isChecked();

    return true;
}


void uiODEarthModelSurfaceTreeItem::checkCB( CallBacker* cb )
{
    uiODDisplayTreeItem::checkCB(cb);

    uiMPEPartServer* mps = applMgr()->mpeServer();
    const int trackerid = mps->getTrackerID( emid_ );
    if ( trackerid == -1 )
	return;

    if ( !treeitemwasenabled_ && isChecked() )
	mps->enableTracking( trackerid, prevtrackstatus_ );

    if ( treeitemwasenabled_ && !isChecked() )
    {
	prevtrackstatus_ = mps->isTrackingEnabled( trackerid );
	mps->enableTracking( trackerid, false );
    }

    treeitemwasenabled_ = isChecked();
    applMgr()->visServer()->updateMPEToolbar();
}


void uiODEarthModelSurfaceTreeItem::prepareForShutdown()
{
    applMgr()->EMServer()->askUserToSave(emid_);
    uiTreeItem::prepareForShutdown();
}


uiODEarthModelSurfaceDataTreeItem::uiODEarthModelSurfaceDataTreeItem(
							EM::ObjectID objid,
							uiVisEMObject* uv,
       							const char* parenttype )
    : uiODAttribTreeItem( parenttype )
    , depthattribmnuitem_("Z values")
    , savesurfacedatamnuitem_("Save attribute ...")
    , loadsurfacedatamnuitem_("Surface data ...")
    , algomnuitem_("&Algorithms")
    , fillholesmnuitem_("&Grid ...")
    , filtermnuitem_("&Filter ...")
    , changed_(false)
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
    if ( uivisemobj_ )
    {
	mAddMenuItem( &selattrmnuitem_, &depthattribmnuitem_, !islocked,
		  as->id()==Attrib::SelSpec::cNoAttrib() );
    }
    else
    {
	mResetMenuItem( &depthattribmnuitem_ );
    }

    const bool enabsave = changed_ ||
	(as && as->id()!=Attrib::SelSpec::cNoAttrib() &&
	 as->id()!=Attrib::SelSpec::cAttribNotSel() ); 

    mAddMenuItem( menu, &savesurfacedatamnuitem_, enabsave, false );
    mAddMenuItem( menu, &algomnuitem_, true, false );
    mAddMenuItem( &algomnuitem_, &fillholesmnuitem_, true, false );
    mAddMenuItem( &algomnuitem_, &filtermnuitem_, true, false );
}


void uiODEarthModelSurfaceDataTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODAttribTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(uiMenuHandler*,menu,caller);
    if ( mnuid==-1 || menu->isHandled() )
	return;

    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    const Attrib::SelSpec* as = visserv->getSelSpec( displayID(), attribNr() );
    if ( mnuid==savesurfacedatamnuitem_.id )
    {
	menu->setIsHandled( true );
	TypeSet<DataPointSet::DataRow> pts;
	BufferStringSet nms;
	DataPointSet vals( pts, nms, false, true );
	vals.bivSet().setNrVals(2);
	visserv->getRandomPosCache( displayID(), attribNr(), vals );
	if ( vals.size() )
	{
	    const int auxnr =
		applMgr()->EMServer()->setAuxData( emid_, vals, name_, 1,
		       	applMgr()->EMAttribServer()->getShift() );
	    BufferString auxdatanm;
	    const bool saved =
		applMgr()->EMServer()->storeAuxData( emid_, auxdatanm, true );
	    if ( saved )
	    {
		const Attrib::SelSpec newas( auxdatanm,
			Attrib::SelSpec::cOtherAttrib() );
		visserv->setSelSpec( displayID(), attribNr(), newas );
		updateColumnText( uiODSceneMgr::cNameColumn() );
	    }
	    changed_ = !saved;
	}
    }
    else if ( mnuid==depthattribmnuitem_.id )
    {
	menu->setIsHandled( true );
	uivisemobj_->setDepthAsAttrib( attribNr() );
	updateColumnText( uiODSceneMgr::cNameColumn() );
	changed_ = false;
    }
    else if ( mnuid==loadsurfacedatamnuitem_.id )
    {
	menu->setIsHandled( true );
	if ( !applMgr()->EMServer()->showLoadAuxDataDlg(emid_) )
	    return;

	BufferStringSet attrnms;
	TypeSet< float > shifts;
	TypeSet<DataPointSet::DataRow> pts;
	DataPointSet vals( pts, attrnms, false, true );
	applMgr()->EMServer()->getAllAuxData( emid_, vals, &shifts );
	BufferString attrnm = attrnms.size() ? attrnms.get(0) : "";
	visserv->setSelSpec( displayID(), attribNr(),
		Attrib::SelSpec(attrnm,Attrib::SelSpec::cOtherAttrib()) );
	visserv->createAndDispDataPack( displayID(), attribNr(), &vals );
	visserv->setAttribShift( displayID(), attribNr(), shifts );
	visserv->selectTexture( displayID(), attribNr(), 0 );

	updateColumnText( uiODSceneMgr::cNameColumn() );
	changed_ = false;
    }
    else if ( mnuid==fillholesmnuitem_.id || mnuid==filtermnuitem_.id )
    {
	menu->setIsHandled( true );

	if ( !as || as->id()!=Attrib::SelSpec::cOtherAttrib() )
	{
	    uiMSG().error( "This algorithm can only be applied on Surface data."
		    	   "\nPlease save attribute first" );
	    return;
	}
	
	TypeSet<DataPointSet::DataRow> pts;
	BufferStringSet nms;
	DataPointSet vals( pts, nms, false, true );
	BinIDValueSet* set = 0;
	if ( mnuid==fillholesmnuitem_.id )
	    set = applMgr()->EMServer()->interpolateAuxData( emid_, name_ );
	else
	    set = applMgr()->EMServer()->filterAuxData( emid_, name_ );
	if ( !set ) return;
	if ( vals.bivSet().nrVals()< set->nrVals() )
	    vals.bivSet().setNrVals( set->nrVals() );
	
	vals.bivSet().append( *set );
	vals.dataChanged();
	visserv->setSelSpec( displayID(), attribNr(),
		Attrib::SelSpec(name_,Attrib::SelSpec::cOtherAttrib()) );
	visserv->setRandomPosData( displayID(), attribNr(), &vals );
	changed_ = true;
    }
}


BufferString uiODEarthModelSurfaceDataTreeItem::createDisplayName() const
{
    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    const Attrib::SelSpec* as = visserv->getSelSpec( displayID(), attribNr() );

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
    MenuItem* trackmnu = menu->findItem(uiVisEMObject::trackingmenutxt());

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
	}
	else if ( hastracker && section!=-1 && !visserv_->isLocked(displayid_)&&
		  !hastransform )
	{
	    mAddMenuItem( trackmnu, &starttrackmnuitem_, false, false );
	    mAddMenuItem( trackmnu, &changesetupmnuitem_, true, false );
	    mAddMenuItem( trackmnu, &enabletrackingmnuitem_, true,
			  mps->isTrackingEnabled(mps->getTrackerID(emid_)) );
	}
	else
	{
	    mResetMenuItem( &starttrackmnuitem_ );
	    mResetMenuItem( &changesetupmnuitem_ );
	    mResetMenuItem( &enabletrackingmnuitem_ );
	}
    }
    else
    {
	mResetMenuItem( &starttrackmnuitem_ );
	mResetMenuItem( &changesetupmnuitem_ );
	mResetMenuItem( &enabletrackingmnuitem_ );
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
    mResetMenuItem( &createflatscenemnuitem_ );
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
	    const bool res = uiMSG().askSave(
		    "Geometry has been changed. Saved 'Surface Data' is\n"
		    "not valid anymore and will be removed now.\n"
		    "Continue saving?" );
	    if ( !res )
		return;
	}

	bool savewithname = EM::EMM().getMultiID( emid_ ).isEmpty();
	if ( !savewithname )
	{
	    PtrMan<IOObj> ioobj = IOM().get( EM::EMM().getMultiID(emid_) );
	    savewithname = !ioobj;
	}
	applMgr()->EMServer()->storeObject( emid_, savewithname );
	applMgr()->visServer()->setObjectName( displayid_,
		(const char*) ems->getName(emid_) );
	const MultiID mid = ems->getStorageID(emid_);
	mps->saveSetup( mid );
	updateColumnText( uiODSceneMgr::cNameColumn() );
    }
    else if ( mnuid==saveasmnuitem_.id )
    {
	menu->setIsHandled(true);
	const MultiID oldmid = ems->getStorageID(emid_);
	mps->prepareSaveSetupAs( oldmid );

	MultiID storedmid;
	ems->storeObject( emid_, true, storedmid );
	applMgr()->visServer()->setObjectName( displayid_,
		(const char*) ems->getName(emid_) );

	const MultiID midintree = ems->getStorageID(emid_);
	EM::EMM().getObject(emid_)->setMultiID( storedmid );
	mps->saveSetupAs( storedmid );
	EM::EMM().getObject(emid_)->setMultiID( midintree );

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
	mps->showSetupDlg( emid_, sectionid );
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
    else if ( mnuid==enabletrackingmnuitem_.id )
    {
	menu->setIsHandled(true);
	const int trackerid = mps->getTrackerID(emid_);
	mps->enableTracking( trackerid, !mps->isTrackingEnabled(trackerid) );
	applMgr()->visServer()->updateMPEToolbar();
    }
    else if ( mnuid==createflatscenemnuitem_.id )
    {
    	mDynamicCastGet(visSurvey::EMObjectDisplay*,
			emd,visserv_->getObject(displayid_));
	const EM::ObjectID objectid = emd->getObjectID();
	mDynamicCastGet(const EM::Horizon*,horizon,
			EM::EMM().getObject( objectid ) );

	if ( !horizon ) return;

	BufferString scenenm = "Flattened on '";
	scenenm += horizon->name(); scenenm += "'";

	RefMan<ZAxisTransform> transform = new EM::HorizonZTransform(horizon);
	const int sceneid = ODMainWin()->sceneMgr().addScene( false, transform,
							      scenenm.buf() );
	ODMainWin()->sceneMgr().viewAll( 0 );
	ODMainWin()->sceneMgr().tile();
    }
}
