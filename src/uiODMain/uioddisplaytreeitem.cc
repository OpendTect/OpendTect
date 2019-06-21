/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jul 2003
___________________________________________________________________

-*/

#include "uioddisplaytreeitem.h"
#include "uiodattribtreeitem.h"

#include "uiicon.h"
#include "uiioobj.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uiodviewer2dmgr.h"
#include "uiodvolproctreeitem.h"
#include "uishortcutsmgr.h"
#include "uitreeview.h"
#include "uiviscoltabed.h"
#include "uivispartserv.h"
#include "vismultiattribsurvobj.h"
#include "vissurvobj.h"

#include "attribsel.h"
#include "ioobj.h"
#include "threadwork.h"


bool uiODDisplayTreeItem::create( uiTreeItem* treeitem, uiODApplMgr* appl,
				  int displayid )
{
    const uiTreeFactorySet* tfs = ODMainWin()->sceneMgr().treeItemFactorySet();
    if ( !tfs )
	return false;

    for ( int idx=0; idx<tfs->nrFactories(); idx++ )
    {
	mDynamicCastGet(const uiODSceneTreeItemFactory*,itmcreater,
			tfs->getFactory(idx))
	if ( !itmcreater ) continue;

	uiTreeItem* res = itmcreater->createForVis( displayid, treeitem );
	if ( res )
	{
	    treeitem->addChild( res, false );
	    return true;
	}
    }

    return false;
}

static const int cHistogramIdx = 991;
static const int cAddIdx = 999;
static const int cDisplayIdx = 998;
static const int cAttribIdx = 1000;
static const int cDuplicateIdx = 900;
static const int cLockIdx = -900;
static const int cHideIdx = -950;
static const int cRemoveIdx = -1000;

uiODDisplayTreeItem::uiODDisplayTreeItem()
    : uiODSceneTreeItem(uiString::empty())
    , displayid_(-1)
    , visserv_(ODMainWin()->applMgr().visServer())
    , addmnuitem_(uiStrings::sAdd(),cAddIdx)
    , addattribmnuitem_(uiStrings::sAttribute(), cAttribIdx)
    , addvolprocmnuitem_(tr("Volume Builder Attribute"),cAttribIdx)
    , displaymnuitem_(uiStrings::sDisplay(),cDisplayIdx)
    , duplicatemnuitem_(tr("Duplicate"),cDuplicateIdx)
    , histogrammnuitem_(m3Dots(uiStrings::sHistogram()),cHistogramIdx)
    , lockmnuitem_(uiString::empty(),cLockIdx)
    , hidemnuitem_(uiStrings::sHide(),cHideIdx )
    , removemnuitem_(tr("Remove from Tree"),cRemoveIdx)
{
    removemnuitem_.iconfnm = "remove";
    histogrammnuitem_.iconfnm = "histogram";
    lockmnuitem_.iconfnm = "lock";
    addattribmnuitem_.iconfnm = "attributes";
    addvolprocmnuitem_.iconfnm = "volproc";
}


uiODDisplayTreeItem::~uiODDisplayTreeItem()
{
    MenuHandler* menu = visserv_->getMenuHandler();
    if ( menu )
    {
	menu->initnotifier.remove(mCB(this,uiODDisplayTreeItem,createMenuCB));
	menu->handlenotifier.remove(
		mCB(this,uiODDisplayTreeItem,handleMenuCB));
    }

    MenuHandler* tb = visserv_->getToolBarHandler();
    if ( tb )
    {
	tb->createnotifier.remove(mCB(this,uiODDisplayTreeItem,addToToolBarCB));
	tb->handlenotifier.remove( mCB(this,uiODDisplayTreeItem,handleMenuCB) );
    }

    visserv_->removeObject( displayid_, sceneID() );
}


int uiODDisplayTreeItem::selectionKey() const
{
    return displayid_;
}


bool uiODDisplayTreeItem::shouldSelect( int selkey ) const
{
    return uiTreeItem::shouldSelect( selkey ) && visserv_->getSelAttribNr()==-1;
}


uiODDataTreeItem* uiODDisplayTreeItem::createAttribItem(
					const Attrib::SelSpec* as ) const
{
    const char* parenttype = typeid(*this).name();
    uiODDataTreeItem* res = as
	? uiODDataTreeItem::factory().createSuitable( *as, parenttype ) : 0;
    if ( !res )
	res = new uiODAttribTreeItem( parenttype );
    return res;
}


