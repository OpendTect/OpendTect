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
#include "mpeengine.h"
#include "ioobj.h"
#include "randcolor.h"

#include "uiempartserv.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uimpepartserv.h"
#include "uimsg.h"
#include "uinewemobjdlg.h"
#include "uiodapplmgr.h"
#include "uiodmenumgr.h"
#include "uiodscenemgr.h"
#include "uiodviewer2dmgr.h"
#include "uistrings.h"
#include "uitreeview.h"
#include "uivispartserv.h"

#include "visfaultdisplay.h"
#include "visfaultsticksetdisplay.h"


uiODFaultParentTreeItem::uiODFaultParentTreeItem()
    : uiODSceneTreeItem( uiStrings::sFault() )
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
	mTIUiMsg().error( tr("Cannot add Faults to this scene type") );
	return false;
    }

    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertAction( new uiAction(m3Dots(uiStrings::sAdd())), mAddMnuID );
    uiAction* newmenu = new uiAction( uiStrings::sNew() );
    mnu.insertAction( newmenu, mNewMnuID );
    newmenu->setEnabled( !hastransform && SI().has3D() );

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
	ObjectSet<EM::Object> objs;
	applMgr()->EMServer()->selectFaults( objs );
	uiUserShowWait usw( getUiParent(), uiStrings::sUpdatingDisplay() );
	for ( int idx=0; idx<objs.size(); idx++ )
	{
	    setMoreObjectsToDoHint( idx<objs.size()-1 );
	    addChild( new uiODFaultTreeItem(objs[idx]->id()), false );
	}

	deepUnRef( objs );
    }
    else if ( mnuid == mNewMnuID )
    {
	uiNewEMObjectDlg* newdlg =
	    uiNewEMObjectDlg::factory().create( EM::Fault3D::typeStr(),
						getUiParent() );
	if ( !newdlg || !newdlg->go() )
	    return false;

	RefMan<EM::Object> emo = newdlg->getEMObject();
	if ( !emo )
	    return false;

	emo->setPreferredColor( getRandomColor(false) );
	emo->setNameToJustCreated();
	emo->setFullyLoaded( true );
	addChild( new uiODFaultTreeItem(emo->id()), false );
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


uiTreeItem* uiODFaultTreeItemFactory::createForVis(int visid, uiTreeItem*) const
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



uiODFaultTreeItem::uiODFaultTreeItem( const DBKey& oid )
    : uiODDisplayTreeItem()
    , emid_( oid )
    mCommonInit
{
    mCommonInit2
}


uiODFaultTreeItem::uiODFaultTreeItem( int id, bool dummy )
    : uiODDisplayTreeItem()
    , emid_(DBKey::getInvalid())
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
	faultdisplay_->unRef();
    }
}


uiODDataTreeItem* uiODFaultTreeItem::createAttribItem(
	const Attrib::SelSpec* as ) const
{
    const char* parenttype = typeid(*this).name();
    uiODDataTreeItem* res = as
	? uiODDataTreeItem::factory().createSuitable( *as, parenttype ) : 0;
    if ( !res )
	res = new uiODFaultSurfaceDataTreeItem( emid_, parenttype );
    return res;
}


bool uiODFaultTreeItem::init()
{
    if ( displayid_==-1 )
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

    faultdisplay_->materialChange()->notify(
	    mCB(this,uiODFaultTreeItem,colorChCB));

    return uiODDisplayTreeItem::init();
}


void uiODFaultTreeItem::colorChCB( CallBacker* )
{
    updateColumnText( uiODSceneMgr::cColorColumn() );
}


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
    if ( !menu || menu->menuID()!=displayID() )
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

    const bool enablesave = applMgr()->EMServer()->isChanged(emid_) &&
			    applMgr()->EMServer()->isFullyLoaded(emid_) &&
			    EM::canOverwrite(emid_);

    mAddMenuOrTBItem( istb, menu, menu, &savemnuitem_, enablesave, false );
    mAddMenuOrTBItem( istb, 0, menu, &saveasmnuitem_, true, false );
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
	bool saveas = mnuid==saveasmnuitem_.id || emid_.isInvalid();
	if ( !saveas )
	{
	    PtrMan<IOObj> ioobj = emid_.getIOObj();
	    saveas = !ioobj;
	}

	applMgr()->EMServer()->storeObject( emid_, saveas );

	if ( saveas && faultdisplay_ && !emid_.name().isEmpty() )
	{
	    faultdisplay_->setName( emid_.name() );
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



// uiODFaultStickSetParentTreeItem
uiODFaultStickSetParentTreeItem::uiODFaultStickSetParentTreeItem()
    : uiODSceneTreeItem( uiStrings::sFaultStickSet() )
{}


const char* uiODFaultStickSetParentTreeItem::iconName() const
{ return "tree-fltss"; }


bool uiODFaultStickSetParentTreeItem::showSubMenu()
{
    mDynamicCastGet(visSurvey::Scene*,scene,
		    ODMainWin()->applMgr().visServer()->getObject(sceneID()));
    if ( scene && scene->getZAxisTransform() )
    {
	mTIUiMsg().error( tr("Cannot add FaultStickSets to this scene type") );
	return false;
    }

    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertAction( new uiAction(m3Dots(uiStrings::sAdd())), mAddMnuID );
    mnu.insertAction( new uiAction(uiStrings::sNew()), mNewMnuID );

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
	ObjectSet<EM::Object> objs;
	applMgr()->EMServer()->selectFaultStickSets( objs );
	uiUserShowWait usw( getUiParent(), uiStrings::sUpdatingDisplay() );
	for ( int idx=0; idx<objs.size(); idx++ )
	{
	    setMoreObjectsToDoHint( idx<objs.size()-1 );
	    addChild( new uiODFaultStickSetTreeItem(objs[idx]->id()), false );
	}
	deepUnRef( objs );
    }
    else if ( mnuid == mNewMnuID )
    {
	uiNewEMObjectDlg* newdlg =
	    uiNewEMObjectDlg::factory().create( EM::FaultStickSet::typeStr(),
						getUiParent() );
	if ( !newdlg || !newdlg->go() )
	    return false;

	RefMan<EM::Object> emo = newdlg->getEMObject();
	if ( !emo )
	    return false;

	emo->setPreferredColor( getRandomColor(false) );
	emo->setNameToJustCreated();
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
    , savemnuitem_(uiStrings::sSave()) \
    , saveasmnuitem_(m3Dots(uiStrings::sSaveAs())) \
    , onlyatsectmnuitem_( uiODFaultTreeItem::sOnlyAtSections() )

#define mCommonInit2 \
    onlyatsectmnuitem_.checkable = true; \
    savemnuitem_.iconfnm = "save"; \
    saveasmnuitem_.iconfnm = "saveas";


uiODFaultStickSetTreeItem::uiODFaultStickSetTreeItem( const DBKey& oid )
    : uiODDisplayTreeItem()
    , emid_( oid )
    mCommonInit
{
    mCommonInit2
}


uiODFaultStickSetTreeItem::uiODFaultStickSetTreeItem( int id, bool dummy )
    : uiODDisplayTreeItem()
    , emid_(DBKey::getInvalid())
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
	faultsticksetdisplay_->unRef();
    }
}


