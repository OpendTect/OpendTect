/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author: 	K. Tingdahl
 Date: 		Jul 2003
___________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uioddisplaytreeitem.h"
#include "uiodscenetreeitem.h"

#include "settings.h"
#include "ui3dviewer.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uiscenepropdlg.h"
#include "uitreeview.h"
#include "uivispartserv.h"
#include "vissurvscene.h"


const char* uiODTreeTop::sceneidkey()		{ return "Sceneid"; }
const char* uiODTreeTop::viewerptr()		{ return "Viewer"; }
const char* uiODTreeTop::applmgrstr()		{ return "Applmgr"; }
const char* uiODTreeTop::scenestr()		{ return "Scene"; }


uiODTreeTop::uiODTreeTop( ui3DViewer* sovwr, uiTreeView* lv, uiODApplMgr* am,
			    uiTreeFactorySet* tfs_ )
    : uiTreeTopItem(lv)
    , tfs(tfs_)
{
    setProperty<int>( sceneidkey(), sovwr->sceneID() );
    setPropertyPtr( viewerptr(), sovwr );
    setPropertyPtr( applmgrstr(), am );

    tfs->addnotifier.notify( mCB(this,uiODTreeTop,addFactoryCB) );
    tfs->removenotifier.notify( mCB(this,uiODTreeTop,removeFactoryCB) );
}


uiODTreeTop::~uiODTreeTop()
{
    tfs->addnotifier.remove( mCB(this,uiODTreeTop,addFactoryCB) );
    tfs->removenotifier.remove( mCB(this,uiODTreeTop,removeFactoryCB) );
}


int uiODTreeTop::sceneID() const
{
    int sceneid=-1;
    getProperty<int>( sceneidkey(), sceneid );
    return sceneid;
}


bool uiODTreeTop::selectWithKey( int selkey )
{
    applMgr()->visServer()->setSelObjectId(selkey);
    return true;
}


uiODApplMgr* uiODTreeTop::applMgr()
{
    void* res = 0;
    getPropertyPtr( applmgrstr(), res );
    return reinterpret_cast<uiODApplMgr*>( res );
}


TypeSet<int> uiODTreeTop::getDisplayIds( int& selectedid, bool usechecked )
{
    TypeSet<int> dispids;
    loopOverChildrenIds( dispids, selectedid, usechecked, children_ );
    return dispids;
}


void uiODTreeTop::loopOverChildrenIds( TypeSet<int>& dispids, int& selectedid,
				       bool usechecked,
				    const ObjectSet<uiTreeItem>& childrenlist )
{
    for ( int idx=0; idx<childrenlist.size(); idx++ )
	loopOverChildrenIds( dispids, selectedid,
			     usechecked, childrenlist[idx]->children_ );

    for ( int idy=0; idy<childrenlist.size(); idy++ )
    {
	mDynamicCastGet(const uiODDisplayTreeItem*,disptreeitem,
			childrenlist[idy]);
	if ( disptreeitem )
	{
	    if ( usechecked && childrenlist[idy]->isChecked() )
		dispids += disptreeitem->displayID();
	    else if ( !usechecked )
		dispids += disptreeitem->displayID();

	    if ( childrenlist[idy]->uitreeviewitem_->isSelected() )
		selectedid = disptreeitem->displayID();
	}
    }
}


uiODTreeItem::uiODTreeItem( const char* name__ )
    : uiTreeItem( name__ )
{}

bool uiODTreeItem::anyButtonClick( uiTreeViewItem* item )
{
    if ( item!=uitreeviewitem_ )
	return uiTreeItem::anyButtonClick( item );

    if ( !select() ) return false;

    applMgr()->updateColorTable( -1, -1 );
    return true;
}


uiODApplMgr* uiODTreeItem::applMgr()
{
    void* res = 0;
    getPropertyPtr( uiODTreeTop::applmgrstr(), res );
    return reinterpret_cast<uiODApplMgr*>( res );
}


ui3DViewer* uiODTreeItem::viewer()
{
    void* res = 0;
    getPropertyPtr( uiODTreeTop::viewerptr(), res );
    return reinterpret_cast<ui3DViewer*>( res );
}


int uiODTreeItem::sceneID() const
{
    int sceneid=-1;
    getProperty<int>( uiODTreeTop::sceneidkey(), sceneid );
    return sceneid;
}


void uiODTreeItem::addStandardItems( uiPopupMenu& mnu )
{
    if ( children_.size() < 2 ) return;

    mnu.insertSeparator( 100 );
    mnu.insertItem( new uiMenuItem("Show all items"), 101 );
    mnu.insertItem( new uiMenuItem("Hide all items"), 102 );
    mnu.insertItem( new uiMenuItem("Remove all items"), 103 );
}


void uiODTreeItem::handleStandardItems( int mnuid )
{
    for ( int idx=0; idx<children_.size(); idx++ )
    {
	if ( mnuid == 101 )
	    children_[idx]->setChecked( true, true );
	else if ( mnuid == 102 )
	    children_[idx]->setChecked( false, true );
    }

    if ( mnuid==103 )
    {
	const BufferString msg( "All ", name(),
	    " items will be removed from the tree.\nDo you want to continue?");
	if ( !uiMSG().askRemove(msg) ) return;

	while ( children_.size() )
	{
	    mDynamicCastGet(uiODDisplayTreeItem*,itm,children_[0])
	    if ( !itm ) continue;
	    itm->prepareForShutdown();
	    applMgr()->visServer()->removeObject( itm->displayID(), sceneID() );
	    removeChild( itm );
	}
    }
}


