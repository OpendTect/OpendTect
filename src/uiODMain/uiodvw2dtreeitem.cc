/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Apr 2010
________________________________________________________________________

-*/

#include "uiodvw2dtreeitem.h"
#include "uiodviewer2d.h"
#include "uiodviewer2dmgr.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uitreeview.h"
#include "zaxistransform.h"
#include "emposid.h"
#include "uiempartserv.h"
#include "uimpepartserv.h"
#include "uivispartserv.h"
#include "uiflatviewwin.h"
#include "uiflatviewer.h"
#include "uigraphicsview.h"

#include "uiodvw2dhor2dtreeitem.h"
#include "uiodvw2dhor3dtreeitem.h"

#include "mpeengine.h"
#include "emmanager.h"
#include "emobject.h"
#include "ioman.h"
#include "ptrman.h"

#define mAddIdx		0
#define mAddInAllIdx	1
#define mRemoveIdx	10
#define mRemoveInAllIdx	11

const char* uiODView2DTreeTop::viewer2dptr()		{ return "Viewer2D"; }
const char* uiODView2DTreeTop::applmgrstr()		{ return "Applmgr"; }


uiODView2DTreeTop::uiODView2DTreeTop( uiTreeView* lv, uiODApplMgr* am,
				  uiODViewer2D* vw2d, uiTreeFactorySet* tfs )
    : uiTreeTopItem( lv, true )
    , tfs_( tfs )
{
    setPropertyPtr( applmgrstr(), am );
    setPropertyPtr( viewer2dptr(), vw2d );
    mAttachCB( tfs_->addnotifier, uiODView2DTreeTop::addFactoryCB );
    mAttachCB( tfs_->removenotifier, uiODView2DTreeTop::addFactoryCB );
}


uiODView2DTreeTop::~uiODView2DTreeTop()
{
    detachAllNotifiers();
}


bool uiODView2DTreeTop::selectWithKey( int selkey )
{
    //TODO send object manager signal about selection
    return true;
}


uiODApplMgr* uiODView2DTreeTop::applMgr()
{
    void* res = 0;
    getPropertyPtr( applmgrstr(), res );
    return reinterpret_cast<uiODApplMgr*>( res );
}


uiODViewer2D* uiODView2DTreeTop::viewer2D()
{
    void* res = 0;
    getPropertyPtr( viewer2dptr(), res );
    return reinterpret_cast<uiODViewer2D*>( res );
}


bool uiODView2DTreeTop::setZAxisTransform( ZAxisTransform* zat )
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DTreeItem*,itm,getChild(idx));
	itm->setZAxisTransform( zat );
    }

    return true;
}


void uiODView2DTreeTop::updSampling( const TrcKeyZSampling& cs, bool update )
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DTreeItem*,itm,getChild(idx));
	if ( itm ) itm->updSampling( cs, update );
    }
}


void uiODView2DTreeTop::updSelSpec( const Attrib::SelSpec* selspec, bool wva )
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DTreeItem*,itm,getChild(idx));
	if ( itm ) itm->updSelSpec( selspec, wva );
    }
}


void uiODView2DTreeTop::addFactoryCB( CallBacker* cb )
{
    mCBCapsuleUnpack(int,factidx,cb);
    const int newplaceidx = tfs_->getPlacementIdx( factidx );
    uiTreeItem* itmbefore = 0;
    int maxidx = -1;
    for ( int idx=0; idx<tfs_->nrFactories(); idx++ )
    {
	const int curidx = tfs_->getPlacementIdx( idx );
	if ( curidx>newplaceidx || curidx<maxidx || curidx==newplaceidx )
	    continue;

	maxidx = curidx;
    }
    for ( int idx=0; idx<tfs_->nrFactories(); idx++ )
    {
	if ( tfs_->getPlacementIdx(idx) != maxidx )
	    continue;

	PtrMan<uiTreeItem> itm = tfs_->getFactory(idx)->create();
	itmbefore = findChild( mFromUiStringTodo(itm->name()) );
	break;
    }

    uiTreeItem* newitm = tfs_->getFactory(factidx)->create();
    addChild( newitm, false );
    if ( itmbefore )
	newitm->moveItem( itmbefore );
}


void uiODView2DTreeTop::removeFactoryCB( CallBacker* cb )
{
    mCBCapsuleUnpack(int,idx,cb);
    PtrMan<uiTreeItem> dummy = tfs_->getFactory(idx)->create();
    const uiTreeItem* child = findChild( mFromUiStringTodo(dummy->name()) );
    if ( !children_.isPresent(child) ) return;
    removeChild( const_cast<uiTreeItem*>(child) );
}


#define mShowAllItemsMenuID 101
#define mHideAllItemsMenuID 102
#define mRemoveAllItemsMenuID 103

uiODView2DTreeItem::uiODView2DTreeItem( const uiString& nm )
    : uiTreeItem( nm )
    , datatransform_(0)
{
}


uiODView2DTreeItem::~uiODView2DTreeItem()
{
    detachAllNotifiers();
    if ( datatransform_ )
	datatransform_->unRef();
}


