/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author: 	K. Tingdahl
 Date: 		Jul 2003
___________________________________________________________________

-*/
static const char* rcsID = "$Id: uiodtreeitem.cc,v 1.217 2011/12/16 09:27:36 cvskris Exp $";

#include "uioddisplaytreeitem.h"
#include "uiodscenetreeitem.h"

#include "uimenu.h"
#include "uiodapplmgr.h"
#include "uivispartserv.h"

#include "settings.h"
#include "uilistview.h"
#include "uimsg.h"
#include "uiodscenemgr.h"
#include "ui3dviewer.h"
#include "uiscenepropdlg.h"
#include "vissurvscene.h"


const char* uiODTreeTop::sceneidkey()		{ return "Sceneid"; }
const char* uiODTreeTop::viewerptr()		{ return "Viewer"; }
const char* uiODTreeTop::applmgrstr()		{ return "Applmgr"; }
const char* uiODTreeTop::scenestr()		{ return "Scene"; }


uiODTreeTop::uiODTreeTop( ui3DViewer* sovwr, uiListView* lv, uiODApplMgr* am,
			    uiTreeFactorySet* tfs_ )
    : uiTreeTopItem(lv)
    , tfs(tfs_)
{
    setProperty<int>( sceneidkey(), sovwr->sceneID() );
    setPropertyPtr( viewerptr(), sovwr );
    setPropertyPtr( applmgrstr(), am );

    tfs->addnotifier.notify( mCB(this,uiODTreeTop,addFactoryCB) );
    tfs->removenotifier.notify( mCB(this,uiODTreeTop,removeFactoryCB) );
}


uiODTreeTop::~uiODTreeTop()
{
    tfs->addnotifier.remove( mCB(this,uiODTreeTop,addFactoryCB) );
    tfs->removenotifier.remove( mCB(this,uiODTreeTop,removeFactoryCB) );
}


int uiODTreeTop::sceneID() const
{
    int sceneid=-1;
    getProperty<int>( sceneidkey(), sceneid );
    return sceneid;
}


bool uiODTreeTop::selectWithKey( int selkey )
{
    applMgr()->visServer()->setSelObjectId(selkey);
    return true;
}


uiODApplMgr* uiODTreeTop::applMgr()
{
    void* res = 0;
    getPropertyPtr( applmgrstr(), res );
    return reinterpret_cast<uiODApplMgr*>( res );
}


TypeSet<int> uiODTreeTop::getDisplayIds( int& selectedid, bool usechecked )
{
    TypeSet<int> dispids;
    loopOverChildrenIds( dispids, selectedid, usechecked, children_ );
    return dispids;
}


void uiODTreeTop::loopOverChildrenIds( TypeSet<int>& dispids, int& selectedid,
				       bool usechecked,
				    const ObjectSet<uiTreeItem>& childrenlist )
{
    for ( int idx=0; idx<childrenlist.size(); idx++ )
	loopOverChildrenIds( dispids, selectedid,
			     usechecked, childrenlist[idx]->children_ );

    for ( int idy=0; idy<childrenlist.size(); idy++ )
    {
	mDynamicCastGet(const uiODDisplayTreeItem*,disptreeitem,
			childrenlist[idy]);
	if ( disptreeitem )
	{
	    if ( usechecked && childrenlist[idy]->isChecked() )
		dispids += disptreeitem->displayID();
	    else if ( !usechecked )
		dispids += disptreeitem->displayID();

	    if ( childrenlist[idy]->uilistviewitem_->isSelected() )
		selectedid = disptreeitem->displayID();
	}
    }
}


uiODTreeItem::uiODTreeItem( const char* name__ )
    : uiTreeItem( name__ )
{}

bool uiODTreeItem::anyButtonClick( uiListViewItem* item )
{
    if ( item!=uilistviewitem_ )
	return uiTreeItem::anyButtonClick( item );

    if ( !select() ) return false;

    applMgr()->updateColorTable( -1, -1 );
    return true;
}


uiODApplMgr* uiODTreeItem::applMgr()
{
    void* res = 0;
    getPropertyPtr( uiODTreeTop::applmgrstr(), res );
    return reinterpret_cast<uiODApplMgr*>( res );
}


ui3DViewer* uiODTreeItem::viewer()
{
    void* res = 0;
    getPropertyPtr( uiODTreeTop::viewerptr(), res );
    return reinterpret_cast<ui3DViewer*>( res );
}


int uiODTreeItem::sceneID() const
{
    int sceneid=-1;
    getProperty<int>( uiODTreeTop::sceneidkey(), sceneid );
    return sceneid;
}