uiODDataTreeItem* uiODDisplayTreeItem::addAttribItem()
{
    uiODDataTreeItem* newitem = createAttribItem(0);
    addChild( newitem, false );
    updateColumnText( uiODSceneMgr::cNameColumn() );
    updateColumnText( uiODSceneMgr::cColorColumn() );
    return newitem;
}


bool uiODDisplayTreeItem::init()
{
    if ( !uiODSceneTreeItem::init() )
	return false;

    if ( getMoreObjectsToDoHint() )
	ODMainWin()->sceneMgr().getTree(sceneID())->triggerUpdate();
    else
	visserv_->setSelObjectId( displayid_ );

    setChecked( visserv_->isOn(displayid_) );
    checkStatusChange()->notify( mCB(this,uiODDisplayTreeItem,checkCB) );
    keyPressed()->notify( mCB(this,uiODDisplayTreeItem,keyPressCB) );

    name_ = createDisplayName();

    MenuHandler* menu = visserv_->getMenuHandler();
    menu->initnotifier.notify( mCB(this,uiODDisplayTreeItem,createMenuCB) );
    menu->handlenotifier.notify( mCB(this,uiODDisplayTreeItem,handleMenuCB) );

    MenuHandler* tb = visserv_->getToolBarHandler();
    tb->createnotifier.notify( mCB(this,uiODDisplayTreeItem,addToToolBarCB) );
    tb->handlenotifier.notify( mCB(this,uiODDisplayTreeItem,handleMenuCB) );

    return true;
}


void uiODDisplayTreeItem::updateCheckStatus()
{
    const bool ison = visserv_->isOn( displayid_ );
    const bool ischecked = isChecked();
    if ( ison != ischecked )
	setChecked( ison );

    uiTreeItem::updateCheckStatus();
}


void uiODDisplayTreeItem::updateLockPixmap( bool islocked )
{
    const char* iconname = islocked ? "lock" : uiIcon::None();
    uitreeviewitem_->setIcon( 0, iconname );

    lockmnuitem_.text = getLockMenuText();
    lockmnuitem_.iconfnm = islocked ? "lock" : "unlock";
}


void uiODDisplayTreeItem::updateColumnText( int col )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,
		    visserv_->getObject(displayid_))
    if ( col==uiODSceneMgr::cNameColumn() )
    {
	name_ = createDisplayName();
	updateLockPixmap( visserv_->isLocked(displayid_) );
    }

    else if ( col==uiODSceneMgr::cColorColumn() )
    {
	if ( !so )
	{
	    uiTreeItem::updateColumnText( col );
	    return;
	}

	if ( so->hasColor() )
	    uitreeviewitem_->setPixmap( uiODSceneMgr::cColorColumn(),
					so->getColor() );
    }

    uiTreeItem::updateColumnText( col );
}


bool uiODDisplayTreeItem::showSubMenu()
{
    return visserv_->showMenu( displayid_, uiMenuHandler::fromTree() );
}


void uiODDisplayTreeItem::handleItemCheck( bool triggerdispreq )
{
    if ( !visserv_->isSoloMode() )
	visserv_->turnOn( displayid_, isChecked() );
    if ( triggerdispreq )
	emitPrRequest( isChecked() ? Presentation::Show : Presentation::Hide );
}


void uiODDisplayTreeItem::checkCB( CallBacker* )
{
    handleItemCheck( true );
}


void uiODDisplayTreeItem::keyPressCB( CallBacker* cb )
{
    mCBCapsuleUnpack(uiKeyDesc,kd,cb);

    if ( kd.state()==OD::ShiftButton && kd.key()==OD::KB_Delete )
	deleteObject();
    else if ( kd.key()==OD::KB_V && kd.state()==OD::NoButton )
	setOnlyAtSectionsDisplay( !displayedOnlyAtSections() );
}


bool uiODDisplayTreeItem::doubleClick( uiTreeViewItem* item )
{
    if ( item != uitreeviewitem_ )
	return uiTreeItem::doubleClick( item );

    if ( !select() ) return false;

    visserv_->setMaterial( displayID() );
    return true;
}