void uiODTreeTop::addFactoryCB( CallBacker* cb )
{
    mCBCapsuleUnpack(int,factidx,cb);
    const int newplaceidx = tfs->getPlacementIdx( factidx );
    uiTreeItem* itmbefore = 0;
    int maxidx = -1;
    for ( int idx=0; idx<tfs->nrFactories(); idx++ )
    {
	const int curidx = tfs->getPlacementIdx( idx );
	if ( curidx>newplaceidx || curidx<maxidx || curidx==newplaceidx )
	    continue;

	maxidx = curidx;
    }
    for ( int idx=0; idx<tfs->nrFactories(); idx++ )
    {
	if ( tfs->getPlacementIdx(idx) != maxidx )
	    continue;

	PtrMan<uiTreeItem> itm = tfs->getFactory(idx)->create();
	itmbefore = findChild( itm->name() );
	break;
    }

    uiTreeItem* newitm = tfs->getFactory(factidx)->create();
    addChild( newitm, false );
    if ( itmbefore )
	newitm->moveItem( itmbefore );
}


void uiODTreeTop::removeFactoryCB( CallBacker* cb )
{
    mCBCapsuleUnpack(int,idx,cb);
    PtrMan<uiTreeItem> dummy = tfs->getFactory(idx)->create();
    const uiTreeItem* child = findChild( dummy->name() );
    if ( children_.indexOf(child)==-1 )
	return;

    removeChild( const_cast<uiTreeItem*>(child) );
}


// uiODSceneTreeItem

uiODSceneTreeItem::uiODSceneTreeItem( const char* nm, int id )
    : uiODTreeItem(nm)
    , displayid_(id)
    , menu_(0)
    , propitem_("&Properties ...")
    , imageitem_("&Top/Bottom image ...")
    , coltabitem_("&Scene color table ...")
    , dumpivitem_("&Export scene ...")
{
    propitem_.iconfnm = "disppars";
}


uiODSceneTreeItem::~uiODSceneTreeItem()
{
    if ( menu_ )
    {
	menu_->createnotifier.remove( mCB(this,uiODSceneTreeItem,createMenuCB) );
	menu_->handlenotifier.remove( mCB(this,uiODSceneTreeItem,handleMenuCB) );
	menu_->unRef();
    }

    MenuHandler* tb = applMgr()->visServer()->getToolBarHandler();
    if ( tb )
    {
	tb->createnotifier.remove( mCB(this,uiODSceneTreeItem,addToToolBarCB) );
	tb->handlenotifier.remove( mCB(this,uiODSceneTreeItem,handleMenuCB) );
    }
}


bool uiODSceneTreeItem::init()
{
    if ( !menu_ )
    {
	menu_ = new uiMenuHandler( getUiParent(), -1 );
	menu_->ref();
	menu_->createnotifier.notify( mCB(this,uiODSceneTreeItem,createMenuCB) );
	menu_->handlenotifier.notify( mCB(this,uiODSceneTreeItem,handleMenuCB) );
    }

    MenuHandler* tb = applMgr()->visServer()->getToolBarHandler();
    if ( tb )
    {
	tb->createnotifier.notify( mCB(this,uiODSceneTreeItem,addToToolBarCB) );
	tb->handlenotifier.notify( mCB(this,uiODSceneTreeItem,handleMenuCB) );
    }

    return uiODTreeItem::init();
}


void uiODSceneTreeItem::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(MenuHandler*,menu,cb);
    createMenu( menu, false );
}


void uiODSceneTreeItem::addToToolBarCB( CallBacker* cb )
{
    mDynamicCastGet(uiTreeItemTBHandler*,tb,cb);
    if ( !tb || tb->menuID() != displayid_ || !isSelected() )
	return;

    createMenu( tb, true );
}


void uiODSceneTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    mAddMenuOrTBItem( istb, menu, menu, &propitem_, true, false );
    mAddMenuOrTBItem( istb, 0, menu, &imageitem_, true, false );
    mAddMenuOrTBItem( istb, 0, menu, &coltabitem_, true, false );
    bool enabdump = true;
    Settings::common().getYN(
		IOPar::compKey("dTect","Dump OI Menu"), enabdump );
    mAddMenuOrTBItem( istb, 0, menu, &dumpivitem_, enabdump, false );
}


void uiODSceneTreeItem::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( mnuid==-1 || menu->isHandled() )
	return;

    uiVisPartServer* visserv = applMgr()->visServer();
    if ( mnuid==propitem_.id )
    {
	ObjectSet<ui3DViewer> viewers;
	ODMainWin()->sceneMgr().getSoViewers( viewers );
	uiScenePropertyDlg dlg( getUiParent(), viewers, viewers.indexOf(viewer()) );
	dlg.go();
    }
    else if ( mnuid==imageitem_.id )
	visserv->setTopBotImg( displayid_ );
    else if ( mnuid==coltabitem_.id )
	visserv->manageSceneColorbar( displayid_ );
    else if( mnuid==dumpivitem_.id )
	visserv->writeSceneToFile( displayid_, "Export scene as ..." );
}


void uiODSceneTreeItem::updateColumnText( int col )
{
    if ( col==uiODSceneMgr::cNameColumn() )
	name_ = applMgr()->visServer()->getObjectName( displayid_ );

    uiTreeItem::updateColumnText( col );
}


bool uiODSceneTreeItem::showSubMenu()
{
    return menu_->executeMenu( uiMenuHandler::fromTree() );
}
