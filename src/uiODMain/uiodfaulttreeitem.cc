/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jul 2003
___________________________________________________________________

-*/

#include "uiodfaulttreeitem.h"

#include "datapointset.h"
#include "emfault3d.h"
#include "emfaultauxdata.h"
#include "emfaultstickset.h"
#include "emmanager.h"
#include "ioman.h"
#include "ioobj.h"
#include "mousecursor.h"
#include "mpeengine.h"
#include "randcolor.h"
#include "threadwork.h"

#include "uiempartserv.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uimpepartserv.h"
#include "uimsg.h"
#include "uinotsaveddlg.h"
#include "uiodapplmgr.h"
#include "uiodmenumgr.h"
#include "uiodscenemgr.h"
#include "uiodviewer2dmgr.h"
#include "uistrings.h"
#include "uitreeview.h"
#include "uivispartserv.h"

#include "visfaultdisplay.h"
#include "visfaultsticksetdisplay.h"


CNotifier<uiODFaultParentTreeItem,uiMenu*>&
	uiODFaultParentTreeItem::showMenuNotifier()
{
    static CNotifier<uiODFaultParentTreeItem,uiMenu*> notif( nullptr );
    return notif;
}


uiODFaultParentTreeItem::uiODFaultParentTreeItem()
    : uiODParentTreeItem( uiStrings::sFault() )
{
}


uiODFaultParentTreeItem::~uiODFaultParentTreeItem()
{
}


const char* uiODFaultParentTreeItem::iconName() const
{ return "tree-flt"; }


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
    uiAction* itm = new uiAction( name ); \
    menu->insertAction( itm, id ); \
    itm->setEnabled( enable ); \
}

bool uiODFaultParentTreeItem::showSubMenu()
{
    mDynamicCastGet(visSurvey::Scene*,scene,
		    ODMainWin()->applMgr().visServer()->getObject(sceneID()));
    const bool hastransform = scene && scene->getZAxisTransform();
    if ( hastransform )
    {
	uiMSG().message( tr("Cannot add Faults to this scene") );
	return false;
    }

    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertAction( new uiAction(m3Dots(uiStrings::sAdd())), mAddMnuID );
    uiAction* newmenu = new uiAction( uiStrings::sNew() );
    mnu.insertAction( newmenu, mNewMnuID );
    newmenu->setEnabled( !hastransform && SI().has3D() );
    showMenuNotifier().trigger( &mnu, this );

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
	uiMenu* dispmnu = new uiMenu( getUiParent(), tr("Display all") );

	mInsertItm( dispmnu, tr("In full"), mDispInFull, true );
	mInsertItm( dispmnu, tr("Only at Sections"), mDispAtSect,
		    candispatsect );
	mInsertItm( dispmnu, tr("Only at Horizons"), mDispAtHors,
		    candispathors );
	mInsertItm( dispmnu, tr("At Sections && Horizons"), mDispAtBoth,
					    candispatsect && candispathors );
	dispmnu->insertSeparator();
	mInsertItm( dispmnu, tr("Fault Planes"), mDispPlanes, true );
	mInsertItm( dispmnu, tr("Fault Sticks"), mDispSticks, true );
	mInsertItm( dispmnu, tr("Fault Planes && Sticks"), mDispPSBoth, true );
	mnu.addMenu( dispmnu );
    }

    addStandardItems( mnu );

    const int mnuid = mnu.exec();
    if ( mnuid==mAddMnuID )
    {
	ObjectSet<EM::EMObject> objs;
	applMgr()->EMServer()->selectFaults( objs, false );
	MouseCursorChanger mcc( MouseCursor::Wait );
	for ( int idx=0; idx<objs.size(); idx++ )
	{
	    setMoreObjectsToDoHint( idx<objs.size()-1 );
	    addChild( new uiODFaultTreeItem(objs[idx]->id()), false );
	}

	deepUnRef( objs );
    }
    else if ( mnuid == mNewMnuID )
    {
	RefMan<EM::EMObject> emo =
	    EM::EMM().createTempObject( EM::Fault3D::typeStr() );
	if ( !emo )
	    return false;

	emo->setPreferredColor( OD::getRandomColor(false) );
	emo->setNewName();
	emo->setFullyLoaded( true );
	addChild( new uiODFaultTreeItem( emo->id() ), false );
	applMgr()->viewer2DMgr().addNewTempFault( emo->id() );
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


uiTreeItem* uiODFaultTreeItemFactory::createForVis(
					VisID visid, uiTreeItem*) const
{
    mDynamicCastGet(visSurvey::FaultDisplay*,fd,
	    ODMainWin()->applMgr().visServer()->getObject(visid));
    return fd ? new uiODFaultTreeItem( visid, true ) : 0;
}


#define mCommonInit \
    , savemnuitem_( uiStrings::sSave() ) \
    , saveasmnuitem_( m3Dots(uiStrings::sSaveAs() )) \
    , displayplanemnuitem_( uiODFaultTreeItem::sFaultPlanes() ) \
    , displaystickmnuitem_( uiODFaultTreeItem::sFaultSticks() ) \
    , displayintersectionmnuitem_( uiODFaultTreeItem::sOnlyAtSections() ) \
    , displayintersecthorizonmnuitem_( uiODFaultTreeItem::sOnlyAtHorizons() ) \
    , singlecolmnuitem_( uiStrings::sUseSingleColor() ) \

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

    mAttachCB( NotSavedPrompter::NSP().promptSaving,
	       uiODFaultTreeItem::askSaveCB );
}


