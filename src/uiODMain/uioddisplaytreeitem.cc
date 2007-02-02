/*+
___________________________________________________________________

 CopyRight: 	(C) dGB Beheer B.V.
 Author: 	K. Tingdahl
 Date: 		Jul 2003
 RCS:		$Id: uioddisplaytreeitem.cc,v 1.9 2007-02-02 15:44:43 cvsnanne Exp $
___________________________________________________________________

-*/

#include "uioddisplaytreeitem.h"
#include "uiodattribtreeitem.h"

#include "uimenu.h"
#include "uiodapplmgr.h"
#include "uivispartserv.h"

#include "pixmap.h"
#include "uilistview.h"
#include "uimenuhandler.h"
#include "uiodscenemgr.h"
#include "vissurvobj.h"


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

	uiTreeItem* res = itmcreater->create( displayid, treeitem );
	if ( res )
	{
	    treeitem->addChild( res, true );
	    return true;
	}
    }

    return false;
}


static const int sAttribIdx = 1000;
static const int sDuplicateIdx = 900;
static const int sLockIdx = -900;
static const int sRemoveIdx = -1000;

uiODDisplayTreeItem::uiODDisplayTreeItem()
    : uiODTreeItem(0)
    , displayid_(-1)
    , visserv(ODMainWin()->applMgr().visServer())
    , addattribmnuitem_("Add attribute",sAttribIdx)
    , duplicatemnuitem_("Duplicate",sDuplicateIdx)
    , lockmnuitem_("Lock",sLockIdx)
    , removemnuitem_("Remove",sRemoveIdx)
{
}


uiODDisplayTreeItem::~uiODDisplayTreeItem()
{
    MenuHandler* menu = visserv->getMenuHandler();
    if ( menu )
    {
	menu->createnotifier.remove(mCB(this,uiODDisplayTreeItem,createMenuCB));
	menu->handlenotifier.remove(mCB(this,uiODDisplayTreeItem,handleMenuCB));
    }

    if ( uilistviewitem_->pixmap(0) )
	delete uilistviewitem_->pixmap(0);

    ODMainWin()->sceneMgr().remove2DViewer( displayid_ );
}


int uiODDisplayTreeItem::selectionKey() const
{
    return displayid_;
}


bool uiODDisplayTreeItem::shouldSelect( int selkey ) const
{
    return uiTreeItem::shouldSelect( selkey ) && visserv->getSelAttribNr()==-1;
}


uiODDataTreeItem* uiODDisplayTreeItem::createAttribItem(
					const Attrib::SelSpec* as ) const
{
    const char* parenttype = typeid(*this).name();
    uiODDataTreeItem* res = as ? uiODDataTreeItem::create( *as, parenttype ) :0;
    if ( !res ) res = new uiODAttribTreeItem( parenttype );
    return res;
}


bool uiODDisplayTreeItem::init()
{
    if ( !uiTreeItem::init() ) return false;

    if ( visserv->hasAttrib( displayid_ ) )
    {
	for ( int attrib=0; attrib<visserv->getNrAttribs(displayid_); attrib++ )
	{
	    const Attrib::SelSpec* as = visserv->getSelSpec(displayid_,attrib);
	    uiODDataTreeItem* item = createAttribItem( as );
	    if ( item )
	    {
		addChild( item, false );
		item->setChecked( visserv->isAttribEnabled(displayid_,attrib) );
	    }
	}
    }

    visserv->setSelObjectId( displayid_ );
    uilistviewitem_->setChecked( visserv->isOn(displayid_) );
    checkStatusChange()->notify(mCB(this,uiODDisplayTreeItem,checkCB));

    name_ = createDisplayName();

    MenuHandler* menu = visserv->getMenuHandler();
    menu->createnotifier.notify( mCB(this,uiODDisplayTreeItem,createMenuCB) );
    menu->handlenotifier.notify( mCB(this,uiODDisplayTreeItem,handleMenuCB) );

    return true;
}


