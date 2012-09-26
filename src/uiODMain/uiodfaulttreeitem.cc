/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author: 	K. Tingdahl
 Date: 		Jul 2003
___________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiodfaulttreeitem.h"

#include "datapointset.h"
#include "uimpepartserv.h"
#include "uivisemobj.h"
#include "visfaultdisplay.h"
#include "visfaultsticksetdisplay.h"
#include "emfaultstickset.h"
#include "emfault3d.h"
#include "emfaultauxdata.h"
#include "emmanager.h"
#include "mpeengine.h"
#include "ioman.h"
#include "ioobj.h"

#include "mousecursor.h"
#include "randcolor.h"
#include "uiempartserv.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uivispartserv.h"


uiODFaultParentTreeItem::uiODFaultParentTreeItem()
   : uiODTreeItem( "Fault" )
{}

#define mAddMnuID	0
#define mNewMnuID	1

#define mDispInFull	2
#define mDispAtSect	3
#define mDispAtHors	4
#define mDispAtBoth	5
#define mDispPlanes	6
#define mDispSticks	7
#define mDispPSBoth	8


#define mInsertItm( menu, name, id, enable ) \
{ \
    uiMenuItem* itm = new uiMenuItem( name ); \
    menu->insertItem( itm, id ); \
    itm->setEnabled( enable ); \
}

bool uiODFaultParentTreeItem::showSubMenu()
{
    mDynamicCastGet(visSurvey::Scene*,scene,
		    ODMainWin()->applMgr().visServer()->getObject(sceneID()));
    if ( scene && scene->getZAxisTransform() )
    {
	//uiMSG().message( "Cannot add Faults to this scene" );
	//return false;
    }

    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("&Add ..."), mAddMnuID );
    mnu.insertItem( new uiMenuItem("&New ..."), mNewMnuID );

    if ( children_.size() )
    {
	bool candispatsect = false;
	bool candispathors = false;
	for ( int idx=0; idx<children_.size(); idx++ )
	{
	    mDynamicCastGet( uiODFaultTreeItem*, itm, children_[idx] );
	    mDynamicCastGet( visSurvey::FaultDisplay*, fd,
			applMgr()->visServer()->getObject(itm->displayID()) );

	    if ( fd && fd->canDisplayIntersections() )
		candispatsect = true;
	    if ( fd && fd->canDisplayHorizonIntersections() )
		candispathors = true;
	}

	mnu.insertSeparator();
	uiPopupMenu* dispmnu = new uiPopupMenu( getUiParent(), "&Display all" );

	mInsertItm( dispmnu, "&In full", mDispInFull, true );
	mInsertItm( dispmnu, "&Only at sections", mDispAtSect, candispatsect );
	mInsertItm( dispmnu, "Only at &horizons", mDispAtHors, candispathors );
	mInsertItm( dispmnu, "&At sections && horizons", mDispAtBoth,
					    candispatsect && candispathors );
	dispmnu->insertSeparator();
	mInsertItm( dispmnu, "Fault &planes", mDispPlanes, true );
	mInsertItm( dispmnu, "Fault &sticks", mDispSticks, true );
	mInsertItm( dispmnu, "&Fault planes && sticks", mDispPSBoth, true );
	mnu.insertItem( dispmnu );
    }

    addStandardItems( mnu );

    const int mnuid = mnu.exec();
    if ( mnuid==mAddMnuID )
    {
	ObjectSet<EM::EMObject> objs;
	applMgr()->EMServer()->selectFaults( objs, false );
	MouseCursorChanger uics( MouseCursor::Wait );
	for ( int idx=0; idx<objs.size(); idx++ )
	    addChild( new uiODFaultTreeItem(objs[idx]->id()), false );

	deepUnRef( objs );
    }
    else if ( mnuid == mNewMnuID )
    {
	RefMan<EM::EMObject> emo =
	    EM::EMM().createTempObject( EM::Fault3D::typeStr() );
	if ( !emo )
	    return false;

	emo->setPreferredColor( getRandomColor(false) );
	emo->setNewName();
	emo->setFullyLoaded( true );
	addChild( new uiODFaultTreeItem( emo->id() ), false );
	return true;
    }
    else if ( mnuid>=mDispInFull && mnuid<=mDispPSBoth )
    {
	for ( int idx=0; idx<children_.size(); idx++ )
	{
	    mDynamicCastGet( uiODFaultTreeItem*, itm, children_[idx] );
	    mDynamicCastGet( visSurvey::FaultDisplay*, fd,
			applMgr()->visServer()->getObject(itm->displayID()) );
	    if ( !fd ) continue;

	    if ( mnuid>=mDispPlanes && mnuid<=mDispPSBoth )
		fd->display( mnuid!=mDispPlanes, mnuid!=mDispSticks );

	    if ( mnuid>=mDispInFull && mnuid<=mDispAtBoth )
	    {
		const bool atboth = mnuid==mDispAtBoth;
		fd->displayIntersections( mnuid==mDispAtSect || atboth );
		fd->displayHorizonIntersections( mnuid==mDispAtHors || atboth );
	    }
	}
    }
    else
	handleStandardItems( mnuid );

    return true;
}


