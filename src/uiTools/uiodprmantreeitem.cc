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


uiODPrManagedTreeItem::uiODPrManagedTreeItem( const uiString& nm )
    : uiODTreeItem(nm)
    , dataobj_(0)
{
}


uiODPrManagedTreeItem::~uiODPrManagedTreeItem()
{
    detachAllNotifiers();
}



void uiODPrManagedTreeItem::setDataObj( SharedObject* dataobj )
{
    if ( dataobj_ )
	mDetachCB( dataobj_->objectChanged(),
		   uiODPrManagedTreeItem::objChangedCB );

    dataobj_ = dataobj;
    mAttachCB( dataobj_->objectChanged(), uiODPrManagedTreeItem::objChangedCB );
}


void uiODPrManagedTreeItem::prepareForShutdown()
{
    uiTreeItem::prepareForShutdown();
    emitPRRequest( OD::Vanish );
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
    objprinfopar.get( IOPar::compKey(OD::sKeyPresentationObj(),sKey::Type()),
		      objtypekey );
    if ( objtypekey != childObjTypeKey() )
	return;

    mEnsureExecutedInMainThreadWithCapsule(
	    uiODPrManagedParentTreeItem::objAddedCB, cbercaps )
    OD::ObjPresentationInfo* prinfo = OD::PRIFac().create( objprinfopar );
    if ( !prinfo )
	return;

    OD::ObjPresentationInfoSet prinfos;
    prinfos.add( prinfo );
    addChildren( prinfos );
}


void uiODPrManagedParentTreeItem::objVanishedCB( CallBacker* cber )
{
    mCBCapsuleUnpack( IOPar,objprinfopar,cber );
    BufferString objtypekey;
    objprinfopar.get( IOPar::compKey(OD::sKeyPresentationObj(),sKey::Type()),
		      objtypekey );
    if ( objtypekey != childObjTypeKey() )
	return;

    mEnsureExecutedInMainThreadWithCapsule(
	    uiODPrManagedParentTreeItem::objVanishedCB, cbercaps )
    PtrMan<OD::ObjPresentationInfo> prinfo =
			OD::PRIFac().create( objprinfopar );
    if ( !prinfo )
	return;

    removeChildren( *prinfo );
}


void uiODPrManagedParentTreeItem::objShownCB( CallBacker* cber )
{
    mCBCapsuleUnpack( IOPar,objprinfopar,cber );
    BufferString objtypekey;
    objprinfopar.get( IOPar::compKey(OD::sKeyPresentationObj(),sKey::Type()),
		      objtypekey );
    if ( objtypekey != childObjTypeKey() )
	return;

    mEnsureExecutedInMainThreadWithCapsule(
	    uiODPrManagedParentTreeItem::objShownCB, cbercaps )
    PtrMan<OD::ObjPresentationInfo> prinfo =
			OD::PRIFac().create( objprinfopar );
    if ( !prinfo )
	return;

    showHideChildren( *prinfo, true );
}


void uiODPrManagedParentTreeItem::objHiddenCB( CallBacker* cber )
{
    mCBCapsuleUnpack( IOPar,objprinfopar,cber );
    BufferString objtypekey;
    objprinfopar.get( IOPar::compKey(OD::sKeyPresentationObj(),sKey::Type()),
		      objtypekey );
    if ( objtypekey != childObjTypeKey() )
	return;

    mEnsureExecutedInMainThreadWithCapsule(
	    uiODPrManagedParentTreeItem::objHiddenCB, cbercaps )
    PtrMan<OD::ObjPresentationInfo> prinfo =
			OD::PRIFac().create( objprinfopar );
    if ( !prinfo )
	return;

    showHideChildren( *prinfo, false );
}


void uiODPrManagedParentTreeItem::objOrphanedCB( CallBacker* cber )
{
    mCBCapsuleUnpack( IOPar,objprinfopar,cber );
    BufferString objtypekey;
    objprinfopar.get( IOPar::compKey(OD::sKeyPresentationObj(),sKey::Type()),
		      objtypekey );
    if ( objtypekey != childObjTypeKey() )
	return;

    mEnsureExecutedInMainThreadWithCapsule(
	    uiODPrManagedParentTreeItem::objHiddenCB, cbercaps )
    PtrMan<OD::ObjPresentationInfo> prinfo =
			OD::PRIFac().create( objprinfopar );
    if ( !prinfo )
	return;
    //TODO do something when we have clearer idea what to do when it happens
}


void uiODPrManagedParentTreeItem::emitChildPRRequest(
	const OD::ObjPresentationInfo& prinfo, OD::PresentationRequestType req )
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODPrManagedTreeItem*,childitem,
			getChild(idx))
	if ( !childitem )
	    continue;

	PtrMan<OD::ObjPresentationInfo> childprinfo = childitem->getObjPRInfo();
	if ( !childprinfo->isSameObj(prinfo) )
	    continue;

	childitem->emitPRRequest( req );
    }
}


void uiODPrManagedParentTreeItem::showHideChildren(
	const OD::ObjPresentationInfo& prinfo, bool show )
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODPrManagedTreeItem*,childitem,getChild(idx))
	if ( !childitem )
	    continue;

	PtrMan<OD::ObjPresentationInfo> childprinfo = childitem->getObjPRInfo();
	if ( !childprinfo || !childprinfo->isSameObj(prinfo) )
	    continue;

	childitem->setChecked( show, false );
	childitem->handleItemCheck( false );
    }
}


void uiODPrManagedParentTreeItem::removeChildren(
	const OD::ObjPresentationInfo& prinfo )
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODPrManagedTreeItem*,childitem,getChild(idx))
	if ( !childitem )
	    continue;

	PtrMan<OD::ObjPresentationInfo> childprinfo = childitem->getObjPRInfo();
	if ( !childprinfo->isSameObj(prinfo) )
	    continue;

	removeChild( childitem );
    }
}


void uiODPrManagedParentTreeItem::getLoadedChildren(
	OD::ObjPresentationInfoSet& prinfos ) const
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(const uiODPrManagedTreeItem*,childitem,getChild(idx))
	if ( !childitem )
	    continue;

	prinfos.add( childitem->getObjPRInfo() );
    }
}


bool uiODPrManagedParentTreeItem::selectChild(
	const OD::ObjPresentationInfo& prinfo )
{
    OD::ObjPresentationInfoSet objsloaded;
    getLoadedChildren( objsloaded );
    if ( !objsloaded.isPresent(prinfo) )
	return false;

    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODPrManagedTreeItem*,childitem,getChild(idx))
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


void uiODPrManagedParentTreeItem::addChildren(
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
