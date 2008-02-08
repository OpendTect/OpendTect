/*+
___________________________________________________________________

 CopyRight: 	(C) dGB Beheer B.V.
 Author: 	K. Tingdahl
 Date: 		Jul 2003
 RCS:		$Id: uiodpicksettreeitem.cc,v 1.37 2008-02-08 07:34:46 cvsnanne Exp $
___________________________________________________________________

-*/

#include "uiodpicksettreeitem.h"

#include "uimenu.h"
#include "uiodapplmgr.h"
#include "uivispartserv.h"

#include "pickset.h"
#include "survinfo.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uiodscenemgr.h"
#include "uipickpartserv.h"
#include "uipickpropdlg.h"
#include "vispicksetdisplay.h"
#include "vissurvscene.h"
#include "vispolylinedisplay.h"


uiODPickSetParentTreeItem::uiODPickSetParentTreeItem()
    : uiODTreeItem("PickSet")
    , display_on_add(false)
{
    Pick::Mgr().setAdded.notify( mCB(this,uiODPickSetParentTreeItem,setAdd) );
    Pick::Mgr().setToBeRemoved.notify(
	    			mCB(this,uiODPickSetParentTreeItem,setRm) );
}


uiODPickSetParentTreeItem::~uiODPickSetParentTreeItem()
{
    Pick::Mgr().removeCBs( this );
}


bool uiODPickSetParentTreeItem::init()
{
    /*
    for ( int idx=0; idx<Pick::Mgr().size(); idx++ )
    {
	uiODDisplayTreeItem* item =
			new uiODPickSetTreeItem( -1, Pick::Mgr().get(idx) );
	addChild( item, true );
	item->setChecked( false );
    }
    */
    return true;
}


void uiODPickSetParentTreeItem::removeChild( uiTreeItem* child )
{
    const int idx = children_.indexOf( child );
    if ( idx<0 ) return;

    mDynamicCastGet(uiODPickSetTreeItem*,itm,child)
    const int setidx = Pick::Mgr().indexOf( itm->getSet() );
    uiTreeItem::removeChild( child );
//    if ( setidx < 0 ) return;

//    Pick::Mgr().set( Pick::Mgr().id(setidx), 0 );
}


void uiODPickSetParentTreeItem::setAdd( CallBacker* cb )
{
    mDynamicCastGet(Pick::Set*,ps,cb)
    if ( !ps ) return;

    uiODDisplayTreeItem* item = new uiODPickSetTreeItem( -1, *ps );
    addChild( item, true );
    item->setChecked( display_on_add );
}


void uiODPickSetParentTreeItem::setRm( CallBacker* cb )
{
    mDynamicCastGet(Pick::Set*,ps,cb)

    for ( int idx=0; idx<children_.size(); idx++ )
    {
	mDynamicCastGet(uiODPickSetTreeItem*,itm,children_[idx])
	if ( !itm ) continue;
	if ( &itm->getSet() == ps )
	{
	    applMgr()->visServer()->removeObject( itm->displayID(), sceneID() );
	    uiTreeItem::removeChild( itm );
	    return;
	}
    }
}


#define mLoadIdx	0
#define mRandom3DIdx	1
#define mRandom2DIdx	2
#define mEmptyIdx	3
#define mPolygonIdx	4
#define mSaveIdx	10
#define mDisplayIdx	11
#define mShowAllIdx	12
#define mMergeIdx	13


