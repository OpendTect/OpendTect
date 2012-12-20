/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author: 	K. Tingdahl
 Date: 		Jul 2003
___________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uioddisplaytreeitem.h"
#include "uiodattribtreeitem.h"

#include "attribsel.h"
#include "pixmap.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uiodviewer2dmgr.h"
#include "uiodvolproctreeitem.h"
#include "uitreeview.h"
#include "uiviscoltabed.h"
#include "uivispartserv.h"
#include "vismultiattribsurvobj.h"
#include "vissurvobj.h"
#include "threadwork.h"


bool uiODDisplayTreeItem::create( uiTreeItem* treeitem, uiODApplMgr* appl,
				  int displayid )
{
    const uiTreeFactorySet* tfs = ODMainWin()->sceneMgr().treeItemFactorySet();
    if ( !tfs )
	return false;

    for ( int idx=0; idx<tfs->nrFactories(); idx++ )
    {
	mDynamicCastGet(const uiODTreeItemFactory*,itmcreater,
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
static const int cTextureInterpIdx = 997;
static const int cDuplicateIdx = 900;
static const int cLockIdx = -900;
static const int cHideIdx = -950;
static const int cRemoveIdx = -1000;

uiODDisplayTreeItem::uiODDisplayTreeItem()
    : uiODTreeItem(0)
    , displayid_(-1)
    , visserv_(ODMainWin()->applMgr().visServer())
    , addmnuitem_("&Add",cAddIdx)
    , addattribmnuitem_("&Attribute",cAttribIdx)
    , addvolprocmnuitem_("&Volume Processing attribute",cAttribIdx)
    , displaymnuitem_("&Display",cDisplayIdx)
    , duplicatemnuitem_("&Duplicate",cDuplicateIdx)
    , histogrammnuitem_("&Histogram ...",cHistogramIdx)
    , lockmnuitem_("&Lock",cLockIdx)
    , hidemnuitem_("&Hide",cHideIdx )
    , removemnuitem_("&Remove",cRemoveIdx)
{
    removemnuitem_.iconfnm = "stop";
    histogrammnuitem_.iconfnm = "histogram";
    lockmnuitem_.iconfnm = "lock_small";
}


uiODDisplayTreeItem::~uiODDisplayTreeItem()
{
    MenuHandler* menu = visserv_->getMenuHandler();
    if ( menu )
    {
	menu->initnotifier.remove(mCB(this,uiODDisplayTreeItem,createMenuCB));
	menu->handlenotifier.remove(mCB(this,uiODDisplayTreeItem,handleMenuCB));
    }

    MenuHandler* tb = visserv_->getToolBarHandler();
    if ( tb )
    {
	tb->createnotifier.remove(mCB(this,uiODDisplayTreeItem,addToToolBarCB));
	tb->handlenotifier.remove( mCB(this,uiODDisplayTreeItem,handleMenuCB) );
    }

    ODMainWin()->viewer2DMgr().remove2DViewer( displayid_ );
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
	? uiODDataTreeItem::factory().create( 0, *as, parenttype, false ) : 0;
    if ( !res ) res = new uiODAttribTreeItem( parenttype );
    return res;
}


uiODDataTreeItem* uiODDisplayTreeItem::addAttribItem()
{
    uiODDataTreeItem* newitem = createAttribItem(0);
    visserv_->addAttrib( displayid_ );
    addChild( newitem, false );
    updateColumnText( uiODSceneMgr::cNameColumn() );
    updateColumnText( uiODSceneMgr::cColorColumn() );
    return newitem;
}


bool uiODDisplayTreeItem::init()
{
    if ( !uiTreeItem::init() ) return false;

    if ( visserv_->hasAttrib( displayid_ ) )
    {
	for ( int attrib=0; attrib<visserv_->getNrAttribs(displayid_); attrib++)
	{
	    const Attrib::SelSpec* as = visserv_->getSelSpec(displayid_,attrib);
	    uiODDataTreeItem* item = createAttribItem( as );
	    if ( item )
	    {
		addChild( item, false );
		item->setChecked( visserv_->isAttribEnabled(displayid_,attrib));
	    }
	}
    }

    visserv_->setSelObjectId( displayid_ );
    setChecked( visserv_->isOn(displayid_) );
    checkStatusChange()->notify(mCB(this,uiODDisplayTreeItem,checkCB));

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
    PtrMan<ioPixmap> pixmap = 0;
    if ( islocked )
	pixmap = new ioPixmap( "lock_small" );
    else
	pixmap = new ioPixmap();

    uitreeviewitem_->setPixmap( 0, *pixmap );

    lockmnuitem_.text = getLockMenuText(); 
    lockmnuitem_.iconfnm = islocked ? "unlock" : "lock_small";
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
	
	PtrMan<ioPixmap> pixmap = 0;
	if ( so->hasColor() )
	{
	    pixmap = new ioPixmap( uiODDataTreeItem::cPixmapWidth(),
		    		   uiODDataTreeItem::cPixmapHeight() );
	    pixmap->fill( so->getColor() );
	}

	if ( pixmap ) uitreeviewitem_->setPixmap( uiODSceneMgr::cColorColumn(),
						 *pixmap );
    }

    uiTreeItem::updateColumnText( col );
}


bool uiODDisplayTreeItem::showSubMenu()
{
    return visserv_->showMenu( displayid_, uiMenuHandler::fromTree() );
}


void uiODDisplayTreeItem::checkCB( CallBacker* )
{
    if ( !visserv_->isSoloMode() )
	visserv_->turnOn( displayid_, isChecked() );
}


int uiODDisplayTreeItem::uiTreeViewItemType() const
{
    return uiTreeViewItem::CheckBox;
}


BufferString uiODDisplayTreeItem::createDisplayName() const
{
    const uiVisPartServer* cvisserv =
		const_cast<uiODDisplayTreeItem*>(this)->applMgr()->visServer();
    return cvisserv->getObjectName( displayid_ );
}


const char* uiODDisplayTreeItem::getLockMenuText() 
{ 
    return visserv_->isLocked(displayid_) ? "Un&lock" : "&Lock";
}


void uiODDisplayTreeItem::addToToolBarCB( CallBacker* cb )
{
    mDynamicCastGet(uiTreeItemTBHandler*,tb,cb);
    if ( !tb || tb->menuID() != displayID() || !isSelected() )
	return;

    createMenu( tb, true );
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

	mAddMenuItem( menu, &lockmnuitem_, true, false );
	mAddMenuItem( menu, &removemnuitem_,
		      !visserv_->isLocked(displayid_),false);
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
	mAddMenuItem( &addmnuitem_, &addvolprocmnuitem_,
		      !visserv_->isLocked(displayid_) &&
		      visserv_->canAddAttrib(displayid_,1), false );
    }
    else
    {
	mResetMenuItem( &addvolprocmnuitem_ );
	mResetMenuItem( &addattribmnuitem_ );
	mResetMenuItem( &histogrammnuitem_ );
    }

    mAddMenuItem( menu, &lockmnuitem_, true, false );

    mAddMenuItemCond( menu, &duplicatemnuitem_, true, false,
		      visserv_->canDuplicate(displayid_) );

    mDynamicCastGet(uiMenuHandler*,uimenu,menu)
    const bool usehide = uimenu &&
	uimenu->getMenuType()==uiMenuHandler::fromScene() &&
	!visserv_->isSoloMode();
    mAddMenuItemCond( menu, &hidemnuitem_, true, false, usehide );

    mAddMenuItem( menu, &removemnuitem_, !visserv_->isLocked(displayid_),false);
}


void uiODDisplayTreeItem::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( menu->isHandled() || menu->menuID()!=displayID() || mnuid==-1 )
	return;

    if ( mnuid==lockmnuitem_.id )
    {
	menu->setIsHandled(true);
	visserv_->lock( displayid_, !visserv_->isLocked(displayid_) );
	updateLockPixmap( visserv_->isLocked(displayid_) );
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
	if ( !askContinueAndSaveIfNeeded( true ) )
	    return;

	Threads::WorkManager::twm().addWork(
	    Threads::Work( *new uiTreeItemRemover( parent_, this ), true ), 0,
	    menu->queueID(), false );
    }
    else if ( mnuid==addattribmnuitem_.id )
    {
	uiODDataTreeItem* newitem = addAttribItem();
	newitem->select();
	const int id = newitem->displayID();
	const int attrib = newitem->attribNr();
	const bool selok = applMgr()->selectAttrib( id, attrib );
	if ( selok )
	    applMgr()->getNewData( id, attrib );
	newitem->select();
	applMgr()->updateColorTable( id, attrib );
	menu->setIsHandled(true);
    }
    else if ( mnuid==addvolprocmnuitem_.id )
    {
	menu->setIsHandled( true );

	const int attrib = visserv_->addAttrib( menu->menuID() );
	Attrib::SelSpec spec( "Velocity", Attrib::SelSpec::cOtherAttrib(),
				false, 0 );
	visserv_->setSelSpec( menu->menuID(), attrib, spec );
	visserv_->enableInterpolation( menu->menuID(), true );

	VolProc::uiDataTreeItem* newitem =
	    new VolProc::uiDataTreeItem( typeid(*this).name() );
	addChild( newitem, false );
	const bool selok = newitem->selectSetup();
	if ( selok )
	    applMgr()->getNewData( newitem->displayID(), newitem->attribNr() );

	updateColumnText( uiODSceneMgr::cNameColumn() );
	updateColumnText( uiODSceneMgr::cColorColumn() );
    }
    else if ( mnuid==histogrammnuitem_.id )
    {
	visserv_->displayMapperRangeEditForAttrbs( displayID() );
    }
    else if ( mnuid==hidemnuitem_.id )
    {
	menu->setIsHandled(true);
	visserv_->turnOn( displayid_, false );
	updateCheckStatus();
    }
}


void uiODDisplayTreeItem::prepareForShutdown()
{
    uiTreeItem::prepareForShutdown();
    mDynamicCastGet( const visSurvey::SurveyObject*, so,
		     visserv_->getObject(displayid_) );
    if ( ODMainWin()->colTabEd().getSurvObj() == so )
	ODMainWin()->colTabEd().setColTab( 0, mUdf(int), mUdf(int) );

    visserv_->turnSeedPickingOn( false );
    visserv_->removeObject( displayid_, sceneID() );
}
