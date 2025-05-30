/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

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
#include "randcolor.h"
#include "seistrctr.h"
#include "threadwork.h"
#include "zaxistransform.h"

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
    RefMan<visSurvey::Scene> scene =
		    ODMainWin()->applMgr().visServer()->getScene( sceneID() );
    const bool hastransform = scene && scene->getZAxisTransform();
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertAction( new uiAction(m3Dots(uiStrings::sAdd())), mAddMnuID );
    if ( !hastransform )
    {
	auto* newmenu = new uiAction( uiStrings::sNew() );
	mnu.insertAction( newmenu, mNewMnuID );
    }

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
	auto* dispmnu = new uiMenu( getUiParent(), tr("Display all") );

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
	RefObjectSet<EM::EMObject> objs;
	const ZDomain::Info* zinfo = nullptr;
	const ZAxisTransform* transform = scene->getZAxisTransform();
	if ( transform )
	    zinfo = &transform->toZDomainInfo();
	else
	    zinfo = &SI().zDomainInfo();

	applMgr()->EMServer()->selectFaults( objs, false, nullptr, zinfo );
	MouseCursorChanger mcc( MouseCursor::Wait );
	for ( int idx=0; idx<objs.size(); idx++ )
	{
	    setMoreObjectsToDoHint( idx<objs.size()-1 );
	    addChild( new uiODFaultTreeItem(objs[idx]->id()), false );
	}
    }
    else if ( mnuid == mNewMnuID )
    {
	auto ctxt = mIOObjContext(SeisTrc);
	if ( ctxt.nrMatches(true)==0 )
	{
	    uiMSG().message( tr("Fault planes can only be picked on 3D data,\n"
			"but there's no 3D data available in this project.\n"
			"When working with 2D data, create fault sticks first."
			"\nThese sticks can then be combined into a "
			"fault plane.") );
	    return false;
	}

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


uiTreeItem* uiODFaultTreeItemFactory::createForVis( const VisID& visid,
						    uiTreeItem*) const
{
    mDynamicCastGet(visSurvey::FaultDisplay*,fd,
	    ODMainWin()->applMgr().visServer()->getObject(visid));
    return fd ? new uiODFaultTreeItem( visid, true ) : nullptr;
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
    displayplanemnuitem_.iconfnm = "tree-flt"; \
    displaystickmnuitem_.checkable = true; \
    displaystickmnuitem_.iconfnm = "tree-fltss"; \
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


uiODFaultTreeItem::uiODFaultTreeItem( const VisID& id, bool dummy )
    : uiODDisplayTreeItem()
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
    visserv_->removeObject( displayid_, sceneID() );
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
    bool dosaveas = EM::EMM().getMultiID( emid_ ).isUdf();
    if ( !dosaveas )
    {
	PtrMan<IOObj> ioobj = IOM().get( EM::EMM().getMultiID(emid_) );
	dosaveas = !ioobj;
    }

    const bool issaved = applMgr()->EMServer()->storeObject( emid_, dosaveas );
    RefMan<visSurvey::FaultDisplay> faultdisplay = getDisplay();
    if ( issaved && faultdisplay &&
	 !applMgr()->EMServer()->getUiName(emid_).isEmpty() )
    {
	faultdisplay->setName( applMgr()->EMServer()->getName(emid_) );
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
	? uiODDataTreeItem::factory().create( 0, *as, parenttype, false)
	: nullptr;
    if ( !res )
	res = new uiODFaultSurfaceDataTreeItem( emid_, parenttype );

    return res;
}


bool uiODFaultTreeItem::init()
{
    if ( !displayid_.isValid() )
    {
	RefMan<visSurvey::FaultDisplay> fd = new visSurvey::FaultDisplay;
	displayid_ = fd->id();
	{ // TODO Should not be needed yet
	    ConstRefMan<visSurvey::Scene> scene =
		ODMainWin()->applMgr().visServer()->getScene( sceneID() );
	    if ( scene )
	    {
		ConstRefMan<ZAxisTransform> transform =
						scene->getZAxisTransform();
		fd->setZAxisTransform( transform.getNonConstPtr(), nullptr );
	    }

	    fd->setEMObjectID( emid_ );
	}

	visserv_->addObject( fd.ptr(), sceneID(), true);
    }

    mDynamicCastGet(visSurvey::FaultDisplay*,fd,
		    visserv_->getObject(displayid_));
    if ( !fd )
	return false;

    if ( emid_.isValid() && fd->getEMObjectID() != emid_ )
	fd->setEMObjectID( emid_ );

    ConstRefMan<ZAxisTransform> transform = fd->getZAxisTransform();
    if ( !transform )
	ODMainWin()->menuMgr().createFaultToolMan();

    mAttachCB( fd->materialChange(), uiODFaultTreeItem::colorChgCB );

    faultdisplay_ = fd;
    return uiODDisplayTreeItem::init();
}


ConstRefMan<visSurvey::FaultDisplay> uiODFaultTreeItem::getDisplay() const
{
    return faultdisplay_.get();
}


RefMan<visSurvey::FaultDisplay> uiODFaultTreeItem::getDisplay()
{
    return faultdisplay_.get();
}


bool uiODFaultTreeItem::askContinueAndSaveIfNeeded( bool withcancel )
{
    return applMgr()->EMServer()->askUserToSave( emid_, withcancel );
}


void uiODFaultTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODDisplayTreeItem::createMenu( menu, istb );
    if ( !menu || !isDisplayID(menu->menuID()) )
	return;

    ConstRefMan<visSurvey::FaultDisplay> faultdisplay = getDisplay();
    if ( !faultdisplay )
	return;

    mAddMenuOrTBItem( istb, menu, &displaymnuitem_, &displayplanemnuitem_,
		      true, faultdisplay->arePanelsDisplayed() );
    mAddMenuOrTBItem( istb, menu, &displaymnuitem_, &displaystickmnuitem_,
		      true, faultdisplay->areSticksDisplayed() );

    if ( !istb )
    {
	mAddMenuItem( &displaymnuitem_, &displayintersectionmnuitem_,
		      faultdisplay->canDisplayIntersections(),
		      faultdisplay->areIntersectionsDisplayed() );
	mAddMenuItem( &displaymnuitem_, &displayintersecthorizonmnuitem_,
		      faultdisplay->canDisplayHorizonIntersections(),
		      faultdisplay->areHorizonIntersectionsDisplayed() );
	mAddMenuItem( menu, &displaymnuitem_, true, true );

	mAddMenuItem( &displaymnuitem_, &singlecolmnuitem_,
		      faultdisplay->canShowTexture(),
		      !faultdisplay->showsTexture() );
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

    RefMan<visSurvey::FaultDisplay> faultdisplay = getDisplay();
    if ( !faultdisplay )
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

	if ( saveas && faultdisplay &&
	     !applMgr()->EMServer()->getUiName(emid_).isEmpty() )
	{
	    faultdisplay->setName( applMgr()->EMServer()->getName(emid_));
	    updateColumnText( uiODSceneMgr::cNameColumn() );
	}
    }
    else if ( mnuid==displayplanemnuitem_.id )
    {
	menu->setIsHandled(true);
	const bool stickchecked = displaystickmnuitem_.checked;
	const bool planechecked = displayplanemnuitem_.checked;
	faultdisplay->display( stickchecked || planechecked, !planechecked );
	select();
    }
    else if ( mnuid==displaystickmnuitem_.id )
    {
	menu->setIsHandled(true);
	const bool stickchecked = displaystickmnuitem_.checked;
	const bool planechecked = displayplanemnuitem_.checked;
	faultdisplay->display( !stickchecked, stickchecked || planechecked );
	select();
    }
    else if ( mnuid==displayintersectionmnuitem_.id )
    {
	menu->setIsHandled(true);
	const bool interchecked = displayintersectionmnuitem_.checked;
	faultdisplay->displayIntersections( !interchecked );
    }
    else if ( mnuid==displayintersecthorizonmnuitem_.id )
    {
	menu->setIsHandled(true);
	const bool interchecked = displayintersecthorizonmnuitem_.checked;
	faultdisplay->displayHorizonIntersections( !interchecked );
    }
    else if ( mnuid==singlecolmnuitem_.id )
    {
	menu->setIsHandled(true);
	faultdisplay->useTexture( !faultdisplay->showsTexture(), true );
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
    RefMan<visSurvey::Scene> scene =
		    ODMainWin()->applMgr().visServer()->getScene( sceneID() );
    const bool hastransform = scene && scene->getZAxisTransform();

    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertAction( new uiAction(m3Dots(uiStrings::sAdd())), mAddMnuID );
    if ( !hastransform )
	mnu.insertAction( new uiAction(uiStrings::sNew()), mNewMnuID );

    showMenuNotifier().trigger( &mnu, this );
    if ( children_.size() )
    {
	mnu.insertSeparator();
	auto* dispmnu = new uiMenu( getUiParent(), tr("Display All") );
	dispmnu->insertAction( new uiAction(tr("In full")), mDispInFull );
	dispmnu->insertAction( new uiAction(tr("Only at Sections")),
			     mDispAtSect );
	mnu.addMenu( dispmnu );
    }

    addStandardItems( mnu );

    const int mnuid = mnu.exec();
    if ( mnuid==mAddMnuID )
    {
	RefObjectSet<EM::EMObject> objs;
	const ZDomain::Info* zinfo = nullptr;
	const ZAxisTransform* transform = scene->getZAxisTransform();
	if ( transform )
	    zinfo = &transform->toZDomainInfo();
	else
	    zinfo = &SI().zDomainInfo();

	applMgr()->EMServer()->selectFaultStickSets( objs, nullptr, zinfo );
	MouseCursorChanger uics( MouseCursor::Wait );
	for ( int idx=0; idx<objs.size(); idx++ )
	{
	    setMoreObjectsToDoHint( idx<objs.size()-1 );
	    addChild( new uiODFaultStickSetTreeItem(objs[idx]->id()), false );
	}
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
uiODFaultStickSetTreeItemFactory::createForVis( const VisID& visid,
						uiTreeItem* ) const
{
    mDynamicCastGet(visSurvey::FaultStickSetDisplay*,fd,
	    ODMainWin()->applMgr().visServer()->getObject(visid));
    return fd ? new uiODFaultStickSetTreeItem( visid, true ) : nullptr;
}


#undef mCommonInit
#undef mCommonInit2
#define mCommonInit \
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


uiODFaultStickSetTreeItem::uiODFaultStickSetTreeItem( const VisID& id,
						      bool /* dummy */ )
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
    visserv_->removeObject( displayid_, sceneID() );
}


bool uiODFaultStickSetTreeItem::init()
{
    if ( !displayid_.isValid() )
    {
	RefMan<visSurvey::FaultStickSetDisplay> fsd =
				    new visSurvey::FaultStickSetDisplay;
	displayid_ = fsd->id();
	{ // TODO Should not be needed yet
	    ConstRefMan<visSurvey::Scene> scene =
		ODMainWin()->applMgr().visServer()->getScene( sceneID() );
	    if ( scene )
	    {
		ConstRefMan<ZAxisTransform> transform =
						scene->getZAxisTransform();
		fsd->setZAxisTransform( transform.getNonConstPtr(), nullptr );
	    }

	    fsd->setEMObjectID( emid_ );
	}

	visserv_->addObject( fsd.ptr(), sceneID(), true);
    }

    mDynamicCastGet(visSurvey::FaultStickSetDisplay*,fsd,
		    visserv_->getObject(displayid_));
    if ( !fsd )
	return false;

    if ( emid_.isValid() && fsd->getEMObjectID() != emid_ )
	fsd->setEMObjectID( emid_ );

    ConstRefMan<ZAxisTransform> transform = fsd->getZAxisTransform();
    if ( !transform )
	ODMainWin()->menuMgr().createFaultToolMan();

    mAttachCB( fsd->materialChange(), uiODFaultStickSetTreeItem::colorChCB );

    faultsticksetdisplay_ = fsd;
    return uiODDisplayTreeItem::init();
}


ConstRefMan<visSurvey::FaultStickSetDisplay>
uiODFaultStickSetTreeItem::getDisplay() const
{
    return faultsticksetdisplay_.get();
}


RefMan<visSurvey::FaultStickSetDisplay> uiODFaultStickSetTreeItem::getDisplay()
{
    return faultsticksetdisplay_.get();
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
    RefMan<visSurvey::FaultStickSetDisplay> faultsticksetdisplay = getDisplay();
    if ( issaved && faultsticksetdisplay &&
	 !applMgr()->EMServer()->getUiName(emid_).isEmpty() )
    {
	faultsticksetdisplay->setName( applMgr()->EMServer()->getName(emid_) );
	updateColumnText( uiODSceneMgr::cNameColumn() );
    }

    if ( issaved && cb )
	NotSavedPrompter::NSP().reportSuccessfullSave();
}


bool uiODFaultStickSetTreeItem::askContinueAndSaveIfNeeded( bool withcancel )
{
    return applMgr()->EMServer()->askUserToSave( emid_, withcancel );
}


void uiODFaultStickSetTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODDisplayTreeItem::createMenu( menu, istb );
    if ( !menu || !isDisplayID(menu->menuID()) || istb )
	return;

    ConstRefMan<visSurvey::FaultStickSetDisplay> faultsticksetdisplay =
								getDisplay();
    if ( !faultsticksetdisplay )
	return;

    mAddMenuItem( menu, &displaymnuitem_, true, false );
    mAddMenuItem( &displaymnuitem_, &onlyatsectmnuitem_,
		  true, faultsticksetdisplay->displayedOnlyAtSections() );

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

    RefMan<visSurvey::FaultStickSetDisplay> faultsticksetdisplay = getDisplay();
    if ( !faultsticksetdisplay )
	return;

    if ( mnuid==onlyatsectmnuitem_.id )
    {
	menu->setIsHandled(true);
	faultsticksetdisplay->setOnlyAtSectionsDisplay(
			!faultsticksetdisplay->displayedOnlyAtSections() );
    }
    else if ( mnuid==saveasmnuitem_.id ||  mnuid==savemnuitem_.id )
    {
	menu->setIsHandled( true );
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
	if ( saveas && faultsticksetdisplay && !emname.isEmpty() )
	{
	    faultsticksetdisplay->setName( emname );
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
    if ( istb )
	return;

    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    const Attrib::SelSpec* as = visserv->getSelSpec( displayID(), attribNr() );
    const bool islocked = visserv->isLocked( displayID() );

    mAddMenuItem( &selattrmnuitem_, &depthattribmnuitem_, !islocked,
		  as->id().asInt()==Attrib::SelSpec::cNoAttrib().asInt() );

    bool enablefaultdata = false;
#ifdef __debug__
    enablefaultdata = true;
#endif
    if ( !enablefaultdata )
    {
	mResetMenuItem( &loadsurfacedatamnuitem_ );
	mResetMenuItem( &savesurfacedatamnuitem_ );
	mResetMenuItem( &algomnuitem_ );
	return;
    }

    mDynamicCastGet(visSurvey::FaultDisplay*,fd,visserv->getObject(displayID()))
    BufferStringSet nmlist;
    if ( fd && fd->emFault() && fd->emFault()->auxData() )
	fd->emFault()->auxData()->getAuxDataList( nmlist );
    const int nrsurfdata = nmlist.size();
    uiString itmtxt = m3Dots(tr("Fault Data (%1)").arg(nrsurfdata ));
    loadsurfacedatamnuitem_.text = itmtxt;

    mAddMenuItem( &selattrmnuitem_, &loadsurfacedatamnuitem_,
	    false && !islocked && nrsurfdata>0, false );

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
	RefMan<DataPointSet> vals = new DataPointSet( false, true );
	vals->bivSet().setNrVals( 3 );
	visserv->getRandomPosCache( visid, attribnr, *vals );
	if ( vals->size() )
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
	RefMan<DataPointSet> vals = new DataPointSet( false, true );
	applMgr()->EMServer()->getAllAuxData( emid_, *vals, &shifts );
	setDataPointSet( *vals );

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