uiODFaultTreeItem::uiODFaultTreeItem( VisID id, bool dummy )
    : uiODDisplayTreeItem()
    , faultdisplay_(0)
    mCommonInit
{
    displayid_ = id;
    mCommonInit2

    mAttachCB( NotSavedPrompter::NSP().promptSaving,
	       uiODFaultTreeItem::askSaveCB );
}


uiODFaultTreeItem::~uiODFaultTreeItem()
{
    detachAllNotifiers();
    if ( faultdisplay_ )
	faultdisplay_->unRef();
}


void uiODFaultTreeItem::askSaveCB( CallBacker* )
{
    uiEMPartServer* ems = applMgr()->EMServer();
    if ( !ems->isChanged(emid_) )
	return;

    bool savewithname = EM::EMM().getMultiID( emid_ ).isUdf();
    if ( !savewithname )
	savewithname = !IOM().implExists( EM::EMM().getMultiID(emid_) );

    const uiString obj = toUiString("%1 \"%2\"")
	.arg( ems->getType(emid_) ).arg( ems->getUiName(emid_) );
    NotSavedPrompter::NSP().addObject( obj, mCB(this,uiODFaultTreeItem,saveCB),
				       savewithname, 0 );

    Threads::WorkManager::twm().addWork(
	    Threads::Work( *new uiTreeItemRemover(parent_, this), true ), 0,
	    NotSavedPrompter::NSP().queueID(), false );
}


void uiODFaultTreeItem::saveCB( CallBacker* cb )
{
    const bool issaved = applMgr()->EMServer()->storeObject( emid_, true );
    if ( issaved && faultdisplay_ &&
	 !applMgr()->EMServer()->getUiName(emid_).isEmpty() )
    {
	faultdisplay_->setName( applMgr()->EMServer()->getName(emid_) );
	updateColumnText( uiODSceneMgr::cNameColumn() );
    }

    if ( issaved && cb )
	NotSavedPrompter::NSP().reportSuccessfullSave();
}


uiODDataTreeItem* uiODFaultTreeItem::createAttribItem(
	const Attrib::SelSpec* as ) const
{
    const char* parenttype = typeid(*this).name();
    uiODDataTreeItem* res = as
	? uiODDataTreeItem::factory().create( 0, *as, parenttype, false) : 0;
    if ( !res )
	res = new uiODFaultSurfaceDataTreeItem( emid_, parenttype );
    return res;
}


