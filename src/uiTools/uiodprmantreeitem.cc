/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		August 2016
___________________________________________________________________

-*/

#include "uiodprmantreeitem.h"
#include "sharedobject.h"
#include "keystrs.h"


uiPresManagedTreeItem::uiPresManagedTreeItem( const uiString& nm )
    : uiODTreeItem(nm)
    , dataobj_(0)
{
}


uiPresManagedTreeItem::~uiPresManagedTreeItem()
{
    detachAllNotifiers();
}



void uiPresManagedTreeItem::setDataObj( SharedObject* dataobj )
{
    replaceMonitoredRef( dataobj_, dataobj, this );
    if ( dataobj )
	mAttachCB( dataobj->objectChanged(),
		    uiPresManagedTreeItem::objChangedCB );
}


void uiPresManagedTreeItem::prepareForShutdown()
{
    uiTreeItem::prepareForShutdown();
    emitPRRequest( OD::Vanish );
}


void uiPresManagedTreeItem::objChangedCB( CallBacker* cb )
{
    mGetMonitoredChgData( cb, chgdata );
    handleObjChanged( chgdata );
}


void uiPresManagedTreeItem::emitPRRequest( OD::PresentationRequestType req )
{
    PtrMan<OD::ObjPresentationInfo> objprinfo = getObjPRInfo();
    if ( !objprinfo )
	return;

    IOPar objprinfopar;
    objprinfo->fillPar( objprinfopar );
    OD::ViewerID vwrid = getViewerID();
    OD::PrMan().request( vwrid, req, objprinfopar );
}



uiPresManagedParentTreeItem::uiPresManagedParentTreeItem(
					const uiString& nm )
    : uiODTreeItem(nm)
{
}


uiPresManagedParentTreeItem::~uiPresManagedParentTreeItem()
{
    detachAllNotifiers();
}


void uiPresManagedParentTreeItem::setPRManagedViewer(
	OD::PresentationManagedViewer& prmanvwr )
{
    mAttachCB( prmanvwr.ObjAdded, uiPresManagedParentTreeItem::objAddedCB );
    mAttachCB( prmanvwr.VanishRequested,
	       uiPresManagedParentTreeItem::objVanishedCB );
    mAttachCB( prmanvwr.ShowRequested,
		uiPresManagedParentTreeItem::objShownCB );
    mAttachCB( prmanvwr.HideRequested,
		uiPresManagedParentTreeItem::objHiddenCB );
    mAttachCB( prmanvwr.ObjOrphaned,
		uiPresManagedParentTreeItem::objOrphanedCB );
}


void uiPresManagedParentTreeItem::objAddedCB( CallBacker* cber )
{
    mCBCapsuleUnpack( IOPar,objprinfopar,cber );
    BufferString objtypekey;
    objprinfopar.get( IOPar::compKey(OD::sKeyPresentationObj(),sKey::Type()),
		      objtypekey );
    if ( objtypekey != childObjTypeKey() )
	return;

    mEnsureExecutedInMainThreadWithCapsule(
	    uiPresManagedParentTreeItem::objAddedCB, cbercaps )
    OD::ObjPresentationInfo* prinfo = OD::PRIFac().create( objprinfopar );
    if ( !prinfo )
	return;

    OD::ObjPresentationInfoSet prinfos;
    prinfos.add( prinfo );
    addChildren( prinfos );
}


void uiPresManagedParentTreeItem::objVanishedCB( CallBacker* cber )
{
    mCBCapsuleUnpack( IOPar,objprinfopar,cber );
    BufferString objtypekey;
    objprinfopar.get( IOPar::compKey(OD::sKeyPresentationObj(),sKey::Type()),
		      objtypekey );
    if ( objtypekey != childObjTypeKey() )
	return;

    mEnsureExecutedInMainThreadWithCapsule(
	    uiPresManagedParentTreeItem::objVanishedCB, cbercaps )
    PtrMan<OD::ObjPresentationInfo> prinfo =
			OD::PRIFac().create( objprinfopar );
    if ( !prinfo )
	return;

    removeChildren( *prinfo );
}


void uiPresManagedParentTreeItem::objShownCB( CallBacker* cber )
{
    mCBCapsuleUnpack( IOPar,objprinfopar,cber );
    BufferString objtypekey;
    objprinfopar.get( IOPar::compKey(OD::sKeyPresentationObj(),sKey::Type()),
		      objtypekey );
    if ( objtypekey != childObjTypeKey() )
	return;

    mEnsureExecutedInMainThreadWithCapsule(
	    uiPresManagedParentTreeItem::objShownCB, cbercaps )
    PtrMan<OD::ObjPresentationInfo> prinfo =
			OD::PRIFac().create( objprinfopar );
    if ( !prinfo )
	return;

    showHideChildren( *prinfo, true );
}


