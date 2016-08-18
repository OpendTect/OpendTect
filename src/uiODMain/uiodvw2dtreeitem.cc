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
#include "uiflatviewwin.h"
#include "uiflatviewer.h"
#include "uigraphicsview.h"
#include "uivispartserv.h"

#include "uimenu.h"
#include "uimsg.h"
#include "uitreeview.h"
#include "odviewer2dpresentationmgr.h"
#include "view2ddata.h"
#include "zaxistransform.h"

#define mAddIdx		0
#define mAddInAllIdx	1

const char* uiODVw2DTreeTop::viewer2dptr() 		{ return "Viewer2D"; }
const char* uiODVw2DTreeTop::applmgrstr()		{ return "Applmgr"; }


uiODVw2DTreeTop::uiODVw2DTreeTop( uiTreeView* lv, uiODApplMgr* am,
				  uiODViewer2D* vw2d, uiTreeFactorySet* tfs )
    : uiTreeTopItem( lv, true )
    , tfs_( tfs )
{
    setPropertyPtr( applmgrstr(), am );
    setPropertyPtr( viewer2dptr(), vw2d );
    mAttachCB( tfs_->addnotifier, uiODVw2DTreeTop::addFactoryCB );
    mAttachCB( tfs_->removenotifier, uiODVw2DTreeTop::addFactoryCB );
}


uiODVw2DTreeTop::~uiODVw2DTreeTop()
{
    detachAllNotifiers();
}


bool uiODVw2DTreeTop::selectWithKey( int selkey )
{
    //TODO send object manager signal about selection
    return true;
}


uiODApplMgr* uiODVw2DTreeTop::applMgr()
{
    void* res = 0;
    getPropertyPtr( applmgrstr(), res );
    return reinterpret_cast<uiODApplMgr*>( res );
}


uiODViewer2D* uiODVw2DTreeTop::viewer2D()
{
    void* res = 0;
    getPropertyPtr( viewer2dptr(), res );
    return reinterpret_cast<uiODViewer2D*>( res );
}


bool uiODVw2DTreeTop::setZAxisTransform( ZAxisTransform* zat )
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DTreeItem*,itm,getChild(idx));
	itm->setZAxisTransform( zat );
    }

    return true;
}


void uiODVw2DTreeTop::updSampling( const TrcKeyZSampling& cs, bool update )
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DTreeItem*,itm,getChild(idx));
	if ( itm ) itm->updSampling( cs, update );
    }
}


void uiODVw2DTreeTop::updSelSpec( const Attrib::SelSpec* selspec, bool wva )
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DTreeItem*,itm,getChild(idx));
	if ( itm ) itm->updSelSpec( selspec, wva );
    }
}


void uiODVw2DTreeTop::addFactoryCB( CallBacker* cb )
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


void uiODVw2DTreeTop::removeFactoryCB( CallBacker* cb )
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

uiODVw2DTreeItem::uiODVw2DTreeItem( const uiString& nm )
    : uiTreeItem( nm )
    , displayid_(-1)
    , datatransform_(0)
    , storedid_( MultiID::udf() )
{}


uiODVw2DTreeItem::~uiODVw2DTreeItem()
{
    detachAllNotifiers();
    if ( datatransform_ )
	datatransform_->unRef();
}


void uiODVw2DTreeItem::emitPRRequest( ODPresentationManager::RequestType req )
{
    PtrMan<ObjPresentationInfo> objprinfo = getObjPRInfo();
    if ( !objprinfo )
	return;

    IOPar objprinfopar;
    objprinfo->fillPar( objprinfopar );
    ODPrMan().request( Viewer2DPresentationMgr::sViewerTypeID(), req,
		       objprinfopar );
}


bool uiODVw2DTreeItem::init()
{
    const char* iconnm = iconName();
    if ( iconnm ) uitreeviewitem_->setIcon( 0, iconnm );

    return uiTreeItem::init();
}


void uiODVw2DTreeItem::addKeyBoardEvent()
{
    for ( int ivwr=0; ivwr<viewer2D()->viewwin()->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewer2D()->viewwin()->viewer( ivwr );
	mAttachCB( vwr.rgbCanvas().getKeyboardEventHandler().keyPressed,
	uiODVw2DTreeItem::keyPressedCB );
    }
}


void uiODVw2DTreeItem::keyPressedCB( CallBacker* cb )
{
    if ( !uitreeviewitem_->isSelected() )
	return;

    mDynamicCastGet( const KeyboardEventHandler*, keh, cb );
    if ( !keh || !keh->hasEvent() ) return;

    if ( KeyboardEvent::isSave(keh->event()) )
	doSave();
    else if ( KeyboardEvent::isSaveAs(keh->event()) )
	doSaveAs();
}


