    /*+
    ___________________________________________________________________

     (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
     Author:	K. Tingdahl
     Date:		Jul 2003
    ___________________________________________________________________

    -*/

#include "uioddisplaytreeitem.h"
#include "uiodscenetreeitem.h"
#include "uiodsceneparenttreeitem.h"
#include "uiodsceneobjtreeitem.h"

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
#include "saveablemanager.h"
#include "vissurvscene.h"


uiODSceneTreeTop::uiODSceneTreeTop( uiTreeView* lv, uiTreeFactorySet* tfs_,
				    int sceneid )
    : uiTreeTopItem(lv)
    , tfs(tfs_)
    , sceneid_(sceneid)
{
    mAttachCB( tfs->addnotifier, uiODSceneTreeTop::addFactoryCB );
    mAttachCB( tfs->removenotifier, uiODSceneTreeTop::removeFactoryCB );
}


uiODSceneTreeTop::~uiODSceneTreeTop()
{
    detachAllNotifiers();
}


int uiODSceneTreeTop::sceneID() const
{
    return sceneid_;
}


bool uiODSceneTreeTop::selectWithKey( int selkey )
{
    applMgr()->visServer()->setSelObjectId(selkey);
    return true;
}


uiODApplMgr* uiODSceneTreeTop::applMgr()
{
    return &ODMainWin()->applMgr();
}


TypeSet<int> uiODSceneTreeTop::getDisplayIds( int& selectedid, bool usechecked )
{
    TypeSet<int> dispids;
    loopOverChildrenIds( dispids, selectedid, usechecked, children_ );
    return dispids;
}


void uiODSceneTreeTop::loopOverChildrenIds(
	TypeSet<int>& dispids, int& selectedid, bool usechecked,
	const ObjectSet<uiTreeItem>& childrenlist )
{
    for ( int idx=0; idx<childrenlist.size(); idx++ )
	loopOverChildrenIds( dispids, selectedid,
			     usechecked, childrenlist[idx]->getChildren() );

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

	    if ( childrenlist[idy]->getItem()->isSelected() )
		selectedid = disptreeitem->displayID();
	}
    }
}


uiODSceneTreeItem::uiODSceneTreeItem( const uiString& nm )
    : uiPresManagedTreeItem(nm)
{}


void uiODSceneTreeItem::prepareForShutdown()
{
    uiPresManagedTreeItem::prepareForShutdown();
}


bool uiODSceneTreeItem::anyButtonClick( uiTreeViewItem* item )
{
    if ( item!=uitreeviewitem_ )
	return uiTreeItem::anyButtonClick( item );

    if ( !select() ) return false;

    applMgr()->hideColorTable();
    return true;
}


uiODApplMgr* uiODSceneTreeItem::applMgr() const
{ return &ODMainWin()->applMgr(); }


uiODApplMgr* uiODSceneTreeItem::applMgr()
{ return &ODMainWin()->applMgr(); }


int uiODSceneTreeItem::sceneID() const
{
    mDynamicCastGet(uiODSceneTreeItem*,scenetreeitem,parent_);
    mDynamicCastGet(uiODSceneParentTreeItem*,sceneptreeitem,parent_);
    mDynamicCastGet(uiODSceneTreeTop*,treetop,parent_);
    return scenetreeitem ? scenetreeitem->sceneID()
			 : sceneptreeitem ? sceneptreeitem->sceneID()
					  : treetop ? treetop->sceneID()
						    : -1;
}


void uiODSceneTreeItem::setMoreObjectsToDoHint( bool yn )
{ applMgr()->visServer()->setMoreObjectsToDoHint( sceneID(), yn ); }


bool uiODSceneTreeItem::getMoreObjectsToDoHint() const
{
    uiODApplMgr* applmgr = const_cast<uiODSceneTreeItem*>(this)->applMgr();
    return applmgr->visServer()->getMoreObjectsToDoHint( sceneID() );
}


Presentation::ViewerID uiODSceneTreeItem::viewerID() const
{
    return Presentation::ViewerID( uiODSceneMgr::theViewerTypeID(),
		     Presentation::ViewerObjID::get(sceneID()) );
}


void uiODSceneTreeTop::addFactoryCB( CallBacker* cb )
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
	itmbefore = findChild( toString(itm->name()) );
	break;
    }

    uiTreeItem* newitm = tfs->getFactory(factidx)->create();
    addChild( newitm, false );
    if ( itmbefore )
	newitm->moveItem( itmbefore );
}


