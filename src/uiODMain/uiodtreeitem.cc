/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uioddisplaytreeitem.h"
#include "uiodscenetreeitem.h"

#include "keyboardevent.h"
#include "settings.h"
#include "ui3dviewer.h"
#include "uimain.h"
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


uiODTreeTop::uiODTreeTop( ui3DViewer* sovwr, uiTreeView* lv, uiODApplMgr* am,
			    uiTreeFactorySet* tfs_ )
    : uiTreeTopItem(lv)
    , tfs(tfs_)
{
    setProperty<int>( sceneidkey(), sovwr->sceneID().asInt() );
    setPropertyPtr( viewerptr(), sovwr );
    setPropertyPtr( applmgrstr(), am );

    mAttachCB(tfs->addnotifier, uiODTreeTop::addFactoryCB);
    mAttachCB(tfs->removenotifier, uiODTreeTop::removeFactoryCB);
}


uiODTreeTop::~uiODTreeTop()
{
    detachAllNotifiers();
}


SceneID uiODTreeTop::sceneID() const
{
    int sceneid = -1;
    getProperty<int>( sceneidkey(), sceneid );
    return SceneID(sceneid);
}


bool uiODTreeTop::selectWithKey( int selkey )
{
    applMgr()->visServer()->setSelObjectId( VisID(selkey) );
    return true;
}


uiODApplMgr* uiODTreeTop::applMgr()
{
    void* res = 0;
    getPropertyPtr( applmgrstr(), res );
    return reinterpret_cast<uiODApplMgr*>( res );
}


TypeSet<VisID> uiODTreeTop::getDisplayIds( VisID& selectedid, bool usechecked )
{
    TypeSet<VisID> dispids;
    loopOverChildrenIds( dispids, selectedid, usechecked, children_ );
    return dispids;
}


void uiODTreeTop::loopOverChildrenIds( TypeSet<VisID>& dispids,
				VisID& selectedid, bool usechecked,
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


uiODTreeItem::uiODTreeItem( const uiString& nm )
    : uiTreeItem( nm )
{}


uiODTreeItem::~uiODTreeItem()
{}


bool uiODTreeItem::anyButtonClick( uiTreeViewItem* item )
{
    if ( item!=uitreeviewitem_ )
	return uiTreeItem::anyButtonClick( item );

    if ( !select() ) return false;

    applMgr()->updateColorTable( VisID::udf(), -1 );
    return true;
}


bool uiODTreeItem::init()
{
    const char* iconnm = iconName();
    if ( iconnm ) uitreeviewitem_->setIcon( 0, iconnm );

    return uiTreeItem::init();
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


SceneID uiODTreeItem::sceneID() const
{
    int sceneid = -1;
    getProperty<int>( uiODTreeTop::sceneidkey(), sceneid );
    return SceneID(sceneid);
}


void uiODTreeItem::addStandardItems( uiMenu& mnu )
{
    if ( children_.size() < 2 ) return;

    mnu.insertSeparator();
    uiAction* action = new uiAction( tr("Show All Items") );
    action->setEnabled( !allChildrenChecked() );
    mnu.insertAction( action, 101 );

    action = new uiAction( tr("Hide All Items") );
    action->setEnabled( !allChildrenUnchecked() );
    mnu.insertAction( action, 102 );

    mnu.insertAction( new uiAction(tr("Remove All Items from Tree")), 103 );

    mDynamicCastGet(uiODDisplayTreeItem*,itm,children_[0])
    if ( !itm || itm->nrChildren()==0 )
	return;

    mnu.insertSeparator();
    action = new uiAction( tr("Expand All Items") );
    action->setEnabled( !allChildrenExpanded() );
    mnu.insertAction( action, 104 );

    action = new uiAction( tr("Collapse All Items") );
    action->setEnabled( !allChildrenCollapsed() );
    mnu.insertAction( action, 105 );
}


void uiODTreeItem::handleStandardItems( int mnuid )
{
    for ( int idx=0; idx<children_.size(); idx++ )
    {
	if ( mnuid == 101 )
	    children_[idx]->setChecked( true, true );
	else if ( mnuid == 102 )
	    children_[idx]->setChecked( false, true );
	else if ( mnuid == 104 )
	    children_[idx]->expand();
	else if ( mnuid == 105 )
	    children_[idx]->collapse();
    }

    if ( mnuid==103 )
    {
	const uiString msg = tr("All %1 items will be removed from the tree."
				"\n\nDo you want to continue?")
			   .arg(name());
	if ( !uiMSG().askRemove(msg) ) return;

	while ( children_.size() )
	{
	    setMoreObjectsToDoHint( children_.size()>1 );
	    mDynamicCastGet(uiODDisplayTreeItem*,itm,children_[0])
	    if ( !itm ) continue;
	    itm->prepareForShutdown();
	    applMgr()->visServer()->removeObject( itm->displayID(), sceneID() );
	    removeChild( itm );
	}
    }
}


void uiODTreeItem::setMoreObjectsToDoHint( bool yn )
{ applMgr()->visServer()->setMoreObjectsToDoHint( sceneID(), yn ); }


bool uiODTreeItem::getMoreObjectsToDoHint() const
{
    uiODApplMgr* applmgr = const_cast<uiODTreeItem*>(this)->applMgr();
    return applmgr->visServer()->getMoreObjectsToDoHint( sceneID() );
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
	itmbefore = findChild( itm->name().getFullString().buf() );
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
    const BufferString treenm( toString(dummy->name()) );
    const uiTreeItem* child = findChild( treenm.buf() );
    if ( !children_.isPresent(child) )
	return;

    removeChild( const_cast<uiTreeItem*>(child) );
}


// uiODParentTreeItem
uiODParentTreeItem::uiODParentTreeItem( const uiString& nm )
    : uiODTreeItem(nm)
{}


uiODParentTreeItem::~uiODParentTreeItem()
{
    detachAllNotifiers();
}


bool uiODParentTreeItem::init()
{
    if ( !uiODTreeItem::init() )
	return false;

    mAttachCB( checkStatusChange(), uiODParentTreeItem::checkCB );
    return true;
}


bool uiODParentTreeItem::anyButtonClick( uiTreeViewItem* itm )
{
    if ( itm==uitreeviewitem_ )
    {
	select();
	showSubMenu();
	return true;
    }

    return uiODTreeItem::anyButtonClick( itm );
}


void uiODParentTreeItem::show( bool yn )
{
    const bool doshow = yn && isChecked();
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODParentTreeItem*,pitm,getChild(idx))
	if ( pitm ) pitm->show( doshow );
	mDynamicCastGet(uiODDisplayTreeItem*,itm,getChild(idx))
	if ( itm ) itm->show( doshow );
    }
}


