/*+
___________________________________________________________________

 CopyRight: 	(C) dGB Beheer B.V.
 Author: 	K. Tingdahl
 Date: 		Jul 2003
 RCS:		$Id: uiodpicksettreeitem.cc,v 1.27 2007-08-13 04:29:50 cvsraman Exp $
___________________________________________________________________

-*/

#include "uiodpicksettreeitem.h"

#include "uimenu.h"
#include "uiodapplmgr.h"
#include "uivispartserv.h"

#include "pickset.h"
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


bool uiODPickSetParentTreeItem::showSubMenu()
{
    mDynamicCastGet(visSurvey::Scene*,scene,
	    	    applMgr()->visServer()->getObject(sceneID()));
    const bool hastransform = scene && scene->getDataTransform();

    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("Load ..."), 0 );
    mnu.insertItem( new uiMenuItem("New ..."), 1 );
    if ( children_.size()>0 )
    {
	mnu.insertItem( new uiMenuItem("Save changes"), 2 );
	mnu.insertSeparator();
	uiMenuItem* filteritem =
	    new uiMenuItem( "Display picks only at sections" );
	mnu.insertItem( filteritem, 3 );
	filteritem->setEnabled( !hastransform );
	uiMenuItem* shwallitem = new uiMenuItem( "Show all picks" );
	mnu.insertItem( shwallitem, 4 );
	shwallitem->setEnabled( !hastransform );
	mnu.insertSeparator();
	mnu.insertItem( new uiMenuItem("Merge Sets"), 5 );
    }

    addStandardItems( mnu );

    const int mnuid = mnu.exec();
    if ( mnuid<0 )
	return false;
    else if ( mnuid==0 )
    {
	display_on_add = true;
	bool res = applMgr()->pickServer()->loadSets();
	display_on_add = false;
	if ( !res )
	    return -1;
    }    
    if ( mnuid==1 )
    {
	if ( !applMgr()->pickServer()->createSet() )
	    return -1;
    }
    else if ( mnuid==2 )
    {
	if ( !applMgr()->pickServer()->storeSets() )
	    uiMSG().error( "Problem saving changes. Check write protection." );
    }
    else if ( mnuid==3 || mnuid==4 )
    {
	const bool showall = mnuid == 4;
	for ( int idx=0; idx<children_.size(); idx++ )
	{
	    mDynamicCastGet(uiODPickSetTreeItem*,itm,children_[idx])
	    if ( !itm ) continue;

	    itm->showAllPicks( showall );
	    itm->updateColumnText( uiODSceneMgr::cColorColumn() );
	}
    }
    else if ( mnuid==5 )
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
    , storemnuitem_("Store")
    , storeasmnuitem_("Store As ...")
    , dirmnuitem_("Set directions ...")
    , onlyatsectmnuitem_("Display only at sections")
    , propertymnuitem_("Properties ...")
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
	    	    visserv->getObject(displayid_));
    if ( psd ) psd->setName( ps->name() );
    updateColumnText( uiODSceneMgr::cNameColumn() );
}


bool uiODPickSetTreeItem::init()
{
    if ( displayid_ == -1 )
    {
	visSurvey::PickSetDisplay* psd = visSurvey::PickSetDisplay::create();
	displayid_ = psd->id();
	visserv->addObject( psd, sceneID(), true );
	psd->setSet( &set_ );
    }
    else
    {
	mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
			visserv->getObject(displayid_));
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

    mAddMenuItem( menu, &storemnuitem_, true, false );
    mAddMenuItem( menu, &storeasmnuitem_, true, false );
//    mAddMenuItem( menu, &dirmnuitem_, true, false );

    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
		    visserv->getObject(displayid_));

    mDynamicCastGet(visSurvey::Scene*,scene,
	    	    applMgr()->visServer()->getObject(sceneID()));
    const bool hastransform = scene && scene->getDataTransform();

    mAddMenuItem( menu, &onlyatsectmnuitem_, true, !psd->allShown() );
    mAddMenuItem( menu, &propertymnuitem_, true, false );
}


void uiODPickSetTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet( uiMenuHandler*, menu, caller );
    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
	                            visserv->getObject(displayid_));

    if ( menu->menuID()!=displayID() || mnuid==-1 || menu->isHandled() )
	return;

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
	uiPickPropDlg dlg( getUiParent(), set_, psd->lineShown() );
	if ( dlg.go() )
	    psd->showLine( dlg.toDraw() );
    }

    updateColumnText( uiODSceneMgr::cNameColumn() );
    updateColumnText( uiODSceneMgr::cColorColumn() );
}


void uiODPickSetTreeItem::showAllPicks( bool yn )
{
    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
		    visserv->getObject(displayid_));
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