bool uiODFaultStickSetTreeItem::init()
{
    if ( displayid_==-1 )
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

    faultsticksetdisplay_->materialChange()->notify(
	    mCB(this,uiODFaultStickSetTreeItem,colorChCB) );

    return uiODDisplayTreeItem::init();
}


void uiODFaultStickSetTreeItem::colorChCB( CallBacker* )
{
    updateColumnText( uiODSceneMgr::cColorColumn() );
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
	    fd->setOnlyAtSectionsDisplay( !fd->displayedOnlyAtSections() );
    }
    else if ( mnuid==saveasmnuitem_.id ||  mnuid==savemnuitem_.id )
    {
	menu->setIsHandled(true);
	bool saveas = mnuid==saveasmnuitem_.id || emid_.isInvalid();
	if ( !saveas )
	{
	    PtrMan<IOObj> ioobj = emid_.getIOObj();
	    saveas = !ioobj;
	}

	applMgr()->EMServer()->storeObject( emid_, saveas );

	const BufferString emname = emid_.name();
	if ( saveas && faultsticksetdisplay_ && !emname.isEmpty() )
	{
	    faultsticksetdisplay_->setName( emname );
	    updateColumnText( uiODSceneMgr::cNameColumn() );
	}
    }
}



// uiODFaultSurfaceDataTreeItem
uiODFaultSurfaceDataTreeItem::uiODFaultSurfaceDataTreeItem( const DBKey& objid,
	const char* parenttype )
    : uiODAttribTreeItem(parenttype)
    , depthattribmnuitem_( uiStrings::sZValue(mPlural) )
    , savesurfacedatamnuitem_(m3Dots(tr("Save as Fault Data")))
    , loadsurfacedatamnuitem_(m3Dots(tr("Fault Data")))
    , algomnuitem_(uiStrings::sSmooth())
    , changed_(false)
    , emid_(objid)
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
	    as->id() == Attrib::SelSpec::cNoAttribID() );

    const bool enabsave = changed_ ||
	    (as && as->id()!=Attrib::SelSpec::cNoAttribID() &&
	     as->id()!=Attrib::SelSpec::cAttribNotSelID() );
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

    const int visid = displayID();
    const int attribnr = attribNr();

    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    if ( mnuid==savesurfacedatamnuitem_.id )
    {
	menu->setIsHandled( true );
	RefMan<DataPointSet> vals = new DataPointSet( false, true );
	vals->bivSet().setNrVals( 3 );
	visserv->getRandomPosCache( visid, attribnr, *vals );
	if ( !vals->isEmpty() )
	{
	    BufferString auxdatanm;
	    const bool saved =
		applMgr()->EMServer()->storeAuxData( emid_, auxdatanm, true );
	    if ( saved )
	    {
		const Attrib::SelSpec newas( auxdatanm,
			Attrib::SelSpec::cOtherAttribID() );
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
    const int visid = displayID();
    const int attribnr = attribNr();
    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    FixedString attrnm = vals.nrCols()>1 ? vals.colName(1) : "";
    visserv->setSelSpec( visid, attribnr,
	    Attrib::SelSpec(attrnm,Attrib::SelSpec::cOtherAttribID()) );
    visserv->setRandomPosData( visid, attribnr, &vals );
    visserv->selectTexture( visid, attribnr, 0 );
    updateColumnText( uiODSceneMgr::cNameColumn() );
}


uiString uiODFaultSurfaceDataTreeItem::createDisplayName() const
{
    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    const Attrib::SelSpec* as = visserv->getSelSpec( displayID(), attribNr() );

    if ( as->id() == Attrib::SelSpec::cNoAttribID() )
	return uiStrings::sZValue(mPlural);

    return uiODAttribTreeItem::createDisplayName();
}