void uiODDisplayTreeItem::setOnlyAtSectionsDisplay( bool yn )
{
    visserv_->setOnlyAtSectionsDisplay( displayid_, yn );
}


bool uiODDisplayTreeItem::displayedOnlyAtSections() const
{ return visserv_->displayedOnlyAtSections( displayid_ ); }


int uiODDisplayTreeItem::uiTreeViewItemType() const
{
    return uiTreeViewItem::CheckBox;
}


uiString uiODDisplayTreeItem::createDisplayName() const
{
    const uiVisPartServer* cvisserv =
		const_cast<uiODDisplayTreeItem*>(this)->applMgr()->visServer();
    return toUiString( cvisserv->getObjectName(displayid_) );
}


uiString uiODDisplayTreeItem::getLockMenuText() const
{
    return visserv_->isLocked(displayid_)
	? tr("Unlock Treeitem") : tr("Lock Treeitem");
}


void uiODDisplayTreeItem::addToToolBarCB( CallBacker* cb )
{
    mDynamicCastGet(uiTreeItemTBHandler*,tb,cb);
    if ( !tb || tb->menuID() != displayID() || !isSelected() )
	return;

    const bool enab = !visserv_->isLocked(displayid_) &&
	visserv_->canRemoveDisplay( displayid_ );

    createMenu( tb, true );
    mAddMenuItem( tb, &lockmnuitem_, true, false );
    mAddMenuItem( tb, &removemnuitem_, enab, false );
}


void uiODDisplayTreeItem::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(uiMenuHandler*,menu,cb);
    if ( !menu || menu->menuID() != displayID() )
	return;

    createMenu( menu, false );
}


void uiODDisplayTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    if ( istb )
    {
	if ( visserv_->hasAttrib(displayid_) &&
	     visserv_->canHaveMultipleAttribs(displayid_) )
	{
	    mAddMenuItem( menu, &histogrammnuitem_, true, false );
	}
	else
	    mResetMenuItem( &histogrammnuitem_ );

	return;
    }

    const bool hasmultiattribs = visserv_->hasAttrib(displayid_) &&
				 visserv_->canHaveMultipleAttribs(displayid_);
    if ( visserv_->hasMaterial(displayid_) || hasmultiattribs )
    {
        mAddMenuItem( menu, &displaymnuitem_, true, false );
        displaymnuitem_.removeItems();
    }

    if ( hasmultiattribs )
    {
	mAddMenuItem( menu, &addmnuitem_, true, false );
	mAddMenuItem( &addmnuitem_, &addattribmnuitem_,
		      !visserv_->isLocked(displayid_) &&
		      visserv_->canAddAttrib(displayid_), false );
	mAddMenuItem( &displaymnuitem_, &histogrammnuitem_, true, false );
    }
    else
    {
	mResetMenuItem( &addattribmnuitem_ );
	mResetMenuItem( &histogrammnuitem_ );
    }

    const bool canaddvolproc = visserv_->canAddAttrib(displayid_,1) &&
	visserv_->getAttributeFormat(displayid_,-1) == uiVisPartServer::Cube;
    if ( hasmultiattribs && canaddvolproc )
    {
	mAddMenuItem( &addmnuitem_, &addvolprocmnuitem_,
		      !visserv_->isLocked(displayid_), false );
    }
    else
    {
	mResetMenuItem( &addvolprocmnuitem_ );
    }

    mAddMenuItem( menu, &lockmnuitem_, true, false );

    mAddMenuItemCond( menu, &duplicatemnuitem_, true, false,
		      visserv_->canDuplicate(displayid_) );

    mDynamicCastGet(uiMenuHandler*,uimenu,menu)
    const bool usehide = uimenu &&
	uimenu->getMenuType()==uiMenuHandler::fromScene() &&
	!visserv_->isSoloMode();
    mAddMenuItemCond( menu, &hidemnuitem_, true, false, usehide );

    const bool enab = !visserv_->isLocked( displayid_ ) &&
	visserv_->canRemoveDisplay( displayid_ );

    mAddMenuItem( menu, &removemnuitem_, enab, false );
}