bool uiODVw2DTreeItem::setZAxisTransform( ZAxisTransform* zat )
{
    if ( datatransform_ )
    {
	mDetachCB( datatransform_->changeNotifier(),
		   uiODVw2DTreeItem::dataTransformCB );
	datatransform_->unRef();
    }

    datatransform_ = zat;
    if ( datatransform_ )
    {
	datatransform_->ref();
	mAttachCB( datatransform_->changeNotifier(),
		   uiODVw2DTreeItem::dataTransformCB );
    }

    return true;
}


void uiODVw2DTreeItem::insertStdSubMenu( uiMenu& menu )
{
    if ( children_.size() > 1 )
    {
	menu.insertSeparator();
	menu.insertItem(
		new uiAction(tr("Show all items")), mShowAllItemsMenuID );
	menu.insertItem(
		new uiAction(tr("Hide all items")), mHideAllItemsMenuID );
	menu.insertItem(
		new uiAction(tr("Remove all items")), mRemoveAllItemsMenuID );
    }
}


bool uiODVw2DTreeItem::handleStdSubMenu( int menuid )
{
    if ( menuid == mShowAllItemsMenuID )
	showAllChildren();
    else if ( menuid == mHideAllItemsMenuID )
	hideAllChildren();
    else if ( menuid == mRemoveAllItemsMenuID )
	removeAllChildren();
    return true;
}


void uiODVw2DTreeItem::addAction( uiMenu& mnu, uiString txt, int id,
				  const char* icon, bool enab )
{
    uiAction* action = new uiAction( txt );
    mnu.insertAction( action, id );
    action->setEnabled( enab );
    action->setIcon( icon );
}


uiMenu* uiODVw2DTreeItem::createAddMenu()
{
    uiMenu* addmenu = new uiMenu( uiStrings::sAdd() );
    addAction( *addmenu, m3Dots(tr("Only in this 2D Viewer")), mAddIdx );
    const int nrvwrs = applMgr()->viewer2DMgr().nr2DViewers();
    addAction( *addmenu, m3Dots(tr("In all 2D Viewers")), mAddInAllIdx,
	       0, nrvwrs>1 );
    return addmenu;
}


bool uiODVw2DTreeItem::isAddItem( int id, bool addall ) const
{
    return addall ? id==mAddInAllIdx : id==mAddIdx;
}


int uiODVw2DTreeItem::getNewItemID() const
{ return mAddInAllIdx+1; }


void uiODVw2DTreeItem::updSampling( const TrcKeyZSampling& cs, bool update )
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DTreeItem*,itm,getChild(idx));
	if ( itm ) itm->updSampling( cs, update );
    }

    updateCS( cs, update );
}


void uiODVw2DTreeItem::updSelSpec(const Attrib::SelSpec* selspec, bool wva )
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DTreeItem*,itm,getChild(idx));
	if ( itm ) itm->updSelSpec( selspec, wva );
    }

    updateSelSpec( selspec, wva );
}


void uiODVw2DTreeItem::showAllChildren()
{
    for ( int idx=children_.size()-1; idx>=0; idx-- )
	children_[idx]->setChecked( true, true );
}


void uiODVw2DTreeItem::hideAllChildren()
{
    for ( int idx=children_.size()-1; idx>=0; idx-- )
	children_[idx]->setChecked( false, true );
}


void uiODVw2DTreeItem::removeAllChildren()
{
    const uiString msg = tr("All %1 items will be removed from the tree."
			    "\n\nDo you want to continue?").arg(name());
    if ( !uiMSG().askRemove(msg) ) return;

    for ( int idx=children_.size()-1; idx>=0; idx-- )
	removeChild( children_[idx] );
}


uiODApplMgr* uiODVw2DTreeItem::applMgr()
{
    void* res = 0;
    getPropertyPtr( uiODVw2DTreeTop::applmgrstr(), res );
    return reinterpret_cast<uiODApplMgr*>( res );
}


uiODViewer2D* uiODVw2DTreeItem::viewer2D()
{
    void* res = 0;
    getPropertyPtr( uiODVw2DTreeTop::viewer2dptr(), res );
    return reinterpret_cast<uiODViewer2D*>( res );
}


bool uiODVw2DTreeItem::create( uiTreeItem* treeitem, int visid, int displayid )
{
    uiODViewer2D* vwr2d = ODMainWin()->viewer2DMgr().find2DViewer(visid,true);
    if ( !vwr2d ) return false;

    return create( treeitem, *vwr2d, displayid );
}