void uiPresManagedParentTreeItem::objHiddenCB( CallBacker* cber )
{
    mCBCapsuleUnpack( IOPar,objprinfopar,cber );
    BufferString objtypekey;
    objprinfopar.get( IOPar::compKey(OD::sKeyPresentationObj(),sKey::Type()),
		      objtypekey );
    if ( objtypekey != childObjTypeKey() )
	return;

    mEnsureExecutedInMainThreadWithCapsule(
	    uiPresManagedParentTreeItem::objHiddenCB, cbercaps )
    PtrMan<OD::ObjPresentationInfo> prinfo =
			OD::PRIFac().create( objprinfopar );
    if ( !prinfo )
	return;

    showHideChildren( *prinfo, false );
}


void uiPresManagedParentTreeItem::objOrphanedCB( CallBacker* cber )
{
    mCBCapsuleUnpack( IOPar,objprinfopar,cber );
    BufferString objtypekey;
    objprinfopar.get( IOPar::compKey(OD::sKeyPresentationObj(),sKey::Type()),
		      objtypekey );
    if ( objtypekey != childObjTypeKey() )
	return;

    mEnsureExecutedInMainThreadWithCapsule(
	    uiPresManagedParentTreeItem::objHiddenCB, cbercaps )
    PtrMan<OD::ObjPresentationInfo> prinfo =
			OD::PRIFac().create( objprinfopar );
    if ( !prinfo )
	return;
    //TODO do something when we have clearer idea what to do when it happens
}


void uiPresManagedParentTreeItem::emitChildPRRequest(
	const OD::ObjPresentationInfo& prinfo, OD::PresentationRequestType req )
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiPresManagedTreeItem*,childitem,
			getChild(idx))
	if ( !childitem )
	    continue;

	PtrMan<OD::ObjPresentationInfo> childprinfo = childitem->getObjPRInfo();
	if ( !childprinfo->isSameObj(prinfo) )
	    continue;

	childitem->emitPRRequest( req );
    }
}


void uiPresManagedParentTreeItem::showHideChildren(
	const OD::ObjPresentationInfo& prinfo, bool show )
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiPresManagedTreeItem*,childitem,getChild(idx))
	if ( !childitem )
	    continue;

	PtrMan<OD::ObjPresentationInfo> childprinfo = childitem->getObjPRInfo();
	if ( !childprinfo || !childprinfo->isSameObj(prinfo) )
	    continue;

	childitem->setChecked( show, false );
	childitem->handleItemCheck( false );
    }
}


void uiPresManagedParentTreeItem::removeChildren(
	const OD::ObjPresentationInfo& prinfo )
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiPresManagedTreeItem*,childitem,getChild(idx))
	if ( !childitem )
	    continue;

	PtrMan<OD::ObjPresentationInfo> childprinfo = childitem->getObjPRInfo();
	if ( !childprinfo->isSameObj(prinfo) )
	    continue;

	removeChild( childitem );
    }
}


void uiPresManagedParentTreeItem::getLoadedChildren(
	OD::ObjPresentationInfoSet& prinfos ) const
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(const uiPresManagedTreeItem*,childitem,getChild(idx))
	if ( !childitem )
	    continue;

	prinfos.add( childitem->getObjPRInfo() );
    }
}


bool uiPresManagedParentTreeItem::selectChild(
	const OD::ObjPresentationInfo& prinfo )
{
    OD::ObjPresentationInfoSet objsloaded;
    getLoadedChildren( objsloaded );
    if ( !objsloaded.isPresent(prinfo) )
	return false;

    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiPresManagedTreeItem*,childitem,getChild(idx))
	if ( !childitem )
	    continue;

	PtrMan<OD::ObjPresentationInfo> childprinfo = childitem->getObjPRInfo();
	if ( childprinfo->isSameObj(prinfo) )
	{
	    childitem->select();
	    return true;
	}
    }

    return false;
}


void uiPresManagedParentTreeItem::addChildren(
    const OD::ObjPresentationInfoSet& prinfos )
{
    OD::ObjPresentationInfoSet prinfostobeloaded, prinfosloaded;
    getLoadedChildren( prinfosloaded );
    for ( int idx=0; idx<prinfos.size(); idx++ )
    {
	if ( !prinfosloaded.isPresent(*prinfos.get(idx)) )
	    prinfostobeloaded.add( prinfos.get(idx)->clone() );
    }

    for ( int idx=0; idx<prinfostobeloaded.size(); idx++ )
	addChildItem( *prinfostobeloaded.get(idx) );
}