void uiODDisplayTreeItem::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( !menu || menu->isHandled() || menu->menuID()!=displayID() || mnuid==-1)
	return;

    if ( mnuid==lockmnuitem_.id )
    {
	menu->setIsHandled(true);
	visserv_->lock( displayid_, !visserv_->isLocked(displayid_) );
	updateLockPixmap( visserv_->isLocked(displayid_) );
	select();
	ODMainWin()->sceneMgr().updateStatusBar();
    }
    else if ( mnuid==duplicatemnuitem_.id )
    {
	menu->setIsHandled(true);
	int newid =visserv_->duplicateObject(displayid_,sceneID());
	if ( newid!=-1 )
	    uiODDisplayTreeItem::create( this, applMgr(), newid );
    }
    else if ( mnuid==removemnuitem_.id )
    {
	menu->setIsHandled(true);
	if ( !askContinueAndSaveIfNeeded(true) )
	    return;

	Threads::WorkManager::twm().addWork(
	    Threads::Work( *new uiTreeItemRemover(parent_,this), true ), 0,
	    menu->queueID(), false );
    }
    else if ( mnuid==addattribmnuitem_.id )
    {
	handleAddAttrib();
	menu->setIsHandled(true);
    }
    else if ( mnuid==addvolprocmnuitem_.id )
    {
	handleAddVolProcAttrib( menu->menuID() );
	menu->setIsHandled(true);
    }
    else if ( mnuid==histogrammnuitem_.id )
    {
	visserv_->displayMapperRangeEditForAttribs( displayID() );
    }
    else if ( mnuid==hidemnuitem_.id )
    {
	menu->setIsHandled(true);
	visserv_->turnOn( displayid_, false );
	updateCheckStatus();
    }
}


void uiODDisplayTreeItem::deleteObject()
{
    const DBKey mid = visserv_->getDBKey( displayid_ );
    PtrMan<IOObj> ioobj = mid.getIOObj();
    if ( !ioobj )
	return;

    if ( visserv_->isLocked(displayid_) )
    {
	mTIUiMsg().error( tr("Treeitem is locked, can not delete") );
	return;
    }

    if ( ioobj->implReadOnly() )
    {
	mTIUiMsg().error( tr("Data is read-only, can not delete") );
	return;
    }

    const int sceneid = sceneID(); // Don't change this order!
    if ( !uiIOObj(*ioobj).removeImpl(true,true) )
	return;

    visserv_->removeObject( displayid_, sceneid );
    parent_->removeChild( this );
}


void uiODDisplayTreeItem::handleAddAttrib()
{ // NO LONGER USED!!! Replaced by probe
    uiODDataTreeItem* newitem = addAttribItem();
    newitem->select();
    const int visid = newitem->displayID();
    const int attrib = newitem->attribNr();
    const bool selok = applMgr()->selectAttrib( visid, attrib );
    if ( selok && !visserv_->calcManipulatedAttribs(visid) )
	applMgr()->getNewData( visid, attrib );

    newitem->select();
    applMgr()->useDefColTab( visid, attrib );
    updateColumnText( uiODSceneMgr::cNameColumn() );
    updateColumnText( uiODSceneMgr::cColorColumn() );
}


void uiODDisplayTreeItem::handleAddVolProcAttrib( int menuid )
{
    const int attrib = visserv_->addAttrib( menuid );
    const Attrib::SelSpec spec( "Velocity",
				Attrib::SelSpec::cExternalAttribID(), false );
    visserv_->setSelSpec( menuid, attrib, spec );
    visserv_->enableInterpolation( menuid, true );

    VolProc::uiDataTreeItem* newitem =
			new VolProc::uiDataTreeItem( typeid(*this).name() );
    addChild( newitem, false );
    const bool selok = newitem->selectSetup();
    const int visid = newitem->displayID();
    const int attribid = newitem->attribNr();
    if ( selok && !visserv_->calcManipulatedAttribs(visid) )
	applMgr()->getNewData( visid, attribid );

    applMgr()->useDefColTab( visid, attribid );
    updateColumnText( uiODSceneMgr::cNameColumn() );
    updateColumnText( uiODSceneMgr::cColorColumn() );
}


void uiODDisplayTreeItem::prepareForShutdown()
{
    uiODSceneTreeItem::prepareForShutdown();
    visserv_->removeObject( displayid_, sceneID() );
}
