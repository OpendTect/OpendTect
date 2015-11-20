/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Apr 2010
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiodvw2dtreeitem.h"
#include "uiodviewer2d.h"
#include "uiodviewer2dmgr.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uitreeview.h"
#include "zaxistransform.h"

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
{}


uiODVw2DTreeItem::~uiODVw2DTreeItem()
{
    detachAllNotifiers();
    if ( datatransform_ )
	datatransform_->unRef();
}


bool uiODVw2DTreeItem::init()
{
    const char* iconnm = iconName();
    if ( iconnm ) uitreeviewitem_->setIcon( 0, iconnm );

    return uiTreeItem::init();
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