uiTreeItem* uiODFaultTreeItemFactory::createForVis(int visid, uiTreeItem*) const
{
    mDynamicCastGet(visSurvey::FaultDisplay*,fd,
	    ODMainWin()->applMgr().visServer()->getObject(visid));
    return fd ? new uiODFaultTreeItem( visid, true ) : 0;
}


#define mCommonInit \
    , savemnuitem_("&Save") \
    , saveasmnuitem_("Save as ...") \
    , displayplanemnuitem_ ( "Fault &planes" ) \
    , displaystickmnuitem_ ( "Fault &sticks" ) \
    , displayintersectionmnuitem_( "&Only at sections" ) \
    , displayintersecthorizonmnuitem_( "Only at &horizons" ) \
    , singlecolmnuitem_( "Use single &color" ) \

#define mCommonInit2 \
    displayplanemnuitem_.checkable = true; \
    displaystickmnuitem_.checkable = true; \
    displayintersectionmnuitem_.checkable = true; \
    displayintersecthorizonmnuitem_.checkable = true; \
    singlecolmnuitem_.checkable = true; \
    savemnuitem_.iconfnm = "save"; \
    saveasmnuitem_.iconfnm = "saveas"; \



uiODFaultTreeItem::uiODFaultTreeItem( const EM::ObjectID& oid )
    : uiODDisplayTreeItem()
    , emid_( oid )
    mCommonInit
{
    mCommonInit2
}


uiODFaultTreeItem::uiODFaultTreeItem( int id, bool dummy )
    : uiODDisplayTreeItem()
    , emid_(-1)
    , uivisemobj_(0)  
    , faultdisplay_(0)
    mCommonInit
{
    displayid_ = id;
    mCommonInit2
}


uiODFaultTreeItem::~uiODFaultTreeItem()
{
    if ( faultdisplay_ )
    {
	faultdisplay_->materialChange()->remove(
	    mCB(this,uiODFaultTreeItem,colorChCB));
	faultdisplay_->selection()->remove(
		mCB(this,uiODFaultTreeItem,selChgCB) );
	faultdisplay_->deSelection()->remove(
		mCB(this,uiODFaultTreeItem,deSelChgCB) );
	faultdisplay_->unRef();
    }
}


uiODDataTreeItem* uiODFaultTreeItem::createAttribItem(
	const Attrib::SelSpec* as ) const
{
    const char* parenttype = typeid(*this).name();
    uiODDataTreeItem* res = as
	? uiODDataTreeItem::factory().create( 0, *as, parenttype, false) : 0;
    if ( !res ) 
	res = new uiODFaultSurfaceDataTreeItem( emid_, uivisemobj_, parenttype);
    return res;
}


bool uiODFaultTreeItem::init()
{
    if ( displayid_==-1 )
    {
	visSurvey::FaultDisplay* fd = visSurvey::FaultDisplay::create();
	displayid_ = fd->id();
	faultdisplay_ = fd;
	faultdisplay_->ref();

	fd->setEMID( emid_ );
	visserv_->addObject( fd, sceneID(), true );
    }
    else
    {
	mDynamicCastGet( visSurvey::FaultDisplay*, fd,
			 visserv_->getObject(displayid_) );
	if ( !fd )
	    return false;

	faultdisplay_ = fd;
	faultdisplay_->ref();
	emid_ = fd->getEMID();
    }

    faultdisplay_->materialChange()->notify(
	    mCB(this,uiODFaultTreeItem,colorChCB));
    faultdisplay_->selection()->notify(
	    mCB(this,uiODFaultTreeItem,selChgCB) );
    faultdisplay_->deSelection()->notify(
	    mCB(this,uiODFaultTreeItem,deSelChgCB) );

    return uiODDisplayTreeItem::init();
}


