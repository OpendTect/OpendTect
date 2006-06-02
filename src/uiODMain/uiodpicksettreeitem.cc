/*+
___________________________________________________________________

 CopyRight: 	(C) dGB Beheer B.V.
 Author: 	K. Tingdahl
 Date: 		Jul 2003
 RCS:		$Id: uiodpicksettreeitem.cc,v 1.7 2006-06-02 10:16:30 cvsbert Exp $
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


uiODPickSetParentTreeItem::uiODPickSetParentTreeItem()
    : uiODTreeItem( "PickSet" )
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
    for ( int idx=0; idx<Pick::Mgr().size(); idx++ )
    {
	uiODDisplayTreeItem* item =
			new uiODPickSetTreeItem( -1, Pick::Mgr().get(idx) );
	addChild( item, true );
	item->setChecked( false );
    }
    return true;
}


void uiODPickSetParentTreeItem::removeChild( uiTreeItem* child )
{
    mDynamicCastGet(uiODPickSetTreeItem*,itm,child)
    const int setidx = Pick::Mgr().indexOf( itm->getSet() );
    uiTreeItem::removeChild( child );
    if ( setidx < 0 ) return;

    Pick::Mgr().set( Pick::Mgr().id(setidx), 0 );
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
	    	    ODMainWin()->applMgr().visServer()->getObject(sceneID()));
    if ( scene && scene->getDataTransform() )
    {
	uiMSG().message( "Cannot add PickSets to this scene" );
	return false;
    }

    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("New/Load ..."), 0 );
    if ( children_.size()>0 )
    {
	mnu.insertItem( new uiMenuItem("Save changes"), 1);
	mnu.insertSeparator();
	mnu.insertItem( new uiMenuItem("Display picks only at sections"), 2 );
	mnu.insertItem( new uiMenuItem("Show all picks"), 3 );
    }

    addStandardItems( mnu );

    const int mnuid = mnu.exec();
    if ( mnuid<0 )
	return false;
    if ( mnuid==0 )
    {
	display_on_add = true;
	bool res = applMgr()->pickServer()->fetchSets();
	display_on_add = false;
	if ( !res )
	    return -1;
    }
    else if ( mnuid==1 )
    {
	if ( !applMgr()->pickServer()->storeSets() )
	    uiMSG().error( "Problem saving changes. Check write protection." );
    }
    else if ( mnuid==2 || mnuid==3 )
    {
	const bool showall = mnuid == 3;
	for ( int idx=0; idx<children_.size(); idx++ )
	{
	    mDynamicCastGet(uiODPickSetTreeItem*,itm,children_[idx])
	    if ( !itm ) continue;

	    itm->showAllPicks( showall );
	    itm->updateColumnText( uiODSceneMgr::cColorColumn() );
	}
    }
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
    , showallmnuitem_("Show all")
    , propertymnuitem_("Properties ...")
{
    displayid_ = did;
    Pick::Mgr().setChanged.notify( mCB(this,uiODPickSetTreeItem,setChg) );
}


uiODPickSetTreeItem::~uiODPickSetTreeItem()
{
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
    mAddMenuItem( menu, &dirmnuitem_, true, false );

    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
		    visserv->getObject(displayid_));

    mAddMenuItem( menu, &showallmnuitem_, true, psd->allShown() );
    mAddMenuItem( menu, &propertymnuitem_, true, false );
}


void uiODPickSetTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet( uiMenuHandler*, menu, caller );
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
    else if ( mnuid==showallmnuitem_.id )
    {
	mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
			visserv->getObject(displayid_));
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
		    visserv->getObject(displayid_));
    psd->showAll( yn );
    mDynamicCastGet(visSurvey::Scene*,scene,visserv->getObject(sceneID()));
    scene->objectMoved(0);
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
