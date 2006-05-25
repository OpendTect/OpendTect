/*+
___________________________________________________________________

 CopyRight: 	(C) dGB Beheer B.V.
 Author: 	K. Tingdahl
 Date: 		Jul 2003
 RCS:		$Id: uiodpicksettreeitem.cc,v 1.4 2006-05-25 13:35:43 cvskris Exp $
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
#include "uipickszdlg.h"
#include "vispicksetdisplay.h"
#include "vissurvscene.h"


uiODPickSetParentTreeItem::uiODPickSetParentTreeItem()
    : uiODTreeItem( "PickSet" )
{}


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
    if ( mnuid<0 ) return false;
    if ( mnuid==0 )
    {
	if ( !applMgr()->pickServer()->fetchSets() )
	    return -1;

	ObjectSet<Pick::Set>& pss = applMgr()->pickServer()->setsFetched();
	for ( int idx=0; idx<pss.size(); idx++ )
	{
	    if ( !findChild(pss[idx]->name()) )
		addChild( new uiODPickSetTreeItem(*pss[idx]), false );
	}
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
    mDynamicCastGet( visSurvey::PickSetDisplay*, psd, 
		     ODMainWin()->applMgr().visServer()->getObject(visid));
    return psd ? new uiODPickSetTreeItem(visid) : 0;
}


uiODPickSetTreeItem::uiODPickSetTreeItem( const Pick::Set& ps )
    : ps_(new Pick::Set(ps))
    , renamemnuitem_("Rename ...")
    , storemnuitem_("Store ...")
    , dirmnuitem_("Set directions ...")
    , showallmnuitem_("Show all")
    , propertymnuitem_("Properties ...")
{}


uiODPickSetTreeItem::uiODPickSetTreeItem( int id )
    : ps_(0)
    , renamemnuitem_("Rename ...")
    , storemnuitem_("Store ...")
    , dirmnuitem_("Set directions ...")
    , showallmnuitem_("Show all")
    , propertymnuitem_("Properties ...")
{ displayid_ = id; }


uiODPickSetTreeItem::~uiODPickSetTreeItem()
{ 
    delete ps_; 
}


bool uiODPickSetTreeItem::init()
{
    if ( displayid_==-1 )
    {
	visSurvey::PickSetDisplay* psd = visSurvey::PickSetDisplay::create();
	displayid_ = psd->id();
	psd->copyFromPickSet( *ps_ );
	visserv->addObject(psd,sceneID(),true);
    }
    else
    {
	mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
			visserv->getObject(displayid_));
	if ( !psd ) return false;
    }

    return uiODDisplayTreeItem::init();
}


void uiODPickSetTreeItem::createMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::createMenuCB(cb);
    mDynamicCastGet( uiMenuHandler*, menu, cb );
    if ( menu->menuID()!=displayID() )
	return;

    mAddMenuItem( menu, &renamemnuitem_, true, false );
    mAddMenuItem( menu, &storemnuitem_, true, false );
    mAddMenuItem( menu, &dirmnuitem_, true, false );

    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
		    visserv->getObject(displayid_));

    mAddMenuItem( menu, &showallmnuitem_, true, psd->allShown() );
    mAddMenuItem( menu, &propertymnuitem_, true, false );
}


void uiODPickSetTreeItem::saveCurSet( visSurvey::PickSetDisplay* psd )
{
    psd->copyToPickSet( applMgr()->pickServer()->set() );
    applMgr()->pickServer()->storeSet();
    psd->setChanged( false );
}


void uiODPickSetTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet( uiMenuHandler*, menu, caller );
    if ( menu->menuID()!=displayID() || mnuid==-1 || menu->isHandled() )
	return;

    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
		    visserv->getObject(displayid_));
    if ( mnuid==renamemnuitem_.id )
    {
	menu->setIsHandled(true);
	BufferString newname;
	const char* oldname = visserv->getObjectName( displayid_ );
	applMgr()->pickServer()->renameSet( oldname, newname );
	visserv->setObjectName( displayid_, newname );
    }
    else if ( mnuid==storemnuitem_.id )
    {
	menu->setIsHandled( true );
	saveCurSet( psd );
    }
    else if ( mnuid==dirmnuitem_.id )
    {
	menu->setIsHandled( true );
	applMgr()->setPickSetDirs( displayid_ );
    }
    else if ( mnuid==showallmnuitem_.id )
    {
	showAllPicks( !psd->allShown() );
    }
    else if ( mnuid==propertymnuitem_.id )
    {
	uiPickSizeDlg dlg( getUiParent(), psd );
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
    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
	    	    visserv->getObject(displayid_));
    if ( !psd->hasChanged() ) return true;

    BufferString warnstr = "This pickset has changed since the last save.\n"
			   "Do you want to save it?";
    const int retval = uiMSG().notSaved( warnstr.buf() );
    if ( retval == 0 )
	return true;
    else if ( retval == -1 )
	return false;
    else
	saveCurSet( psd );

    return true;
}
