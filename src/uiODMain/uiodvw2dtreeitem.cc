/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Apr 2010
 RCS:		$Id: uiodvw2dtreeitem.cc,v 1.5 2011/06/28 13:35:43 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uiodvw2dtreeitem.h"
#include "uiodviewer2d.h"
#include "uiodviewer2dmgr.h"

const char* uiODVw2DTreeTop::viewer2dptr() 		{ return "Viewer2D"; }
const char* uiODVw2DTreeTop::applmgrstr()		{ return "Applmgr"; }


uiODVw2DTreeTop::uiODVw2DTreeTop( uiListView* lv, uiODApplMgr* am,
				  uiODViewer2D* vw2d, uiTreeFactorySet* tfs )
    : uiTreeTopItem( lv, true )
    , tfs_( tfs )
{
    setPropertyPtr( applmgrstr(), am );
    setPropertyPtr( viewer2dptr(), vw2d );

    tfs_->addnotifier.notify( mCB(this,uiODVw2DTreeTop,addFactoryCB) );
    tfs_->removenotifier.notify( mCB(this,uiODVw2DTreeTop,addFactoryCB) );
}


uiODVw2DTreeTop::~uiODVw2DTreeTop()
{
    tfs_->addnotifier.remove( mCB(this,uiODVw2DTreeTop,addFactoryCB) );
    tfs_->removenotifier.remove( mCB(this,uiODVw2DTreeTop,addFactoryCB) );
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


void uiODVw2DTreeTop::updCubeSamling( const CubeSampling& cs, bool update )
{
        for ( int idx=0; idx<nrChildren(); idx++ )
	{
	    mDynamicCastGet(uiODVw2DTreeItem*,itm,getChild(idx));
	    itm->updCubeSamling( cs, update );
	}
}


void uiODVw2DTreeTop::updSelSpec( const Attrib::SelSpec* selspec, bool wva )
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DTreeItem*,itm,getChild(idx));
	itm->updSelSpec( selspec, wva );
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
	itmbefore = findChild( itm->name() );
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
    const uiTreeItem* child = findChild( dummy->name() );
    if ( children_.indexOf(child)==-1 )
	return;

    removeChild( const_cast<uiTreeItem*>(child) );
}


uiODVw2DTreeItem::uiODVw2DTreeItem( const char* name__ )
    : uiTreeItem( name__ )
    , displayid_(-1)  
{}


void uiODVw2DTreeItem::updCubeSamling( const CubeSampling& cs, bool update )
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DTreeItem*,itm,getChild(idx));
	itm->updCubeSamling( cs, update );
    }

    updateCS( cs, update );
}


void uiODVw2DTreeItem::updSelSpec(const Attrib::SelSpec* selspec, bool wva )
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DTreeItem*,itm,getChild(idx));
	itm->updSelSpec( selspec, wva );
    }

    updateSelSpec( selspec, wva );
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
    uiODViewer2D* vwr2d = ODMainWin()->viewer2DMgr().find2DViewer( visid );
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