void uiODFaultTreeItem::colorChCB( CallBacker* )
{
    updateColumnText( uiODSceneMgr::cColorColumn() );
}


void uiODFaultTreeItem::selChgCB( CallBacker* )
{ MPE::engine().setActiveFaultObjID( emid_ ); }


void uiODFaultTreeItem::deSelChgCB( CallBacker* )
{ MPE::engine().setActiveFaultObjID( -1 ); }


bool uiODFaultTreeItem::askContinueAndSaveIfNeeded( bool withcancel )
{
    return applMgr()->EMServer()->askUserToSave( emid_, withcancel );
}


void uiODFaultTreeItem::prepareForShutdown()
{
    if ( faultdisplay_ )
    {
	faultdisplay_->materialChange()->remove(
	    mCB(this,uiODFaultTreeItem,colorChCB));
	faultdisplay_->unRef();
    }

    faultdisplay_ = 0;
    uiODDisplayTreeItem::prepareForShutdown();
}


void uiODFaultTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODDisplayTreeItem::createMenu( menu, istb );
    if ( !menu || menu->menuID()!=displayID() || istb )
	return;

    mDynamicCastGet(visSurvey::FaultDisplay*,fd,
	    ODMainWin()->applMgr().visServer()->getObject(displayID()));
    if ( !fd )
	return;

    mAddMenuItem( &displaymnuitem_, &displayintersectionmnuitem_,
		  faultdisplay_->canDisplayIntersections(),
		  faultdisplay_->areIntersectionsDisplayed() );
    mAddMenuItem( &displaymnuitem_, &displayintersecthorizonmnuitem_,
		  faultdisplay_->canDisplayHorizonIntersections(),
		  faultdisplay_->areHorizonIntersectionsDisplayed() );
    mAddMenuItem( &displaymnuitem_, &displayplanemnuitem_, true,
		  faultdisplay_->arePanelsDisplayed() );
    mAddMenuItem( &displaymnuitem_, &displaystickmnuitem_, true,
		  faultdisplay_->areSticksDisplayed() );
    mAddMenuItem( menu, &displaymnuitem_, true, true );

    mAddMenuItem( &displaymnuitem_, &singlecolmnuitem_,
		  faultdisplay_->arePanelsDisplayedInFull(),
		  !faultdisplay_->showingTexture() );

    const bool enablesave = applMgr()->EMServer()->isChanged(emid_) &&
			    applMgr()->EMServer()->isFullyLoaded(emid_);

    mAddMenuItem( menu, &savemnuitem_, enablesave, false );
    mAddMenuItem( menu, &saveasmnuitem_, true, false );
}


void uiODFaultTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( menu->isHandled() || menu->menuID()!=displayID() || mnuid==-1 )
	return;

    if ( mnuid==saveasmnuitem_.id ||  mnuid==savemnuitem_.id )
    {
	menu->setIsHandled(true);
	bool saveas = mnuid==saveasmnuitem_.id ||
		      applMgr()->EMServer()->getStorageID(emid_).isEmpty();
	if ( !saveas )
	{
	    PtrMan<IOObj> ioobj =
		IOM().get( applMgr()->EMServer()->getStorageID(emid_) );
	    saveas = !ioobj;
	}

	applMgr()->EMServer()->storeObject( emid_, saveas );

	if ( saveas && faultdisplay_ &&
	     !applMgr()->EMServer()->getName(emid_).isEmpty() )
	{
	    faultdisplay_->setName( applMgr()->EMServer()->getName(emid_));
	    updateColumnText( uiODSceneMgr::cNameColumn() );
	}
    }
    else if ( mnuid==displayplanemnuitem_.id )
    {
	menu->setIsHandled(true);
	const bool stickchecked = displaystickmnuitem_.checked;
	const bool planechecked = displayplanemnuitem_.checked;
	faultdisplay_->display( stickchecked || planechecked, !planechecked );
    }
    else if ( mnuid==displaystickmnuitem_.id )
    {
	menu->setIsHandled(true);
	const bool stickchecked = displaystickmnuitem_.checked;
	const bool planechecked = displayplanemnuitem_.checked;
	faultdisplay_->display( !stickchecked, stickchecked || planechecked );
    }
    else if ( mnuid==displayintersectionmnuitem_.id )
    {
	menu->setIsHandled(true);
	const bool interchecked = displayintersectionmnuitem_.checked;
	faultdisplay_->displayIntersections( !interchecked );
    }
    else if ( mnuid==displayintersecthorizonmnuitem_.id )
    {
	menu->setIsHandled(true);
	const bool interchecked = displayintersecthorizonmnuitem_.checked;
	faultdisplay_->displayHorizonIntersections( !interchecked );
    }
    else if ( mnuid==singlecolmnuitem_.id )
    {
	menu->setIsHandled(true);
	faultdisplay_->useTexture( !faultdisplay_->showingTexture(), true );
	visserv_->triggerTreeUpdate();
    }
}