bool uiODView2DTreeItem::init()
{
    const char* iconnm = iconName();
    if ( iconnm ) uitreeviewitem_->setIcon( 0, iconnm );

    return uiTreeItem::init();
}


void uiODView2DTreeItem::addKeyBoardEvent( const EM::ObjectID& emid )
{
    for(int ivwr = 0; ivwr<viewer2D()->viewwin()->nrViewers(); ivwr++)
    {
	uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(ivwr);
	mAttachCB(vwr.rgbCanvas().getKeyboardEventHandler().keyPressed,
	    uiODView2DTreeItem::keyPressedCB);
    }

    if ( emid.isValid() )
	emobjid_ = emid;
}


void uiODView2DTreeItem::keyPressedCB( CallBacker* cb )
{
    if ( !uitreeviewitem_->isSelected() )
	return;

    mDynamicCastGet(const KeyboardEventHandler*,keh,cb)
    if ( !keh || !keh->hasEvent() )
	return;

    if ( KeyboardEvent::isSave(keh->event()) )
	doSave();
    else if ( KeyboardEvent::isSaveAs(keh->event()) )
	doSaveAs();
}


bool uiODView2DTreeItem::setZAxisTransform( ZAxisTransform* zat )
{
    if ( datatransform_ )
    {
	mDetachCB( datatransform_->changeNotifier(),
		   uiODView2DTreeItem::dataTransformCB );
	datatransform_->unRef();
    }

    datatransform_ = zat;
    if ( datatransform_ )
    {
	datatransform_->ref();
	mAttachCB( datatransform_->changeNotifier(),
		   uiODView2DTreeItem::dataTransformCB );
    }

    return true;
}


void uiODView2DTreeItem::insertStdSubMenu( uiMenu& menu )
{
    if ( children_.size() > 1 )
    {
	menu.insertSeparator();
	menu.insertAction(
		new uiAction(tr("Show all items")), mShowAllItemsMenuID );
	menu.insertAction(
		new uiAction(tr("Hide all items")), mHideAllItemsMenuID );
	menu.insertAction(
		new uiAction(tr("Remove all items")), mRemoveAllItemsMenuID );
    }
}


bool uiODView2DTreeItem::handleStdSubMenu( int menuid )
{
    if ( menuid == mShowAllItemsMenuID )
	showAllChildren();
    else if ( menuid == mHideAllItemsMenuID )
	hideAllChildren();
    else if ( menuid == mRemoveAllItemsMenuID )
	removeAllChildren();
    return true;
}


void uiODView2DTreeItem::addAction( uiMenu& mnu, uiString txt, int id,
				  const char* icon, bool enab )
{
    uiAction* action = new uiAction( txt );
    mnu.insertAction( action, id );
    action->setEnabled( enab );
    action->setIcon( icon );
}


uiMenu* uiODView2DTreeItem::createAddMenu()
{
    uiMenu* addmenu = new uiMenu( uiStrings::sAdd() );
    addAction( *addmenu, m3Dots(tr("Only in this 2D Viewer")), mAddIdx );
    const int nrvwrs = applMgr()->viewer2DMgr().nr2DViewers();
    addAction( *addmenu, m3Dots(tr("In all 2D Viewers")), mAddInAllIdx,
	       0, nrvwrs>1 );
    return addmenu;
}


bool uiODView2DTreeItem::isAddItem( int id, bool addall ) const
{
    return addall ? id==mAddInAllIdx : id==mAddIdx;
}


uiMenu* uiODView2DTreeItem::createRemoveMenu()
{
    uiMenu* removemenu = new uiMenu( uiStrings::sRemove(), "remove" );
    addAction( *removemenu, m3Dots(tr("Only from this 2D Viewer")), mRemoveIdx);
    const int nrvwrs = applMgr()->viewer2DMgr().nr2DViewers();
    addAction( *removemenu, m3Dots(tr("From all 2D Viewers")), mRemoveInAllIdx,
	       0, nrvwrs>1 );
    return removemenu;
}


bool uiODView2DTreeItem::isRemoveItem( int id, bool removeall ) const
{
    return removeall ? id==mRemoveInAllIdx : id==mRemoveIdx;
}


int uiODView2DTreeItem::getNewItemID() const
{ return mAddInAllIdx+1; }


void uiODView2DTreeItem::updSampling( const TrcKeyZSampling& cs, bool update )
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DTreeItem*,itm,getChild(idx));
	if ( itm ) itm->updSampling( cs, update );
    }

    updateCS( cs, update );
}


void uiODView2DTreeItem::updSelSpec(const Attrib::SelSpec* selspec, bool wva )
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DTreeItem*,itm,getChild(idx));
	if ( itm ) itm->updSelSpec( selspec, wva );
    }

    updateSelSpec( selspec, wva );
}


void uiODView2DTreeItem::showAllChildren()
{
    for ( int idx=children_.size()-1; idx>=0; idx-- )
	children_[idx]->setChecked( true, true );
}


void uiODView2DTreeItem::hideAllChildren()
{
    for ( int idx=children_.size()-1; idx>=0; idx-- )
	children_[idx]->setChecked( false, true );
}


