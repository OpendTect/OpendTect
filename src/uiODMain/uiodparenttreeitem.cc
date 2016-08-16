/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		August 2016
___________________________________________________________________

-*/

#include "uiodparenttreeitem.h"
#include "uioddisplaytreeitem.h"
#include "saveablemanager.h"
#include "odpresentationmgr.h"


uiODParentTreeItem::uiODParentTreeItem( const uiString& nm )
    : uiODTreeItem(nm)
    , objectmgr_(0)
{
}


void uiODParentTreeItem::setObjectManager( SaveableManager* mgr )
{
    if ( objectmgr_ )
    {
	mDetachCB( objectmgr_->ObjAdded, uiODParentTreeItem::objAddedCB );
	mDetachCB( objectmgr_->VanishRequested,
		   uiODParentTreeItem::objVanishedCB );
	mDetachCB( objectmgr_->ShowRequested,
		   uiODParentTreeItem::objShownCB );
	mDetachCB( objectmgr_->HideRequested,
		   uiODParentTreeItem::objHiddenCB );
	mDetachCB( objectmgr_->ObjOrphaned,
		   uiODParentTreeItem::objOrphanedCB );
    }

    objectmgr_ = mgr;
    if ( !mgr )
	return;

    mAttachCB( objectmgr_->ObjAdded, uiODParentTreeItem::objAddedCB );
    mAttachCB( objectmgr_->VanishRequested,
	       uiODParentTreeItem::objVanishedCB );
    mAttachCB( objectmgr_->ShowRequested, uiODParentTreeItem::objShownCB );
    mAttachCB( objectmgr_->HideRequested, uiODParentTreeItem::objHiddenCB );
    mAttachCB( objectmgr_->ObjOrphaned, uiODParentTreeItem::objOrphanedCB );
}


void uiODParentTreeItem::objAddedCB( CallBacker* cber )
{
    if ( !ODPrMan().isSyncedWithTriggerDomain(ODPresentationManager::Scene3D) )
	return;

    mCBCapsuleUnpack( MultiID,mid,cber );
    mEnsureExecutedInMainThreadWithCapsule(
	    uiODParentTreeItem::objAddedCB, cbercaps )
    if ( mid.isUdf() )
	return;

    TypeSet<MultiID> setids;
    setids += mid;
    addChildren( setids );
}


void uiODParentTreeItem::objVanishedCB( CallBacker* cber )
{
    if ( !ODPrMan().isSyncedWithTriggerDomain(ODPresentationManager::Scene3D) )
	return;

    mCBCapsuleUnpack( MultiID,mid,cber );
    mEnsureExecutedInMainThreadWithCapsule(
	    uiODParentTreeItem::objVanishedCB, cbercaps )
    if ( mid.isUdf() )
	return;

    removeChildren( mid );
}


void uiODParentTreeItem::objShownCB( CallBacker* cber )
{
    if ( !ODPrMan().isSyncedWithTriggerDomain(ODPresentationManager::Scene3D) )
	return;

    mCBCapsuleUnpack( MultiID,mid,cber );
    mEnsureExecutedInMainThreadWithCapsule(
	    uiODParentTreeItem::objShownCB, cbercaps )
    if ( mid.isUdf() )
	return;

    showHideChildren( mid, true );
}


void uiODParentTreeItem::objHiddenCB( CallBacker* cber )
{
    if ( !ODPrMan().isSyncedWithTriggerDomain(ODPresentationManager::Scene3D) )
	return;

    mCBCapsuleUnpack( MultiID,mid,cber );
    mEnsureExecutedInMainThreadWithCapsule(
	    uiODParentTreeItem::objHiddenCB, cbercaps )
    if ( mid.isUdf() )
	return;

    showHideChildren( mid, false );
}


void uiODParentTreeItem::objOrphanedCB( CallBacker* cber )
{
    if ( !ODPrMan().isSyncedWithTriggerDomain(ODPresentationManager::Scene3D) )
	return;

    mCBCapsuleUnpack( MultiID,mid,cber );
    mEnsureExecutedInMainThreadWithCapsule(
	    uiODParentTreeItem::objOrphanedCB, cbercaps )
    if ( mid.isUdf() )
	return;
    //TODO do something when we have clearer idea what to do when it happens
}


void uiODParentTreeItem::showHideChildren( const MultiID& mid, bool show )
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODDisplayTreeItem*,childitem,getChild(idx))
	if ( !childitem || mid!=childitem->storedID() )
	    continue;

	childitem->setChecked( show, false );
	childitem->handleItemCheck( false );
    }
}


void uiODParentTreeItem::removeChildren( const MultiID& mid )
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODDisplayTreeItem*,childitem,getChild(idx))
	if ( !childitem || mid!=childitem->storedID() )
	    continue;

	removeChild( childitem );
    }
}


void uiODParentTreeItem::getLoadedChildren( TypeSet<MultiID>& mids ) const
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(const uiODDisplayTreeItem*,childitem,getChild(idx))
	if ( !childitem )
	    continue;

	mids.addIfNew( childitem->storedID() );
    }
}


bool uiODParentTreeItem::selectChild( const MultiID& mid )
{
    TypeSet<MultiID> midsloaded;
    getLoadedChildren( midsloaded );
    if ( !midsloaded.isPresent(mid) )
	return false;

    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODDisplayTreeItem*,childtreeitm,getChild(idx))
	if ( childtreeitm && mid==childtreeitm->storedID() )
	{
	    childtreeitm->select();
	    return true;
	}
    }

    return false;
}


void uiODParentTreeItem::addChildren( const TypeSet<MultiID>& setids )
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