const char* uiODParentTreeItem::parentType() const
{ return typeid(uiODTreeTop).name(); }


int uiODParentTreeItem::uiTreeViewItemType() const
{
    return uiTreeViewItem::CheckBox;
}


void uiODParentTreeItem::checkCB( CallBacker* )
{
    const bool doshow = isChecked() && areAllParentsChecked();
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODParentTreeItem*,pitm,getChild(idx))
	if ( pitm ) pitm->show( doshow );
	mDynamicCastGet(uiODDisplayTreeItem*,itm,getChild(idx))
	if ( itm ) itm->show( doshow && itm->isChecked() );
    }
}


// uiODSceneTreeItem
uiODSceneTreeItem::uiODSceneTreeItem( const uiString& nm, VisID id )
    : uiODTreeItem(nm)
    , displayid_(id)
    , menu_(0)
    , propitem_( m3Dots(uiStrings::sProperties() ) )
    , imageitem_(m3Dots(tr("Top/Bottom Image")))
    , coltabitem_(m3Dots(tr("Scene Color Bar")))
    , dumpivitem_( m3Dots( uiStrings::phrExport( uiStrings::sScene() )) )
{
    propitem_.iconfnm = "disppars";
}


uiODSceneTreeItem::~uiODSceneTreeItem()
{
    if ( menu_ )
    {
	menu_->createnotifier.remove(
		mCB(this,uiODSceneTreeItem,createMenuCB) );
	menu_->handlenotifier.remove(
		mCB(this,uiODSceneTreeItem,handleMenuCB) );
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
	menu_->createnotifier.notify(
		mCB(this,uiODSceneTreeItem,createMenuCB) );
	menu_->handlenotifier.notify(
		mCB(this,uiODSceneTreeItem,handleMenuCB) );
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
    if ( !tb || tb->menuID() != displayid_.asInt() || !isSelected() )
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
	ODMainWin()->sceneMgr().get3DViewers( viewers );
	uiScenePropertyDlg dlg( getUiParent(), viewers,
				viewers.indexOf(viewer()) );
	dlg.go();
    }
    else if ( mnuid==imageitem_.id )
	visserv->setTopBotImg( displayid_ );
    else if ( mnuid==coltabitem_.id )
	visserv->manageSceneColorbar( displayid_ );
    else if( mnuid==dumpivitem_.id )
	visserv->writeSceneToFile( displayid_,
			    uiStrings::phrExport( uiStrings::sScene() ));
}


void uiODSceneTreeItem::updateColumnText( int col )
{
    if ( col==uiODSceneMgr::cNameColumn() )
	name_ = applMgr()->visServer()->getUiObjectName( displayid_ );

    uiTreeItem::updateColumnText( col );
}


bool uiODSceneTreeItem::showSubMenu()
{
    return menu_->executeMenu( uiMenuHandler::fromTree() );
}
