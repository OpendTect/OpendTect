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
    emitPrRequest( Presentation::Vanish );
    uiTreeItem::prepareForShutdown();
}


void uiPresManagedTreeItem::objChangedCB( CallBacker* cb )
{
    mGetMonitoredChgData( cb, chgdata );
    handleObjChanged( chgdata );
}


void uiPresManagedTreeItem::emitPrRequest( ReqType req )
{
    PtrMan<PresInfo> objprinfo = getObjPrInfo();
    IOPar objprinfopar;
    if ( objprinfo )
	objprinfo->fillPar( objprinfopar );

    OD::PrMan().handleRequest( viewerID(), req, objprinfopar );
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


void uiPresManagedParentTreeItem::setPrManagedViewer(
	Presentation::ManagedViewer& prmanvwr )
{
    mAttachCB( prmanvwr.ObjAdded, uiPresManagedParentTreeItem::objAddedCB );
    mAttachCB( prmanvwr.VanishRequested,
	       uiPresManagedParentTreeItem::objVanishReqCB );
    mAttachCB( prmanvwr.ShowRequested,
		uiPresManagedParentTreeItem::objShowReqCB );
    mAttachCB( prmanvwr.HideRequested,
		uiPresManagedParentTreeItem::objHideReqCB );
    mAttachCB( prmanvwr.ObjOrphaned,
		uiPresManagedParentTreeItem::objOrphanedCB );
}


void uiPresManagedParentTreeItem::objAddedCB( CallBacker* cber )
{
    mCBCapsuleUnpack( IOPar,objprinfopar,cber );
    BufferString objtypekey;
    objprinfopar.get( IOPar::compKey(Presentation::sKeyObj(),sKey::Type()),
		      objtypekey );
    if ( objtypekey != childObjTypeKey() )
	return;

    mEnsureExecutedInMainThreadWithCapsule(
	    uiPresManagedParentTreeItem::objAddedCB, cbercaps )
    PresInfo* prinfo = OD::PrIFac().create( objprinfopar );
    if ( !prinfo )
	return;

    PresInfoSet prinfos;
    prinfos.add( prinfo );
    addChildren( prinfos );
}


void uiPresManagedParentTreeItem::objVanishReqCB( CallBacker* cber )
{
    mCBCapsuleUnpack( IOPar,objprinfopar,cber );
    BufferString objtypekey;
    objprinfopar.get( IOPar::compKey(Presentation::sKeyObj(),sKey::Type()),
		      objtypekey );
    if ( objtypekey != childObjTypeKey() )
	return;

    mEnsureExecutedInMainThreadWithCapsule(
	    uiPresManagedParentTreeItem::objVanishReqCB, cbercaps )
    PtrMan<PresInfo> prinfo = OD::PrIFac().create( objprinfopar );
    if ( !prinfo )
	return;

    removeChildren( *prinfo );
}


void uiPresManagedParentTreeItem::objShowReqCB( CallBacker* cber )
{
    mCBCapsuleUnpack( IOPar,objprinfopar,cber );
    BufferString objtypekey;
    objprinfopar.get( IOPar::compKey(Presentation::sKeyObj(),sKey::Type()),
		      objtypekey );
    if ( objtypekey != childObjTypeKey() )
	return;

    mEnsureExecutedInMainThreadWithCapsule(
	    uiPresManagedParentTreeItem::objShowReqCB, cbercaps )
    PtrMan<PresInfo> prinfo = OD::PrIFac().create( objprinfopar );
    if ( !prinfo )
	return;

    showHideChildren( *prinfo, true );
}


void uiPresManagedParentTreeItem::objHideReqCB( CallBacker* cber )
{
    mCBCapsuleUnpack( IOPar,objprinfopar,cber );
    BufferString objtypekey;
    objprinfopar.get( IOPar::compKey(Presentation::sKeyObj(),sKey::Type()),
		      objtypekey );
    if ( objtypekey != childObjTypeKey() )
	return;

    mEnsureExecutedInMainThreadWithCapsule(
	    uiPresManagedParentTreeItem::objHideReqCB, cbercaps )
    PtrMan<PresInfo> prinfo = OD::PrIFac().create( objprinfopar );
    if ( !prinfo )
	return;

    showHideChildren( *prinfo, false );
}


void uiPresManagedParentTreeItem::objOrphanedCB( CallBacker* cber )
{
    mCBCapsuleUnpack( IOPar, objprinfopar, cber );
    BufferString objtypekey;
    objprinfopar.get( IOPar::compKey(Presentation::sKeyObj(),sKey::Type()),
		      objtypekey );
    if ( objtypekey != childObjTypeKey() )
	return;

    mEnsureExecutedInMainThreadWithCapsule(
	    uiPresManagedParentTreeItem::objHideReqCB, cbercaps )
    PtrMan<PresInfo> prinfo = OD::PrIFac().create( objprinfopar );
    if ( !prinfo )
	return;

    emitChildPrRequest( *prinfo, Presentation::Vanish );
}


void uiPresManagedParentTreeItem::emitChildPrRequest( const PresInfo& prinfo,
						      ReqType req )
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiPresManagedTreeItem*,childitem,
			getChild(idx))
	if ( !childitem )
	    continue;

	PtrMan<PresInfo> childprinfo = childitem->getObjPrInfo();
	if ( !childprinfo || !childprinfo->isSameObj(prinfo) )
	    continue;

	childitem->emitPrRequest( req );
    }
}