void uiODView2DTreeItem::removeAllChildren()
{
    const uiString msg = tr("All %1 items will be removed from the tree."
			    "\n\nDo you want to continue?").arg(name());
    if ( !uiMSG().askRemove(msg) ) return;

    for ( int idx=children_.size()-1; idx>=0; idx-- )
	removeChild( children_[idx] );
}


uiODApplMgr* uiODView2DTreeItem::applMgr()
{
    void* res = 0;
    getPropertyPtr( uiODView2DTreeTop::applmgrstr(), res );
    return reinterpret_cast<uiODApplMgr*>( res );
}


uiODViewer2D* uiODView2DTreeItem::viewer2D()
{
    void* res = 0;
    getPropertyPtr( uiODView2DTreeTop::viewer2dptr(), res );
    return reinterpret_cast<uiODViewer2D*>( res );
}


bool uiODView2DTreeItem::create( uiTreeItem* treeitem, VisID visid,
				Vis2DID vis2did )
{
    uiODViewer2D* vwr2d = ODMainWin()->viewer2DMgr().find2DViewer( visid );
    if ( !vwr2d ) return false;

    return create( treeitem, *vwr2d, vis2did );
}


bool uiODView2DTreeItem::create( uiTreeItem* treeitem,
				const uiODViewer2D& vwr2d,
				Vis2DID vis2did )
{
    const uiTreeFactorySet* tfs = vwr2d.uiTreeItemFactorySet();
    if ( !tfs )
	return false;

    for ( int idx=0; idx<tfs->nrFactories(); idx++ )
    {
	mDynamicCastGet(const uiODView2DTreeItemFactory*,itmcreater,
			tfs->getFactory(idx))
	if ( !itmcreater ) continue;

	uiTreeItem* res = itmcreater->createForVis( vwr2d, vis2did );
	if ( res )
	{
	    if ( treeitem->addChild( res, false ) )
		return true;
	}
    }
    return false;
}


const uiODView2DTreeItem* uiODView2DTreeItem::getView2DItem( Vis2DID id ) const
{
    if ( displayid_ == id )
	return this;

    for ( int idx=0; idx<children_.size(); idx++ )
    {
	mDynamicCastGet(const uiODView2DTreeItem*,vw2ditem,children_[idx]);
	const uiODView2DTreeItem* chliditem = vw2ditem->getView2DItem( id );
	if ( chliditem )
	    return chliditem;
    }

    return 0;
}


const uiODView2DTreeItem* uiODView2DTreeTop::getView2DItem( Vis2DID id ) const
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	const uiTreeItem* childitm = getChild( idx );
	mDynamicCastGet(const uiODView2DTreeItem*,vw2dtreeitm,childitm)
	if ( !vw2dtreeitm )
	    continue;

	const uiODView2DTreeItem* childvw2ditm =
					vw2dtreeitm->getView2DItem( id );
	if ( childvw2ditm )
	    return childvw2ditm;
    }

    return nullptr;
}


void uiODView2DTreeItem::doSave()
{
    if ( !emobjid_.isValid() )
	return;

    bool savewithname = false;
    if ( !EM::EMM().getMultiID(emobjid_).isUdf() )
	savewithname = !IOM().get( EM::EMM().getMultiID(emobjid_) );
    doStoreObject( savewithname );

    if ( MPE::engine().hasTracker(emobjid_) )
    {
	uiMPEPartServer* mps = applMgr()->mpeServer();
	if ( mps )
	    mps->saveSetup( applMgr()->EMServer()->getStorageID(emobjid_) );
    }
}


void uiODView2DTreeItem::doSaveAs()
{
    if ( !emobjid_.isValid() )
	return;

    doStoreObject( true );

    if ( MPE::engine().hasTracker(emobjid_) )
    {
	uiMPEPartServer* mps = applMgr()->mpeServer();
	if ( mps )
	{
	   const MultiID oldmid =
		applMgr()->EMServer()->getStorageID( emobjid_ );
	   mps->prepareSaveSetupAs( oldmid );
	   mps->saveSetupAs( EM::EMM().getObject(emobjid_)->multiID() );
	}
    }
}


void uiODView2DTreeItem::doStoreObject( bool saveas )
{
    if ( !emobjid_.isValid() )
	return;

    applMgr()->EMServer()->storeObject( emobjid_, saveas );
    renameVisObj();
}


void uiODView2DTreeItem::renameVisObj()
{
    if ( !emobjid_.isValid() )
	return;

    const MultiID midintree = applMgr()->EMServer()->getStorageID( emobjid_ );
    TypeSet<VisID> visobjids;
    applMgr()->visServer()->findObject( midintree, visobjids );
    name_ = applMgr()->EMServer()->getUiName( emobjid_ );
    for ( int idx = 0; idx<visobjids.size(); idx++ )
	applMgr()->visServer()->setUiObjectName( visobjids[idx], name_ );

    uiTreeItem::updateColumnText(uiODViewer2DMgr::cNameColumn());
    applMgr()->visServer()->triggerTreeUpdate();
}