void uiODTreeItem::addStandardItems( uiPopupMenu& mnu )
{
    if ( children_.size() < 2 ) return;

    mnu.insertSeparator( 100 );
    mnu.insertItem( new uiMenuItem("Show all items"), 101 );
    mnu.insertItem( new uiMenuItem("Hide all items"), 102 );
    mnu.insertItem( new uiMenuItem("Remove all items"), 103 );
}


void uiODTreeItem::handleStandardItems( int mnuid )
{
    for ( int idx=0; idx<children_.size(); idx++ )
    {
	if ( mnuid == 101 )
	    children_[idx]->setChecked( true, true );
	else if ( mnuid == 102 )
	    children_[idx]->setChecked( false, true );
    }

    if ( mnuid==103 )
    {
	const BufferString msg( "All ", name(),
	    " items will be removed from the tree.\nDo you want to continue?");
	if ( !uiMSG().askRemove(msg) ) return;

	while ( children_.size() )
	{
	    mDynamicCastGet(uiODDisplayTreeItem*,itm,children_[0])
	    if ( !itm ) continue;
	    itm->prepareForShutdown();
	    applMgr()->visServer()->removeObject( itm->displayID(), sceneID() );
	    removeChild( itm );
	}
    }
}


void uiODTreeTop::addFactoryCB( CallBacker* cb )
{
    mCBCapsuleUnpack(int,factidx,cb);
    const int newplaceidx = tfs->getPlacementIdx( factidx );
    uiTreeItem* itmbefore = 0;
    int maxidx = -1;
    for ( int idx=0; idx<tfs->nrFactories(); idx++ )
    {
	const int curidx = tfs->getPlacementIdx( idx );
	if ( curidx>newplaceidx || curidx<maxidx || curidx==newplaceidx )
	    continue;

	maxidx = curidx;
    }
    for ( int idx=0; idx<tfs->nrFactories(); idx++ )
    {
	if ( tfs->getPlacementIdx(idx) != maxidx )
	    continue;

	PtrMan<uiTreeItem> itm = tfs->getFactory(idx)->create();
	itmbefore = findChild( itm->name() );
	break;
    }

    uiTreeItem* newitm = tfs->getFactory(factidx)->create();
    addChild( newitm, false );
    if ( itmbefore )
	newitm->moveItem( itmbefore );
}


void uiODTreeTop::removeFactoryCB( CallBacker* cb )
{
    mCBCapsuleUnpack(int,idx,cb);
    PtrMan<uiTreeItem> dummy = tfs->getFactory(idx)->create();
    const uiTreeItem* child = findChild( dummy->name() );
    if ( children_.indexOf(child)==-1 )
	return;

    removeChild( const_cast<uiTreeItem*>(child) );
}


uiODSceneTreeItem::uiODSceneTreeItem( const char* nm, int id )
	: uiODTreeItem( nm )
	, displayid_( id )
{
}


void uiODSceneTreeItem::updateColumnText( int col )
{
    if ( col==uiODSceneMgr::cNameColumn() )
	name_ = applMgr()->visServer()->getObjectName( displayid_ );

    uiTreeItem::updateColumnText( col );
}

#define mProperties	0
#define mTopBotImg	1
#define mDumpIV		2
#define mScnColBar	3


bool uiODSceneTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );

    uiMenuItem* anntxt = new uiMenuItem( "&Properties ..." );
    mnu.insertItem( anntxt, mProperties );
    mnu.insertItem( new uiMenuItem("&Top/Bottom image ..."), mTopBotImg );
    mnu.insertItem( new uiMenuItem("&Scene Color Bar Properties ..."), mScnColBar );

    bool yn = false;
    Settings::common().getYN( IOPar::compKey("dTect","Dump OI Menu"), yn );
    if ( yn )
	mnu.insertItem( new uiMenuItem("&Export scene ..."), mDumpIV );

    uiVisPartServer* visserv = applMgr()->visServer();
    const int mnuid=mnu.exec();
    if ( mnuid==mProperties )
    {
	ObjectSet<ui3DViewer> viewers;
	ODMainWin()->sceneMgr().getSoViewers( viewers );

	uiScenePropertyDlg dlg( getUiParent(), viewers, 
			viewers.indexOf( viewer() ) );
	dlg.go();
    }
    else if ( mnuid==mTopBotImg )
	visserv->setTopBotImg( displayid_ );
    else if ( mnuid== mScnColBar )
	visserv->manageSceneColorbar( displayid_ );
    else if( mnuid==mDumpIV )
	visserv->writeSceneToFile( displayid_, "Export scene as ..." );

    return true;
}