bool uiODPickSetParentTreeItem::showSubMenu()
{
    mDynamicCastGet(visSurvey::Scene*,scene,
	    	    applMgr()->visServer()->getObject(sceneID()));
    const bool hastransform = scene && scene->getDataTransform();

    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("&Load ..."), mLoadIdx );
    uiPopupMenu* newmnu = new uiPopupMenu( getUiParent(), "&New" );
    if ( SI().has2D() )
    {
	uiPopupMenu* randmnu = new uiPopupMenu( getUiParent(), "&Random" );
	randmnu->insertItem( new uiMenuItem("3D ..."), mRandom3DIdx );
	randmnu->insertItem( new uiMenuItem("2D ..."), mRandom2DIdx );
	newmnu->insertItem( randmnu );
    }
    else
	newmnu->insertItem( new uiMenuItem("&Random ..."), mRandom3DIdx );

    newmnu->insertItem( new uiMenuItem("&Empty ..."), mEmptyIdx );
    newmnu->insertItem( new uiMenuItem("&Polygon ..."), mPolygonIdx );
    mnu.insertItem( newmnu );
    if ( children_.size() > 0 )
    {
	mnu.insertItem( new uiMenuItem("&Save changes"), mSaveIdx );
	mnu.insertSeparator();
	uiMenuItem* filteritem =
	    new uiMenuItem( "&Display picks only at sections" );
	mnu.insertItem( filteritem, mDisplayIdx );
	filteritem->setEnabled( !hastransform );
	uiMenuItem* shwallitem = new uiMenuItem( "Show &all picks" );
	mnu.insertItem( shwallitem, mShowAllIdx );
	shwallitem->setEnabled( !hastransform );
	mnu.insertSeparator();
	mnu.insertItem( new uiMenuItem("&Merge Sets"), mMergeIdx );
    }

    addStandardItems( mnu );

    const int mnuid = mnu.exec();
    if ( mnuid<0 )
	return false;
    else if ( mnuid==mLoadIdx )
    {
	display_on_add = true;
	bool res = applMgr()->pickServer()->loadSets();
	display_on_add = false;
	if ( !res )
	    return -1;
    }    
    else if ( mnuid==mRandom3DIdx || mnuid==mRandom2DIdx )
    {
	display_on_add = true;
	if ( !applMgr()->pickServer()->createRandomSet(mnuid==mRandom2DIdx) )
	    return -1;
	display_on_add = false;
    }
    else if ( mnuid==mEmptyIdx || mnuid==mPolygonIdx )
    {
	display_on_add = true;
	if ( !applMgr()->pickServer()->createEmptySet(mnuid==mPolygonIdx) )
	    return -1;
	display_on_add = false;
    }
    else if ( mnuid==mSaveIdx )
    {
	if ( !applMgr()->pickServer()->storeSets() )
	    uiMSG().error( "Problem saving changes. Check write protection." );
    }
    else if ( mnuid==mDisplayIdx || mnuid==mShowAllIdx )
    {
	const bool showall = mnuid==mShowAllIdx;
	for ( int idx=0; idx<children_.size(); idx++ )
	{
	    mDynamicCastGet(uiODPickSetTreeItem*,itm,children_[idx])
	    if ( !itm ) continue;

	    itm->showAllPicks( showall );
	    itm->updateColumnText( uiODSceneMgr::cColorColumn() );
	}
    }
    else if ( mnuid==mMergeIdx )
	{ MultiID mid; applMgr()->pickServer()->mergeSets( mid ); }
    else
	handleStandardItems( mnuid );

    return true;
}


uiTreeItem* uiODPickSetTreeItemFactory::create( int visid, uiTreeItem* ) const
{
    mDynamicCastGet(visSurvey::PickSetDisplay*,psd, 
		    ODMainWin()->applMgr().visServer()->getObject(visid));
    return !psd ? 0 : new uiODPickSetTreeItem( visid, *psd->getSet() );
}


uiODPickSetTreeItem::uiODPickSetTreeItem( int did, Pick::Set& ps )
    : set_(ps)
    , storemnuitem_("&Save")
    , storeasmnuitem_("Save &As ...")
    , dirmnuitem_("Set &directions ...")
    , onlyatsectmnuitem_("Display only at s&ections")
    , propertymnuitem_("&Properties ...")
    , closepolyitem_("&Close Polygon")
{
    displayid_ = did;
    Pick::Mgr().setChanged.notify( mCB(this,uiODPickSetTreeItem,setChg) );

    onlyatsectmnuitem_.checkable = true;
}


uiODPickSetTreeItem::~uiODPickSetTreeItem()
{
    const int setidx = Pick::Mgr().indexOf( set_ );
    if ( setidx>= 0 )
	Pick::Mgr().set( Pick::Mgr().id(setidx), 0 );
    
    Pick::Mgr().removeCBs( this );
}