bool uiODFaultTreeItem::init()
{
    if ( !displayid_.isValid() )
    {
	visSurvey::FaultDisplay* fd = new visSurvey::FaultDisplay;
	displayid_ = fd->id();
	faultdisplay_ = fd;
	faultdisplay_->ref();

	fd->setEMObjectID( emid_ );
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
	emid_ = fd->getEMObjectID();
    }

    ODMainWin()->menuMgr().createFaultToolMan();

    mAttachCB( faultdisplay_->materialChange(),
	       uiODFaultTreeItem::colorChgCB );

    return uiODDisplayTreeItem::init();
}


bool uiODFaultTreeItem::askContinueAndSaveIfNeeded( bool withcancel )
{
    return applMgr()->EMServer()->askUserToSave( emid_, withcancel );
}


void uiODFaultTreeItem::prepareForShutdown()
{
    if ( faultdisplay_ )
    {
	mDetachCB( faultdisplay_->materialChange(),
		   uiODFaultTreeItem::colorChgCB );
	faultdisplay_->unRef();
    }

    faultdisplay_ = nullptr;
    uiODDisplayTreeItem::prepareForShutdown();
}


void uiODFaultTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODDisplayTreeItem::createMenu( menu, istb );
    if ( !menu || !isDisplayID(menu->menuID()) )
	return;

    mDynamicCastGet(visSurvey::FaultDisplay*,fd,
	    ODMainWin()->applMgr().visServer()->getObject(displayID()));
    if ( !fd )
	return;

    if ( !istb )
    {
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
		      faultdisplay_->canShowTexture(),
		      !faultdisplay_->showsTexture() );
    }

    const MultiID mid = EM::EMM().getMultiID( emid_ );
    const bool enablesave = applMgr()->EMServer()->isChanged(emid_) &&
			    applMgr()->EMServer()->isFullyLoaded(emid_) &&
			    EM::canOverwrite(mid);

    mAddMenuOrTBItem( istb, menu, menu, &savemnuitem_, enablesave, false );
    mAddMenuOrTBItem( istb, 0, menu, &saveasmnuitem_, true, false );
}


void uiODFaultTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( menu->isHandled() || !isDisplayID(menu->menuID()) || mnuid==-1 )
	return;

    if ( mnuid==saveasmnuitem_.id ||  mnuid==savemnuitem_.id )
    {
	menu->setIsHandled(true);
	bool saveas = mnuid==saveasmnuitem_.id ||
		      applMgr()->EMServer()->getStorageID(emid_).isUdf();
	if ( !saveas )
	{
	    PtrMan<IOObj> ioobj =
		IOM().get( applMgr()->EMServer()->getStorageID(emid_) );
	    saveas = !ioobj;
	}

	applMgr()->EMServer()->storeObject( emid_, saveas );

	if ( saveas && faultdisplay_ &&
	     !applMgr()->EMServer()->getUiName(emid_).isEmpty() )
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
	faultdisplay_->useTexture( !faultdisplay_->showsTexture(), true );
	visserv_->triggerTreeUpdate();
    }
}


void uiODFaultTreeItem::setOnlyAtSectionsDisplay( bool yn )
{ uiODDisplayTreeItem::setOnlyAtSectionsDisplay( yn ); }


bool uiODFaultTreeItem::isOnlyAtSections() const
{ return uiODDisplayTreeItem::displayedOnlyAtSections(); }



// uiODFaultStickSetParentTreeItem
CNotifier<uiODFaultStickSetParentTreeItem,uiMenu*>&
	uiODFaultStickSetParentTreeItem::showMenuNotifier()
{
    static CNotifier<uiODFaultStickSetParentTreeItem,uiMenu*> notif( nullptr );
    return notif;
}