void uiPresManagedParentTreeItem::showHideChildren( const PresInfo& prinfo,
						    bool show )
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiPresManagedTreeItem*,childitem,getChild(idx))
	if ( !childitem )
	    continue;

	PtrMan<PresInfo> childprinfo = childitem->getObjPrInfo();
	if ( !childprinfo || !childprinfo->isSameObj(prinfo) )
	    continue;

	childitem->setChecked( show, false );
	childitem->handleItemCheck( false );
    }
}


void uiPresManagedParentTreeItem::removeChildren( const PresInfo& prinfo )
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiPresManagedTreeItem*,childitem,getChild(idx))
	if ( !childitem )
	    continue;

	PtrMan<PresInfo> childprinfo = childitem->getObjPrInfo();
	if ( !childprinfo || !childprinfo->isSameObj(prinfo) )
	    continue;

	removeChild( childitem );
    }
}


void uiPresManagedParentTreeItem::getLoadedChildren(
					    PresInfoSet& prinfos ) const
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(const uiPresManagedTreeItem*,childitem,getChild(idx))
	if ( !childitem )
	    continue;

	PresInfo* prinf = childitem->getObjPrInfo();
	if ( prinf )
	    prinfos.add( prinf );
    }
}


bool uiPresManagedParentTreeItem::selectChild( const PresInfo& prinfo )
{
    PresInfoSet objsloaded;
    getLoadedChildren( objsloaded );
    if ( !objsloaded.isPresent(prinfo) )
	return false;

    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiPresManagedTreeItem*,childitem,getChild(idx))
	if ( !childitem )
	    continue;

	PtrMan<PresInfo> childprinfo = childitem->getObjPrInfo();
	if ( childprinfo && childprinfo->isSameObj(prinfo) )
	    { childitem->select(); return true; }
    }

    return false;
}


void uiPresManagedParentTreeItem::addChildren( const PresInfoSet& prinfos )
{
    PresInfoSet prinfostobeloaded, prinfosloaded;
    getLoadedChildren( prinfosloaded );
    for ( int idx=0; idx<prinfos.size(); idx++ )
    {
	if ( !prinfosloaded.isPresent(*prinfos.get(idx)) )
	    prinfostobeloaded.add( prinfos.get(idx)->clone() );
    }

    for ( int idx=0; idx<prinfostobeloaded.size(); idx++ )
	addChildItem( *prinfostobeloaded.get(idx) );
}