uiODFaultStickSetParentTreeItem::uiODFaultStickSetParentTreeItem()
   : uiODTreeItem( "FaultStickSet" )
{}


bool uiODFaultStickSetParentTreeItem::showSubMenu()
{
    mDynamicCastGet(visSurvey::Scene*,scene,
		    ODMainWin()->applMgr().visServer()->getObject(sceneID()));
    if ( scene && scene->getZAxisTransform() )
    {
	uiMSG().message( "Cannot add FaultStickSets to this scene" );
	return false;
    }

    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("&Add ..."), mAddMnuID );
    mnu.insertItem( new uiMenuItem("&New ..."), mNewMnuID );

    if ( children_.size() )
    {
	mnu.insertSeparator();
	uiPopupMenu* dispmnu = new uiPopupMenu( getUiParent(), "&Display all" );
	dispmnu->insertItem( new uiMenuItem("&In full"), mDispInFull );
	dispmnu->insertItem( new uiMenuItem("&Only at sections"), mDispAtSect );
	mnu.insertItem( dispmnu );
    }

    addStandardItems( mnu );

    const int mnuid = mnu.exec();
    if ( mnuid==mAddMnuID )
    {
	ObjectSet<EM::EMObject> objs;
	applMgr()->EMServer()->selectFaultStickSets( objs );
	MouseCursorChanger uics( MouseCursor::Wait );
	for ( int idx=0; idx<objs.size(); idx++ )
	    addChild( new uiODFaultStickSetTreeItem(objs[idx]->id()), false );
	deepUnRef( objs );
    }
    else if ( mnuid == mNewMnuID )
    {
	//applMgr()->mpeServer()->saveUnsaveEMObject();
	RefMan<EM::EMObject> emo =
	    EM::EMM().createTempObject( EM::FaultStickSet::typeStr() );
	if ( !emo )
	    return false;

	emo->setPreferredColor( getRandomColor(false) );
	emo->setNewName();
	emo->setFullyLoaded( true );
	addChild( new uiODFaultStickSetTreeItem( emo->id() ), false );
	return true;
    }
    else if ( mnuid==mDispInFull || mnuid==mDispAtSect )
    {
	for ( int idx=0; idx<children_.size(); idx++ )
	{
	    mDynamicCastGet( uiODFaultStickSetTreeItem*, itm, children_[idx] );
	    mDynamicCastGet( visSurvey::FaultStickSetDisplay*, fssd,
			applMgr()->visServer()->getObject(itm->displayID()) );
	    if ( fssd )
		fssd->setDisplayOnlyAtSections( mnuid==mDispAtSect );
	}
    }
    else
	handleStandardItems( mnuid );

    return true;
}


uiTreeItem*
uiODFaultStickSetTreeItemFactory::createForVis( int visid, uiTreeItem* ) const
{
    mDynamicCastGet(visSurvey::FaultStickSetDisplay*,fd,
	    ODMainWin()->applMgr().visServer()->getObject(visid));
    return fd ? new uiODFaultStickSetTreeItem( visid, true ) : 0;
}


#undef mCommonInit
#undef mCommonInit2
#define mCommonInit \
    , faultsticksetdisplay_(0) \
    , savemnuitem_("&Save") \
    , saveasmnuitem_("Save &as ...") \
    , onlyatsectmnuitem_("&Only at sections")

#define mCommonInit2 \
    onlyatsectmnuitem_.checkable = true; \
    savemnuitem_.iconfnm = "save"; \
    saveasmnuitem_.iconfnm = "saveas";


uiODFaultStickSetTreeItem::uiODFaultStickSetTreeItem( const EM::ObjectID& oid )
    : uiODDisplayTreeItem()
    , emid_( oid )
    mCommonInit
{
    mCommonInit2
}


uiODFaultStickSetTreeItem::uiODFaultStickSetTreeItem( int id, bool dummy )
    : uiODDisplayTreeItem()
    , emid_(-1)
    mCommonInit
{
    displayid_ = id;
    mCommonInit2
}