uiODFaultStickSetParentTreeItem::uiODFaultStickSetParentTreeItem()
    : uiODParentTreeItem( uiStrings::sFaultStickSet() )
{}


uiODFaultStickSetParentTreeItem::~uiODFaultStickSetParentTreeItem()
{}


const char* uiODFaultStickSetParentTreeItem::iconName() const
{ return "tree-fltss"; }


bool uiODFaultStickSetParentTreeItem::showSubMenu()
{
    mDynamicCastGet(visSurvey::Scene*,scene,
		    ODMainWin()->applMgr().visServer()->getObject(sceneID()));
    if ( scene && scene->getZAxisTransform() )
    {
	uiMSG().message( tr("Cannot add FaultStickSets to this scene") );
	return false;
    }

    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertAction( new uiAction(m3Dots(uiStrings::sAdd())), mAddMnuID );
    mnu.insertAction( new uiAction(uiStrings::sNew()), mNewMnuID );
    showMenuNotifier().trigger( &mnu, this );

    if ( children_.size() )
    {
	mnu.insertSeparator();
	uiMenu* dispmnu = new uiMenu( getUiParent(), tr("Display All") );
	dispmnu->insertAction( new uiAction(tr("In full")), mDispInFull );
	dispmnu->insertAction( new uiAction(tr("Only at Sections")),
			     mDispAtSect );
	mnu.addMenu( dispmnu );
    }

    addStandardItems( mnu );

    const int mnuid = mnu.exec();
    if ( mnuid==mAddMnuID )
    {
	ObjectSet<EM::EMObject> objs;
	applMgr()->EMServer()->selectFaultStickSets( objs );
	MouseCursorChanger uics( MouseCursor::Wait );
	for ( int idx=0; idx<objs.size(); idx++ )
	{
	    setMoreObjectsToDoHint( idx<objs.size()-1 );
	    addChild( new uiODFaultStickSetTreeItem(objs[idx]->id()), false );
	}
	deepUnRef( objs );
    }
    else if ( mnuid == mNewMnuID )
    {
	//applMgr()->mpeServer()->saveUnsaveEMObject();
	RefMan<EM::EMObject> emo =
	    EM::EMM().createTempObject( EM::FaultStickSet::typeStr() );
	if ( !emo )
	    return false;

	emo->setPreferredColor( OD::getRandomColor(false) );
	emo->setNewName();
	emo->setFullyLoaded( true );
	addChild( new uiODFaultStickSetTreeItem( emo->id() ), false );
	applMgr()->viewer2DMgr().addNewTempFaultSS( emo->id() );
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
		fssd->setOnlyAtSectionsDisplay( mnuid==mDispAtSect );
	}
    }
    else
	handleStandardItems( mnuid );

    return true;
}


uiTreeItem*
uiODFaultStickSetTreeItemFactory::createForVis( VisID visid, uiTreeItem* ) const
{
    mDynamicCastGet(visSurvey::FaultStickSetDisplay*,fd,
	    ODMainWin()->applMgr().visServer()->getObject(visid));
    return fd ? new uiODFaultStickSetTreeItem( visid, true ) : 0;
}


#undef mCommonInit
#undef mCommonInit2
#define mCommonInit \
    , faultsticksetdisplay_(0) \
    , savemnuitem_(uiStrings::sSave()) \
    , saveasmnuitem_(m3Dots(uiStrings::sSaveAs())) \
    , onlyatsectmnuitem_( uiODFaultTreeItem::sOnlyAtSections() )

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

    mAttachCB( NotSavedPrompter::NSP().promptSaving,
	       uiODFaultStickSetTreeItem::askSaveCB );
}


uiODFaultStickSetTreeItem::uiODFaultStickSetTreeItem( VisID id, bool dummy )
    : uiODDisplayTreeItem()
    mCommonInit
{
    displayid_ = id;
    mCommonInit2

    mAttachCB( NotSavedPrompter::NSP().promptSaving,
	       uiODFaultStickSetTreeItem::askSaveCB );
}