void uiODDisplayTreeItem::updateColumnText( int col )
{
    if ( col==uiODSceneMgr::cNameColumn() )
	name_ = createDisplayName();

    else if ( col==uiODSceneMgr::cColorColumn() )
    {
	mDynamicCastGet(visSurvey::SurveyObject*,so,
			visserv->getObject(displayid_))
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

	if ( pixmap ) uilistviewitem_->setPixmap( uiODSceneMgr::cColorColumn(),
						 *pixmap );
    }

    uiTreeItem::updateColumnText( col );
}


bool uiODDisplayTreeItem::showSubMenu()
{
    return visserv->showMenu( displayid_, uiMenuHandler::fromTree );
}


void uiODDisplayTreeItem::checkCB( CallBacker* )
{
    if ( !visserv->isSoloMode() )
	visserv->turnOn( displayid_, isChecked() );
}


int uiODDisplayTreeItem::uiListViewItemType() const
{
    return uiListViewItem::CheckBox;
}


BufferString uiODDisplayTreeItem::createDisplayName() const
{
    const uiVisPartServer* cvisserv =
		const_cast<uiODDisplayTreeItem*>(this)->applMgr()->visServer();
    return cvisserv->getObjectName( displayid_ );
}


const char* uiODDisplayTreeItem::getLockMenuText() 
{ 
    return visserv->isLocked(displayid_) ? "Unlock" : "Lock";
}


void uiODDisplayTreeItem::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(uiMenuHandler*,menu,cb);
    if ( menu->menuID() != displayID() )
	return;

    if ( visserv->hasAttrib(displayid_) &&
	 visserv->canHaveMultipleAttribs(displayid_) )
    {
	mAddMenuItem( menu, &addattribmnuitem_,
		      !visserv->isLocked(displayid_) &&
		      visserv->canAddAttrib(displayid_), false );
    }
    else
	mResetMenuItem( &addattribmnuitem_ );

    lockmnuitem_.text = getLockMenuText(); 
    mAddMenuItem( menu, &lockmnuitem_, true, false );
    
    mAddMenuItemCond( menu, &duplicatemnuitem_, true, false,
		      visserv->canDuplicate(displayid_) );

    mAddMenuItem( menu, &removemnuitem_, !visserv->isLocked(displayid_), false);
}


void uiODDisplayTreeItem::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(uiMenuHandler*,menu,caller);
    if ( menu->menuID()!=displayID() || mnuid==-1 || menu->isHandled() )
	return;

    if ( mnuid==lockmnuitem_.id )
    {
	menu->setIsHandled(true);
	mDynamicCastGet(visSurvey::SurveyObject*,so,
			visserv->getObject(displayid_))
	if ( !so )
	    return;

	so->lock( !so->isLocked() );
	PtrMan<ioPixmap> pixmap = 0;
	if ( so->isLocked() )
	    pixmap = new ioPixmap( "lock_small.png" );
	else
	    pixmap = new ioPixmap();
	uilistviewitem_->setPixmap( 0, *pixmap );
	ODMainWin()->sceneMgr().updateStatusBar();
    }
    else if ( mnuid==duplicatemnuitem_.id )
    {
	menu->setIsHandled(true);
	int newid =visserv->duplicateObject(displayid_,sceneID());
	if ( newid!=-1 )
	    uiODDisplayTreeItem::create( this, applMgr(), newid );
    }
    else if ( mnuid==removemnuitem_.id )
    {
	menu->setIsHandled(true);
	if ( askContinueAndSaveIfNeeded() )
	{
	    prepareForShutdown();
	    visserv->removeObject( displayid_, sceneID() );
	    parent_->removeChild( this );
	}
    }
    else if ( mnuid==addattribmnuitem_.id )
    {
	uiODDataTreeItem* newitem = createAttribItem(0);
	visserv->addAttrib( displayid_ );
	addChild( newitem, false );
	updateColumnText( uiODSceneMgr::cNameColumn() );
	updateColumnText( uiODSceneMgr::cColorColumn() );
	menu->setIsHandled(true);
    }
}