uiODFaultStickSetTreeItem::~uiODFaultStickSetTreeItem()
{
    if ( faultsticksetdisplay_ )
    {
	faultsticksetdisplay_->materialChange()->remove(
	    mCB(this,uiODFaultStickSetTreeItem,colorChCB) );
	faultsticksetdisplay_->selection()->remove(
		mCB(this,uiODFaultStickSetTreeItem,selChgCB) );
	faultsticksetdisplay_->deSelection()->remove(
		mCB(this,uiODFaultStickSetTreeItem,deSelChgCB) );
	faultsticksetdisplay_->unRef();
    }
}


bool uiODFaultStickSetTreeItem::init()
{
    if ( displayid_==-1 )
    {
	visSurvey::FaultStickSetDisplay* fd =
				    visSurvey::FaultStickSetDisplay::create();
	displayid_ = fd->id();
	faultsticksetdisplay_ = fd;
	faultsticksetdisplay_->ref();

	fd->setEMID( emid_ );
	visserv_->addObject( fd, sceneID(), true );
    }
    else
    {
	mDynamicCastGet(visSurvey::FaultStickSetDisplay*,fd,
			visserv_->getObject(displayid_));
	if ( !fd )
	    return false;

	faultsticksetdisplay_ = fd;
	faultsticksetdisplay_->ref();
	emid_ = fd->getEMID();
    }

    faultsticksetdisplay_->materialChange()->notify(
	    mCB(this,uiODFaultStickSetTreeItem,colorChCB) );
    faultsticksetdisplay_->selection()->notify(
	    mCB(this,uiODFaultStickSetTreeItem,selChgCB) );
    faultsticksetdisplay_->deSelection()->notify(
	    mCB(this,uiODFaultStickSetTreeItem,deSelChgCB) );
    		
    return uiODDisplayTreeItem::init();
}


void uiODFaultStickSetTreeItem::colorChCB( CallBacker* )
{
    updateColumnText( uiODSceneMgr::cColorColumn() );
}


void uiODFaultStickSetTreeItem::selChgCB( CallBacker* )
{ MPE::engine().setActiveFSSObjID( emid_ ); }


void uiODFaultStickSetTreeItem::deSelChgCB( CallBacker* )
{ MPE::engine().setActiveFSSObjID( -1 ); }


bool uiODFaultStickSetTreeItem::askContinueAndSaveIfNeeded( bool withcancel )
{
    return applMgr()->EMServer()->askUserToSave( emid_, withcancel );
}


void uiODFaultStickSetTreeItem::prepareForShutdown()
{
    if ( faultsticksetdisplay_ )
    {
	faultsticksetdisplay_->materialChange()->remove(
	    mCB(this,uiODFaultStickSetTreeItem,colorChCB) );
	faultsticksetdisplay_->unRef();
    }

    faultsticksetdisplay_ = 0;
    uiODDisplayTreeItem::prepareForShutdown();
}


void uiODFaultStickSetTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODDisplayTreeItem::createMenu( menu, istb );
    if ( !menu || menu->menuID()!=displayID() || istb )
	return;

    mDynamicCastGet(visSurvey::FaultStickSetDisplay*,fd,
	    ODMainWin()->applMgr().visServer()->getObject(displayID()));
    if ( !fd )
	return;

    mAddMenuItem( menu, &displaymnuitem_, true, false );
    mAddMenuItem( &displaymnuitem_, &onlyatsectmnuitem_,
		  true, fd->displayedOnlyAtSections() );

    const bool enablesave = applMgr()->EMServer()->isChanged(emid_) &&
			    applMgr()->EMServer()->isFullyLoaded(emid_);
    mAddMenuItem( menu, &savemnuitem_, enablesave, false );
    mAddMenuItem( menu, &saveasmnuitem_, true, false );
}


void uiODFaultStickSetTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( menu->isHandled() || menu->menuID()!=displayID() || mnuid==-1 )
	return;

    mDynamicCastGet(visSurvey::FaultStickSetDisplay*,fd,
	    ODMainWin()->applMgr().visServer()->getObject(displayID()));

    if ( mnuid==onlyatsectmnuitem_.id )
    {
	menu->setIsHandled(true);
	if ( fd )
	    fd->setDisplayOnlyAtSections( !fd->displayedOnlyAtSections() );
    }
    else if ( mnuid==saveasmnuitem_.id ||  mnuid==savemnuitem_.id )
    {
	menu->setIsHandled(true);
	bool saveas = mnuid==saveasmnuitem_.id ||
		      applMgr()->EMServer()->getStorageID(emid_).isEmpty();
	if ( !saveas )
	{
	    PtrMan<IOObj> ioobj =
		IOM().get( applMgr()->EMServer()->getStorageID(emid_) );
	    saveas = !ioobj;
	}

	applMgr()->EMServer()->storeObject( emid_, saveas );

	const BufferString emname = applMgr()->EMServer()->getName(emid_);
	if ( saveas && faultsticksetdisplay_ && !emname.isEmpty() )
	{
	    faultsticksetdisplay_->setName( emname );
	    updateColumnText( uiODSceneMgr::cNameColumn() );
	}
    }
}

