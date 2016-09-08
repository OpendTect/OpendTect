/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		August 2016
___________________________________________________________________

-*/

#include "uiodprmantreeitem.h"


uiODPrManagedTreeItem::uiODPrManagedTreeItem( const uiString& nm )
    : uiODTreeItem(nm)
{
}


void uiODPrManagedTreeItem::emitPRRequest( OD::PresentationRequestType req )
{
    PtrMan<OD::ObjPresentationInfo> objprinfo = getObjPRInfo();
    if ( !objprinfo )
	return;

    IOPar objprinfopar;
    objprinfo->fillPar( objprinfopar );
    OD::ViewerID vwrid = getViewerID();
    OD::PrMan().request( vwrid, req, objprinfopar );
}



uiODPrManagedParentTreeItem::uiODPrManagedParentTreeItem( const uiString& nm )
    : uiODTreeItem(nm)
{
}


uiODPrManagedParentTreeItem::~uiODPrManagedParentTreeItem()
{
    detachAllNotifiers();
}


void uiODPrManagedParentTreeItem::setPRManagedViewer(
	OD::PresentationManagedViewer& prmanvwr )
{
    mAttachCB( prmanvwr.ObjAdded, uiODPrManagedParentTreeItem::objAddedCB );
    mAttachCB( prmanvwr.VanishRequested,
	       uiODPrManagedParentTreeItem::objVanishedCB );
    mAttachCB( prmanvwr.ShowRequested, uiODPrManagedParentTreeItem::objShownCB);
    mAttachCB( prmanvwr.HideRequested,uiODPrManagedParentTreeItem::objHiddenCB);
    mAttachCB( prmanvwr.ObjOrphaned,uiODPrManagedParentTreeItem::objOrphanedCB);
}


void uiODPrManagedParentTreeItem::objAddedCB( CallBacker* cber )
{
    mCBCapsuleUnpack( IOPar,objprinfopar,cber );
    BufferString objtypekey;
    objprinfopar.get( sKey::Type(), objtypekey );
    if ( objtypekey != childObjTypeKey() )
	return;

    mEnsureExecutedInMainThreadWithCapsule(
	    uiODPrManagedParentTreeItem::objAddedCB, cbercaps )
    OD::ObjPresentationInfo* prinfo = OD::PRIFac().create( objprinfopar );
    const DBKey mid = prinfo->storedID();
    if ( mid.isUdf() )
	return;

    DBKeySet setids;
    setids += mid;
    addChildren( setids );
}


void uiODPrManagedParentTreeItem::objVanishedCB( CallBacker* cber )
{
    mCBCapsuleUnpack( IOPar,objprinfopar,cber );
    BufferString objtypekey;
    objprinfopar.get( sKey::Type(), objtypekey );
    if ( objtypekey != childObjTypeKey() )
	return;

    mEnsureExecutedInMainThreadWithCapsule(
	    uiODPrManagedParentTreeItem::objVanishedCB, cbercaps )
    OD::ObjPresentationInfo* prinfo = OD::PRIFac().create( objprinfopar );
    const DBKey mid = prinfo->storedID();
    if ( mid.isUdf() )
	return;

    removeChildren( mid );
}


void uiODPrManagedParentTreeItem::objShownCB( CallBacker* cber )
{
    mCBCapsuleUnpack( IOPar,objprinfopar,cber );
    BufferString objtypekey;
    objprinfopar.get( sKey::Type(), objtypekey );
    if ( objtypekey != childObjTypeKey() )
	return;

    mEnsureExecutedInMainThreadWithCapsule(
	    uiODPrManagedParentTreeItem::objShownCB, cbercaps )
    OD::ObjPresentationInfo* prinfo = OD::PRIFac().create( objprinfopar );
    const DBKey mid = prinfo->storedID();
    if ( mid.isUdf() )
	return;

    showHideChildren( mid, true );
}


void uiODPrManagedParentTreeItem::objHiddenCB( CallBacker* cber )
{
    mCBCapsuleUnpack( IOPar,objprinfopar,cber );
    BufferString objtypekey;
    objprinfopar.get( sKey::Type(), objtypekey );
    if ( objtypekey != childObjTypeKey() )
	return;

    mEnsureExecutedInMainThreadWithCapsule(
	    uiODPrManagedParentTreeItem::objHiddenCB, cbercaps )
    OD::ObjPresentationInfo* prinfo = OD::PRIFac().create( objprinfopar );
    const DBKey mid = prinfo->storedID();
    if ( mid.isUdf() )
	return;

    showHideChildren( mid, false );
}


void uiODPrManagedParentTreeItem::objOrphanedCB( CallBacker* cber )
{
    mCBCapsuleUnpack( IOPar,objprinfopar,cber );
    BufferString objtypekey;
    objprinfopar.get( sKey::Type(), objtypekey );
    if ( objtypekey != childObjTypeKey() )
	return;

    mEnsureExecutedInMainThreadWithCapsule(
	    uiODPrManagedParentTreeItem::objHiddenCB, cbercaps )
    OD::ObjPresentationInfo* prinfo = OD::PRIFac().create( objprinfopar );
    const DBKey mid = prinfo->storedID();
    if ( mid.isUdf() )
	return;
    //TODO do something when we have clearer idea what to do when it happens
}


void uiODPrManagedParentTreeItem::emitChildPRRequest(
	const DBKey& mid, OD::PresentationRequestType req )
{
    if ( mid.isUdf() )
	return;

    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODPrManagedTreeItem*,childitem,
			getChild(idx))
	if ( !childitem || childitem->storedID() != mid )
	    continue;

	childitem->emitPRRequest( req );
    }
}


void uiODPrManagedParentTreeItem::showHideChildren( const DBKey& mid,
						    bool show )
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODPrManagedTreeItem*,childitem,getChild(idx))
	if ( !childitem || mid!=childitem->storedID() )
	    continue;

	childitem->setChecked( show, false );
	childitem->handleItemCheck( false );
    }
}


void uiODPrManagedParentTreeItem::removeChildren( const DBKey& mid )
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODPrManagedTreeItem*,childitem,getChild(idx))
	if ( !childitem || mid!=childitem->storedID() )
	    continue;

	childitem->prepareForShutdown();
	removeChild( childitem );
    }
}


void uiODPrManagedParentTreeItem::getLoadedChildren(
	DBKeySet& mids ) const
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(const uiODPrManagedTreeItem*,childitem,getChild(idx))
	if ( !childitem )
	    continue;

	mids.addIfNew( childitem->storedID() );
    }
}


bool uiODPrManagedParentTreeItem::selectChild( const DBKey& mid )
{
    DBKeySet midsloaded;
    getLoadedChildren( midsloaded );
    if ( !midsloaded.isPresent(mid) )
	return false;

    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODPrManagedTreeItem*,childtreeitm,getChild(idx))
	if ( childtreeitm && mid==childtreeitm->storedID() )
	{
	    childtreeitm->select();
	    return true;
	}
    }

    return false;
}


void uiODPrManagedParentTreeItem::addChildren( const DBKeySet& setids )
{
    DBKeySet setidstobeloaded, setidsloaded;
    getLoadedChildren( setidsloaded );
    for ( int idx=0; idx<setids.size(); idx++ )
    {
	if ( !setidsloaded.isPresent(setids[idx]) )
	    setidstobeloaded.addIfNew( setids[idx] );
    }

    for ( int idx=0; idx<setidstobeloaded.size(); idx++ )
	addChildItem( setidstobeloaded[idx] );
}