void uiODSceneTreeTop::removeFactoryCB( CallBacker* cb )
{
    mCBCapsuleUnpack(int,idx,cb);
    PtrMan<uiTreeItem> dummy = tfs->getFactory(idx)->create();
    const uiTreeItem* child = findChild( toString(dummy->name()) );
    if ( !children_.isPresent(child) ) return;
    removeChild( const_cast<uiTreeItem*>(child) );
}

uiODSceneObjTreeItem::uiODSceneObjTreeItem( const uiString& nm, int id )
    : uiODSceneTreeItem(nm)
    , displayid_(id)
    , menu_(0)
    , propitem_( m3Dots(uiStrings::sProperties() ) )
    , imageitem_(m3Dots(tr("Top/Bottom Image")))
    , coltabitem_(m3Dots(tr("Scene Color Bar")))
    , dumpivitem_( m3Dots( uiStrings::phrExport( uiStrings::sScene() )) )
{
    propitem_.iconfnm = "disppars";
}


uiODSceneObjTreeItem::~uiODSceneObjTreeItem()
{
    if ( menu_ )
    {
	menu_->createnotifier.remove(
		mCB(this,uiODSceneObjTreeItem,createMenuCB) );
	menu_->handlenotifier.remove(
		mCB(this,uiODSceneObjTreeItem,handleMenuCB) );
	menu_->unRef();
    }

    MenuHandler* tb = applMgr()->visServer()->getToolBarHandler();
    if ( tb )
    {
	tb->createnotifier.remove(
		mCB(this,uiODSceneObjTreeItem,addToToolBarCB) );
	tb->handlenotifier.remove(
		mCB(this,uiODSceneObjTreeItem,handleMenuCB) );
    }
}


bool uiODSceneObjTreeItem::init()
{
    if ( !uiODSceneTreeItem::init() )
	return false;

    if ( !menu_ )
    {
	menu_ = new uiMenuHandler( getUiParent(), -1 );
	menu_->ref();
	menu_->createnotifier.notify(
		mCB(this,uiODSceneObjTreeItem,createMenuCB) );
	menu_->handlenotifier.notify(
		mCB(this,uiODSceneObjTreeItem,handleMenuCB) );
    }

    MenuHandler* tb = applMgr()->visServer()->getToolBarHandler();
    if ( tb )
    {
	tb->createnotifier.notify(
		mCB(this,uiODSceneObjTreeItem,addToToolBarCB) );
	tb->handlenotifier.notify(
		mCB(this,uiODSceneObjTreeItem,handleMenuCB) );
    }
    return true;
}


void uiODSceneObjTreeItem::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(MenuHandler*,menu,cb);
    createMenu( menu, false );
}


void uiODSceneObjTreeItem::addToToolBarCB( CallBacker* cb )
{
    mDynamicCastGet(uiTreeItemTBHandler*,tb,cb);
    if ( !tb || tb->menuID() != displayid_ || !isSelected() )
	return;

    createMenu( tb, true );
}


void uiODSceneObjTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    mAddMenuOrTBItem( istb, menu, menu, &propitem_, true, false );
    mAddMenuOrTBItem( istb, 0, menu, &imageitem_, true, false );
    mAddMenuOrTBItem( istb, 0, menu, &coltabitem_, true, false );
    bool enabdump = true;
    Settings::common().getYN(
	    IOPar::compKey("dTect","Dump OI Menu"), enabdump );
    mAddMenuOrTBItem( istb, 0, menu, &dumpivitem_, enabdump, false );
}


void uiODSceneObjTreeItem::handleMenuCB( CallBacker* cb )
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
	const ui3DViewer* vwr3d =
	    ODMainWin()->sceneMgr().get3DViewer( sceneID() );
	uiScenePropertyDlg dlg( getUiParent(), viewers,
				viewers.indexOf(vwr3d) );
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


int uiODSceneObjTreeItem::sceneID() const
{
    return displayid_;
}


void uiODSceneObjTreeItem::updateColumnText( int col )
{
    if ( col==uiODSceneMgr::cNameColumn() )
	name_ = toUiString(applMgr()->visServer()->getObjectName(displayid_) );

    uiTreeItem::updateColumnText( col );
}


bool uiODSceneObjTreeItem::showSubMenu()
{
    return menu_->executeMenu( uiMenuHandler::fromTree() );
}