uiODFaultSurfaceDataTreeItem::uiODFaultSurfaceDataTreeItem( EM::ObjectID objid,
	uiVisEMObject* uv, const char* parenttype )
    : uiODAttribTreeItem(parenttype)
    , depthattribmnuitem_("Z values")  
    , savesurfacedatamnuitem_("Save as Fault Data ...")
    , loadsurfacedatamnuitem_("Fault Data ...")
    , algomnuitem_("&Smooth")
    , changed_(false)
    , emid_(objid)
    , uivisemobj_(uv)
{}


void uiODFaultSurfaceDataTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODAttribTreeItem::createMenu( menu, istb );
    if ( istb ) return;

    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    const Attrib::SelSpec* as = visserv->getSelSpec( displayID(), attribNr() );
    const bool islocked = visserv->isLocked( displayID() );
    mDynamicCastGet( visSurvey::FaultDisplay*, fd, 
	    visserv->getObject(displayID()) );
    const int nrsurfdata = fd ? fd->emFault()->auxdata.auxDataList().size() : 0;
	//applMgr()->EMServer()->nrAttributes( emid_ );
    BufferString itmtxt = "Fault Data ("; 
    itmtxt += nrsurfdata; itmtxt += ") ...";
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

    const bool enabsave = changed_ ||
	(as && as->id()!=Attrib::SelSpec::cNoAttrib() &&
	 as->id()!=Attrib::SelSpec::cAttribNotSel() );

    mAddMenuItem( menu, &savesurfacedatamnuitem_, enabsave, false );
    mAddMenuItem( menu, &algomnuitem_, true, false );
}


void uiODFaultSurfaceDataTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODAttribTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( mnuid==-1 || menu->isHandled() )
	return;

    uiMSG().message("Not complete yet");
    return;
    
    const int visid = displayID();
    const int attribnr = attribNr();

    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    if ( mnuid==savesurfacedatamnuitem_.id )
    {
	menu->setIsHandled( true );
	DataPointSet vals( false, true );
	vals.bivSet().setNrVals( 3 );
	visserv->getRandomPosCache( visid, attribnr, vals );
	if ( vals.size() )
	{
	    BufferString auxdatanm;
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
	changed_ = false;
    }
    else if ( mnuid==loadsurfacedatamnuitem_.id )
    {
	menu->setIsHandled( true );
	if ( !applMgr()->EMServer()->showLoadAuxDataDlg(emid_) )
	    return;
	
	TypeSet<float> shifts;
	DataPointSet vals( false, true );
	applMgr()->EMServer()->getAllAuxData( emid_, vals, &shifts );
	setDataPointSet( vals );
	
	//mDynamicCastGet( visSurvey::FaultDisplay*, visflt,
	//	visserv->getObject(visid) );
	//visflt->setAttribShift( attribnr, shifts );
	
	updateColumnText( uiODSceneMgr::cNameColumn() );
	changed_ = false;
    }
    else
    {
    }
}


void uiODFaultSurfaceDataTreeItem::setDataPointSet( const DataPointSet& vals )
{
    const int visid = displayID();
    const int attribnr = attribNr();
    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    FixedString attrnm = vals.nrCols()>1 ? vals.colName(1) : "";
    visserv->setSelSpec( visid, attribnr,
	    Attrib::SelSpec(attrnm,Attrib::SelSpec::cOtherAttrib()) );
    visserv->createAndDispDataPack( visid, attribnr, &vals );
    visserv->selectTexture( visid, attribnr, 0 );
    updateColumnText( uiODSceneMgr::cNameColumn() );
}


BufferString uiODFaultSurfaceDataTreeItem::createDisplayName() const
{
    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    const Attrib::SelSpec* as = visserv->getSelSpec( displayID(), attribNr() );
    
    if ( as->id().asInt()==Attrib::SelSpec::cNoAttrib().asInt() )
	return BufferString("Z values");
    
    return uiODAttribTreeItem::createDisplayName();
}