bool uiODVw2DTreeItem::create(
		uiTreeItem* treeitem, const uiODViewer2D& vwr2d, int displayid )
{
    const uiTreeFactorySet* tfs = vwr2d.uiTreeItemFactorySet();
    if ( !tfs )
	return false;

    for ( int idx=0; idx<tfs->nrFactories(); idx++ )
    {
	mDynamicCastGet(const uiODVw2DTreeItemFactory*,itmcreater,
			tfs->getFactory(idx))
	if ( !itmcreater ) continue;

	uiTreeItem* res = itmcreater->createForVis( vwr2d, displayid );
	if ( res )
	{
	    if ( treeitem->addChild( res, false ) )
		return true;
	}
    }
    return false;
}


const uiODVw2DTreeItem* uiODVw2DTreeItem::getVW2DItem( int displayid ) const
{
    if ( displayid_ == displayid )
	return this;

    for ( int idx=0; idx<children_.size(); idx++ )
    {
	mDynamicCastGet(const uiODVw2DTreeItem*,vw2ditem,children_[idx]);
	const uiODVw2DTreeItem* chliditem = vw2ditem->getVW2DItem( displayid );
	if ( chliditem )
	    return chliditem;
    }

    return 0;
}


const uiODVw2DTreeItem* uiODVw2DTreeTop::getVW2DItem( int displayid ) const
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	const uiTreeItem* childitm = getChild( idx );
	mDynamicCastGet(const uiODVw2DTreeItem*,vw2dtreeitm,childitm)
	if ( !vw2dtreeitm )
	    continue;

	const uiODVw2DTreeItem* childvw2ditm =
	    vw2dtreeitm->getVW2DItem( displayid );
	if ( childvw2ditm )
	    return childvw2ditm;
    }

    return 0;
}

uiODVw2DParentTreeItem::uiODVw2DParentTreeItem( const uiString& nm )
    : uiODVw2DTreeItem( nm )
{
}


uiODVw2DParentTreeItem::~uiODVw2DParentTreeItem()
{
    detachAllNotifiers();
}


bool uiODVw2DParentTreeItem::init()
{
    ODVwrTypePresentationMgr* vtmgr =
	ODPrMan().getViewerTypeMgr( Viewer2DPresentationMgr::sViewerTypeID() );
    if ( !vtmgr )
	return false;

    mAttachCB( vtmgr->ObjAdded, uiODVw2DParentTreeItem::objAddedCB );
    mAttachCB( vtmgr->VanishRequested,
	       uiODVw2DParentTreeItem::objVanishedCB );
    mAttachCB( vtmgr->ShowRequested, uiODVw2DParentTreeItem::objShownCB );
    mAttachCB( vtmgr->HideRequested, uiODVw2DParentTreeItem::objHiddenCB );
    mAttachCB( vtmgr->ObjOrphaned, uiODVw2DParentTreeItem::objOrphanedCB );
    return uiODVw2DTreeItem::init();
}


void uiODVw2DParentTreeItem::objAddedCB( CallBacker* cber )
{
    mCBCapsuleUnpack( IOPar,objprinfopar,cber );
    BufferString objtypekey;
    objprinfopar.get( sKey::Type(), objtypekey );
    if ( objtypekey != childObjTypeKey() )
	return;

    mEnsureExecutedInMainThreadWithCapsule(
	    uiODVw2DParentTreeItem::objAddedCB, cbercaps )
    ObjPresentationInfo* prinfo = ODIFac().create( objprinfopar );
    const MultiID mid = prinfo->storedID();
    if ( mid.isUdf() )
	return;

    TypeSet<MultiID> setids;
    setids += mid;
    addChildren( setids );
}


void uiODVw2DParentTreeItem::objVanishedCB( CallBacker* cber )
{
    mCBCapsuleUnpack( IOPar,objprinfopar,cber );
    BufferString objtypekey;
    objprinfopar.get( sKey::Type(), objtypekey );
    if ( objtypekey != childObjTypeKey() )
	return;

    mEnsureExecutedInMainThreadWithCapsule(
	    uiODVw2DParentTreeItem::objVanishedCB, cbercaps )
    ObjPresentationInfo* prinfo = ODIFac().create( objprinfopar );
    const MultiID mid = prinfo->storedID();
    if ( mid.isUdf() )
	return;

    removeChildren( mid );
}