uiODFaultStickSetTreeItem::~uiODFaultStickSetTreeItem()
{
    detachAllNotifiers();
    if ( faultsticksetdisplay_ )
	faultsticksetdisplay_->unRef();
}


bool uiODFaultStickSetTreeItem::init()
{
    if ( !displayid_.isValid() )
    {
	visSurvey::FaultStickSetDisplay* fd =
				    new visSurvey::FaultStickSetDisplay;
	displayid_ = fd->id();
	faultsticksetdisplay_ = fd;
	faultsticksetdisplay_->ref();

	fd->setEMObjectID( emid_ );
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
	emid_ = fd->getEMObjectID();
    }

    ODMainWin()->menuMgr().createFaultToolMan();

    mAttachCB( faultsticksetdisplay_->materialChange(),
	       uiODFaultStickSetTreeItem::colorChCB );

    return uiODDisplayTreeItem::init();
}


void uiODFaultStickSetTreeItem::colorChCB( CallBacker* )
{
    updateColumnText( uiODSceneMgr::cColorColumn() );
}


void uiODFaultStickSetTreeItem::askSaveCB( CallBacker* )
{
    uiEMPartServer* ems = applMgr()->EMServer();
    if ( !ems->isChanged(emid_) )
	return;

    bool savewithname = EM::EMM().getMultiID( emid_ ).isUdf();
    if ( !savewithname )
	savewithname = !IOM().implExists( EM::EMM().getMultiID(emid_) );

    const uiString obj = toUiString("%1 \"%2\"")
	.arg( ems->getType(emid_) ).arg( ems->getUiName(emid_) );
    NotSavedPrompter::NSP().addObject( obj,
				mCB(this,uiODFaultStickSetTreeItem,saveCB),
				savewithname, 0 );

    Threads::WorkManager::twm().addWork(
	    Threads::Work( *new uiTreeItemRemover(parent_, this), true ), 0,
	    NotSavedPrompter::NSP().queueID(), false );
}