void uiODPickSetTreeItem::setChg( CallBacker* cb )
{
    mDynamicCastGet(Pick::Set*,ps,cb)
    if ( !ps || &set_!=ps ) return;

    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
	    	    visserv_->getObject(displayid_));
    if ( psd ) psd->setName( ps->name() );
    updateColumnText( uiODSceneMgr::cNameColumn() );
}


bool uiODPickSetTreeItem::init()
{
    if ( displayid_ == -1 )
    {
	visSurvey::PickSetDisplay* psd = visSurvey::PickSetDisplay::create();
	displayid_ = psd->id();
	visserv_->addObject( psd, sceneID(), true );
	if ( set_.disp_.pixsize_>100 )
	    set_.disp_.pixsize_ = 3;
	psd->setSet( &set_ );
    }
    else
    {
	mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
			visserv_->getObject(displayid_));
	if ( !psd ) return false;
	const MultiID setid = psd->getMultiID();
	NotifyStopper ntfstop( Pick::Mgr().setAdded );
	Pick::Mgr().set( setid, psd->getSet() );
    }

    updateColumnText( uiODSceneMgr::cColorColumn() );
    return uiODDisplayTreeItem::init();
}


void uiODPickSetTreeItem::createMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::createMenuCB(cb);
    mDynamicCastGet( uiMenuHandler*, menu, cb );
    if ( menu->menuID()!=displayID() )
	return;

    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
		    visserv_->getObject(displayid_));

    if ( set_.disp_.connect_ == Pick::Set::Disp::Open )
	mAddMenuItem( menu, &closepolyitem_, true, false )
    else
	mResetMenuItem( &closepolyitem_ )

    mAddMenuItem( menu, &propertymnuitem_, true, false );
    mAddMenuItem( menu, &onlyatsectmnuitem_, true, !psd->allShown() );
    mAddMenuItem( menu, &storemnuitem_, true, false );
    mAddMenuItem( menu, &storeasmnuitem_, true, false );
}


void uiODPickSetTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet( uiMenuHandler*, menu, caller );
    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
	                            visserv_->getObject(displayid_));

    if ( menu->menuID()!=displayID() || mnuid==-1 || menu->isHandled() )
	return;

    if ( set_.disp_.connect_==Pick::Set::Disp::Open 
	 && mnuid==closepolyitem_.id )
    {
	menu->setIsHandled( true );
	set_.disp_.connect_ = Pick::Set::Disp::Close;
	Pick::Mgr().reportDispChange( this, set_ );
    }
    if ( mnuid==storemnuitem_.id )
    {
	menu->setIsHandled( true );
	applMgr()->storePickSet( set_ );
    }
    if ( mnuid==storeasmnuitem_.id )
    {
	menu->setIsHandled( true );
	applMgr()->storePickSetAs( set_ );
    }
    else if ( mnuid==dirmnuitem_.id )
    {
	menu->setIsHandled( true );
	applMgr()->setPickSetDirs( set_ );
    }
    else if ( mnuid==onlyatsectmnuitem_.id )
    {
	if ( psd )
	    showAllPicks( !psd->allShown() );
    }
    else if ( mnuid==propertymnuitem_.id )
    {
	uiPickPropDlg dlg( getUiParent(), set_ );
	dlg.go();
    }

    updateColumnText( uiODSceneMgr::cNameColumn() );
    updateColumnText( uiODSceneMgr::cColorColumn() );
}


void uiODPickSetTreeItem::showAllPicks( bool yn )
{
    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
		    visserv_->getObject(displayid_));
    psd->showAll( yn );
}


bool uiODPickSetTreeItem::askContinueAndSaveIfNeeded()
{
    const int setidx = Pick::Mgr().indexOf( set_ );
    if ( setidx < 0 || !Pick::Mgr().isChanged(setidx) )
	return true;

    BufferString warnstr = "This pickset has changed since the last save.\n"
			   "Do you want to save it?";
    const int retval = uiMSG().notSaved( warnstr.buf() );
    if ( retval == 0 )
	return true;
    else if ( retval == -1 )
	return false;
    else
	applMgr()->storePickSet( set_ );

    return true;
}
