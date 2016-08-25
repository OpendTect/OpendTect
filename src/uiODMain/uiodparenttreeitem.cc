/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		August 2016
___________________________________________________________________

-*/

#include "uiodparenttreeitem.h"
#include "uioddisplaytreeitem.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uivispartserv.h"


uiODParentTreeItem::uiODParentTreeItem( const uiString& nm )
    : uiODTreeItem(nm)
{
}


uiODParentTreeItem::~uiODParentTreeItem()
{
    detachAllNotifiers();
}


bool uiODParentTreeItem::init()
{
    uiODScene* scene = applMgr()->sceneMgr().getScene( sceneID() );
    if ( !scene )
	return false;

    mAttachCB( scene->ObjAdded, uiODParentTreeItem::objAddedCB );
    mAttachCB( scene->VanishRequested,
	       uiODParentTreeItem::objVanishedCB );
    mAttachCB( scene->ShowRequested, uiODParentTreeItem::objShownCB );
    mAttachCB( scene->HideRequested, uiODParentTreeItem::objHiddenCB );
    mAttachCB( scene->ObjOrphaned, uiODParentTreeItem::objOrphanedCB );
    return uiODTreeItem::init();
}


void uiODParentTreeItem::objAddedCB( CallBacker* cber )
{
    mCBCapsuleUnpack( IOPar,objprinfopar,cber );
    BufferString objtypekey;
    objprinfopar.get( sKey::Type(), objtypekey );
    if ( objtypekey != childObjTypeKey() )
	return;

    mEnsureExecutedInMainThreadWithCapsule(
	    uiODParentTreeItem::objAddedCB, cbercaps )
    ObjPresentationInfo* prinfo = ODIFac().create( objprinfopar );
    const MultiID mid = prinfo->storedID();
    if ( mid.isUdf() )
	return;

    TypeSet<MultiID> setids;
    setids += mid;
    addChildren( setids );
}


void uiODParentTreeItem::objVanishedCB( CallBacker* cber )
{
    mCBCapsuleUnpack( IOPar,objprinfopar,cber );
    BufferString objtypekey;
    objprinfopar.get( sKey::Type(), objtypekey );
    if ( objtypekey != childObjTypeKey() )
	return;

    mEnsureExecutedInMainThreadWithCapsule(
	    uiODParentTreeItem::objVanishedCB, cbercaps )
    ObjPresentationInfo* prinfo = ODIFac().create( objprinfopar );
    const MultiID mid = prinfo->storedID();
    if ( mid.isUdf() )
	return;

    removeChildren( mid );
}


void uiODParentTreeItem::objShownCB( CallBacker* cber )
{
    mCBCapsuleUnpack( IOPar,objprinfopar,cber );
    BufferString objtypekey;
    objprinfopar.get( sKey::Type(), objtypekey );
    if ( objtypekey != childObjTypeKey() )
	return;

    mEnsureExecutedInMainThreadWithCapsule(
	    uiODParentTreeItem::objShownCB, cbercaps )
    ObjPresentationInfo* prinfo = ODIFac().create( objprinfopar );
    const MultiID mid = prinfo->storedID();
    if ( mid.isUdf() )
	return;

    showHideChildren( mid, true );
}


void uiODParentTreeItem::objHiddenCB( CallBacker* cber )
{
    mCBCapsuleUnpack( IOPar,objprinfopar,cber );
    BufferString objtypekey;
    objprinfopar.get( sKey::Type(), objtypekey );
    if ( objtypekey != childObjTypeKey() )
	return;

    mEnsureExecutedInMainThreadWithCapsule(
	    uiODParentTreeItem::objHiddenCB, cbercaps )
    ObjPresentationInfo* prinfo = ODIFac().create( objprinfopar );
    const MultiID mid = prinfo->storedID();
    if ( mid.isUdf() )
	return;

    showHideChildren( mid, false );
}


void uiODParentTreeItem::objOrphanedCB( CallBacker* cber )
{
    mCBCapsuleUnpack( IOPar,objprinfopar,cber );
    BufferString objtypekey;
    objprinfopar.get( sKey::Type(), objtypekey );
    if ( objtypekey != childObjTypeKey() )
	return;

    mEnsureExecutedInMainThreadWithCapsule(
	    uiODParentTreeItem::objHiddenCB, cbercaps )
    ObjPresentationInfo* prinfo = ODIFac().create( objprinfopar );
    const MultiID mid = prinfo->storedID();
    if ( mid.isUdf() )
	return;
    //TODO do something when we have clearer idea what to do when it happens
}


void uiODParentTreeItem::emitChildPRRequest(
	const MultiID& mid, OD::PresentationRequestType req )
{
    if ( mid.isUdf() )
	return;

    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODDisplayTreeItem*,childitem,
			getChild(idx))
	if ( !childitem || childitem->storedID() != mid )
	    continue;

	childitem->emitPRRequest( req );
    }
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

	childitem->prepareForShutdown();
	applMgr()->visServer()->removeObject( childitem->displayID(),
					      sceneID() );
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