void uiODFaultStickSetTreeItem::saveCB( CallBacker* cb )
{
    const bool issaved = applMgr()->EMServer()->storeObject( emid_, true );
    if ( issaved && faultsticksetdisplay_ &&
	 !applMgr()->EMServer()->getUiName(emid_).isEmpty() )
    {
	faultsticksetdisplay_->setName( applMgr()->EMServer()->getName(emid_) );
	updateColumnText( uiODSceneMgr::cNameColumn() );
    }

    if ( issaved && cb )
	NotSavedPrompter::NSP().reportSuccessfullSave();
}


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
    if ( !menu || !isDisplayID(menu->menuID()) || istb )
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
    if ( menu->isHandled() || !isDisplayID(menu->menuID()) || mnuid==-1 )
	return;

    mDynamicCastGet(visSurvey::FaultStickSetDisplay*,fd,
	    ODMainWin()->applMgr().visServer()->getObject(displayID()));

    if ( mnuid==onlyatsectmnuitem_.id )
    {
	menu->setIsHandled(true);
	if ( fd )
	    fd->setOnlyAtSectionsDisplay( !fd->displayedOnlyAtSections() );
    }
    else if ( mnuid==saveasmnuitem_.id ||  mnuid==savemnuitem_.id )
    {
	menu->setIsHandled(true);
	bool saveas = mnuid==saveasmnuitem_.id ||
		      applMgr()->EMServer()->getStorageID(emid_).isUdf();
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



// uiODFaultSurfaceDataTreeItem
uiODFaultSurfaceDataTreeItem::uiODFaultSurfaceDataTreeItem( EM::ObjectID objid,
	const char* parenttype )
    : uiODAttribTreeItem(parenttype)
    , depthattribmnuitem_( uiStrings::sZValue(mPlural) )
    , savesurfacedatamnuitem_(m3Dots(tr("Save as Fault Data")))
    , loadsurfacedatamnuitem_(m3Dots(tr("Fault Data")))
    , algomnuitem_(tr("Smooth"))
    , changed_(false)
    , emid_(objid)
{}


uiODFaultSurfaceDataTreeItem::~uiODFaultSurfaceDataTreeItem()
{}


void uiODFaultSurfaceDataTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODAttribTreeItem::createMenu( menu, istb );
    if ( istb ) return;

    bool enablefaultdata = false;
#ifdef __debug__
    enablefaultdata = true;
#endif
    if ( !enablefaultdata )
    {
	mResetMenuItem( &loadsurfacedatamnuitem_ );
	mResetMenuItem( &depthattribmnuitem_ );
	mResetMenuItem( &savesurfacedatamnuitem_ );
	mResetMenuItem( &algomnuitem_ );
	return;
    }

    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    const Attrib::SelSpec* as = visserv->getSelSpec( displayID(), attribNr() );
    const bool islocked = visserv->isLocked( displayID() );
    mDynamicCastGet( visSurvey::FaultDisplay*, fd,
	    visserv->getObject(displayID()) );
    BufferStringSet nmlist;
    if ( fd && fd->emFault() && fd->emFault()->auxData() )
	fd->emFault()->auxData()->getAuxDataList( nmlist );
    const int nrsurfdata = nmlist.size();
    uiString itmtxt = m3Dots(tr("Fault Data (%1)").arg(nrsurfdata ));
    loadsurfacedatamnuitem_.text = itmtxt;

    mAddMenuItem( &selattrmnuitem_, &loadsurfacedatamnuitem_,
	    false && !islocked && nrsurfdata>0, false );

    mAddMenuItem( &selattrmnuitem_, &depthattribmnuitem_, !islocked,
	    as->id().asInt()==Attrib::SelSpec::cNoAttrib().asInt() );

    const bool enabsave = changed_ ||
	    (as && as->id()!=Attrib::SelSpec::cNoAttrib() &&
	     as->id()!=Attrib::SelSpec::cAttribNotSel() );
    mAddMenuItem( menu, &savesurfacedatamnuitem_, enabsave, false );
    mAddMenuItem( menu, &algomnuitem_, false, false );
}


void uiODFaultSurfaceDataTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODAttribTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( mnuid==-1 || menu->isHandled() )
	return;

    const VisID visid = displayID();
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

	mDynamicCastGet( visSurvey::FaultDisplay*, fltdisp,
		visserv->getObject(visid) );
	fltdisp->setDepthAsAttrib( attribnr );
	updateColumnText( uiODSceneMgr::cNameColumn() );
	updateColumnText( uiODSceneMgr::cColorColumn() );
	applMgr()->updateColorTable( displayID(), attribnr );
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
    else if ( mnuid==algomnuitem_.id )
    {
    }
}


void uiODFaultSurfaceDataTreeItem::setDataPointSet( const DataPointSet& vals )
{
    const VisID visid = displayID();
    const int attribnr = attribNr();
    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    StringView attrnm = vals.nrCols()>1 ? vals.colName(1) : "";
    visserv->setSelSpec( visid, attribnr,
	    Attrib::SelSpec(attrnm,Attrib::SelSpec::cOtherAttrib()) );
    visserv->setRandomPosData( visid, attribnr, &vals );
    visserv->selectTexture( visid, attribnr, 0 );
    updateColumnText( uiODSceneMgr::cNameColumn() );
}


uiString uiODFaultSurfaceDataTreeItem::createDisplayName() const
{
    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    const Attrib::SelSpec* as = visserv->getSelSpec( displayID(), attribNr() );

    if ( as->id().asInt()==Attrib::SelSpec::cNoAttrib().asInt() )
	return uiStrings::sZValue(mPlural);

    return uiODAttribTreeItem::createDisplayName();
}