void uiODVw2DParentTreeItem::objShownCB( CallBacker* cber )
{
    mCBCapsuleUnpack( IOPar,objprinfopar,cber );
    BufferString objtypekey;
    objprinfopar.get( sKey::Type(), objtypekey );
    if ( objtypekey != childObjTypeKey() )
	return;

    mEnsureExecutedInMainThreadWithCapsule(
	    uiODVw2DParentTreeItem::objShownCB, cbercaps )
    ObjPresentationInfo* prinfo = ODIFac().create( objprinfopar );
    const MultiID mid = prinfo->storedID();
    if ( mid.isUdf() )
	return;

    showHideChildren( mid, true );
}


void uiODVw2DParentTreeItem::objHiddenCB( CallBacker* cber )
{
    mCBCapsuleUnpack( IOPar,objprinfopar,cber );
    BufferString objtypekey;
    objprinfopar.get( sKey::Type(), objtypekey );
    if ( objtypekey != childObjTypeKey() )
	return;

    mEnsureExecutedInMainThreadWithCapsule(
	    uiODVw2DParentTreeItem::objHiddenCB, cbercaps )
    ObjPresentationInfo* prinfo = ODIFac().create( objprinfopar );
    const MultiID mid = prinfo->storedID();
    if ( mid.isUdf() )
	return;

    showHideChildren( mid, false );
}


void uiODVw2DParentTreeItem::objOrphanedCB( CallBacker* cber )
{
    mCBCapsuleUnpack( IOPar,objprinfopar,cber );
    BufferString objtypekey;
    objprinfopar.get( sKey::Type(), objtypekey );
    if ( objtypekey != childObjTypeKey() )
	return;

    mEnsureExecutedInMainThreadWithCapsule(
	    uiODVw2DParentTreeItem::objOrphanedCB, cbercaps )
    ObjPresentationInfo* prinfo = ODIFac().create( objprinfopar );
    const MultiID mid = prinfo->storedID();
    if ( mid.isUdf() )
	return;
    //TODO do something when we have clearer idea what to do when it happens
}


void uiODVw2DParentTreeItem::emitChildPRRequest(
	const MultiID& childstoredid, ODPresentationManager::RequestType req )
{
    if ( childstoredid.isUdf() )
	return;

    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DTreeItem*,childitem,
			getChild(idx))
	if ( !childitem || childitem->storedID() != childstoredid )
	    continue;

	childitem->emitPRRequest( req );
    }
}


void uiODVw2DParentTreeItem::getVwr2DOjIDs(
	const MultiID& mid, TypeSet<int>& vw2dobjids ) const
{
    if ( mid.isUdf() )
	return;

    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(const uiODVw2DTreeItem*,childitem,
			getChild(idx))
	if ( !childitem || childitem->storedID() != mid )
	    continue;

	if ( childitem->vw2DObject() )
	    vw2dobjids.addIfNew( childitem->vw2DObject()->id() );
    }
}


void uiODVw2DParentTreeItem::showHideChildren( const MultiID& mid, bool show )
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DTreeItem*,childitem,getChild(idx))
	if ( !childitem || mid!=childitem->storedID() )
	    continue;

	childitem->setChecked( show, false );
	childitem->enableDisplay( show, false );
    }
}


void uiODVw2DParentTreeItem::removeChildren( const MultiID& mid )
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DTreeItem*,childitem,getChild(idx))
	if ( !childitem || mid!=childitem->storedID() )
	    continue;

	removeChild( childitem );
    }
}


void uiODVw2DParentTreeItem::getLoadedChildren( TypeSet<MultiID>& mids ) const
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(const uiODVw2DTreeItem*,childitem,getChild(idx))
	if ( !childitem )
	    continue;

	mids.addIfNew( childitem->storedID() );
    }
}


bool uiODVw2DParentTreeItem::selectChild( const MultiID& mid )
{
    TypeSet<MultiID> midsloaded;
    getLoadedChildren( midsloaded );
    if ( !midsloaded.isPresent(mid) )
	return false;

    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DTreeItem*,childtreeitm,getChild(idx))
	if ( childtreeitm && mid==childtreeitm->storedID() )
	{
	    childtreeitm->select();
	    return true;
	}
    }

    return false;
}


void uiODVw2DParentTreeItem::addChildren( const TypeSet<MultiID>& setids )
{
    TypeSet<MultiID> setidstobeloaded, setidsloaded;
    getLoadedChildren( setidsloaded );
    for ( int idx=0; idx<setids.size(); idx++ )
    {
	if ( !setidsloaded.isPresent(setids[idx]) )
	    setidstobeloaded.addIfNew( setids[idx] );
    }

    for ( int idx=0; idx<setidstobeloaded.size(); idx++ )
	addChildItem( setidstobeloaded[idx] );
}
