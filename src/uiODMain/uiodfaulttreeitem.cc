/*+
___________________________________________________________________

 CopyRight: 	(C) dGB Beheer B.V.
 Author: 	K. Tingdahl
 Date: 		Jul 2003
 RCS:		$Id: uiodfaulttreeitem.cc,v 1.1 2008-02-05 22:14:24 cvskris Exp $
___________________________________________________________________

-*/

#include "uiodfaulttreeitem.h"

//#include "arrayndimpl.h"
//#include "marchingcubes.h"
#include "uimpepartserv.h"
#include "visfaultdisplay.h"
#include "emfault.h"
#include "emmanager.h"

#include "uicursor.h"
#include "uiempartserv.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uivispartserv.h"
//#include "vismarchingcubessurfacedisplay.h"


uiODFaultParentTreeItem::uiODFaultParentTreeItem()
   : uiODTreeItem( "Fault" )
{}

#define mLoadMnuID	0
#define mNewMnuID	1


bool uiODFaultParentTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("Load ..."), mLoadMnuID );
    mnu.insertItem( new uiMenuItem("New ..."), mNewMnuID );
    addStandardItems( mnu );

    const int mnuid = mnu.exec();
    if ( mnuid==mLoadMnuID )
    {
	TypeSet<EM::ObjectID> emids;
	applMgr()->EMServer()->selectFaults( emids );
	uiCursorChanger uics( uiCursor::Wait );
	for ( int idx=0; idx<emids.size(); idx++ )
	{
	    if ( emids[idx]<0 ) continue;
	    addChild( new uiODFaultTreeItem(emids[idx]), false );
	}
    }
    else if ( mnuid == mNewMnuID )
    {
	RefMan<EM::EMObject> emo =
	    EM::EMM().createTempObject( EM::Fault::typeStr() );
	if ( !emo )
	    return false;

	addChild( new uiODFaultTreeItem( emo->id() ), false );
	uiMPEPartServer* mps = applMgr()->mpeServer();
	mps->addTracker( emo->id(), Coord3::udf() );
	uiVisPartServer* visserv = applMgr()->visServer();
	visserv->showMPEToolbar();
	visserv->turnSeedPickingOn( true );
	return true;
    }
    else
	handleStandardItems( mnuid );

    return true;
}


uiTreeItem* uiODFaultTreeItemFactory::create( int visid,
						      uiTreeItem* ) const
{
    mDynamicCastGet(visSurvey::FaultDisplay*,fd,
	    ODMainWin()->applMgr().visServer()->getObject(visid));
    return fd ? new uiODFaultTreeItem( visid, true ) : 0;
}


uiODFaultTreeItem::uiODFaultTreeItem( const EM::ObjectID& oid )
    : uiODDisplayTreeItem()
    , emid_( oid )
    , savemnuitem_("Save")
    , saveasmnuitem_("Save as ...")
{}


uiODFaultTreeItem::uiODFaultTreeItem( int id, bool dummy )
    : uiODDisplayTreeItem()
    , emid_( -1 )
    , savemnuitem_("Save")
    , saveasmnuitem_("Save as ...")
    , faultdisplay_( 0 )
{ displayid_=id; }


uiODFaultTreeItem::~uiODFaultTreeItem()
{
    if ( faultdisplay_ )
    {
	faultdisplay_->unRef();
	faultdisplay_->materialChange()->remove(
	    mCB(this,uiODFaultTreeItem,colorChCB));
    }
}


bool uiODFaultTreeItem::init()
{
    if ( displayid_==-1 )
    {
	visSurvey::FaultDisplay* fd = visSurvey::FaultDisplay::create();
	displayid_ = fd->id();
	faultdisplay_ = fd;
	faultdisplay_->ref();

	visserv_->addObject( fd, sceneID(), true );
	fd->setEMID(emid_);
    }
    else
    {
	mDynamicCastGet( visSurvey::FaultDisplay*, fd,
			 visserv_->getObject(displayid_) );
	if ( !fd )
	    return false;

	faultdisplay_ = fd;
	faultdisplay_->ref();
	emid_ = fd->getEMID();
    }

    faultdisplay_->materialChange()->notify(
	    mCB(this,uiODFaultTreeItem,colorChCB));

    return uiODDisplayTreeItem::init();
}


void uiODFaultTreeItem::colorChCB( CallBacker* )
{
    updateColumnText( uiODSceneMgr::cColorColumn() );
}


void uiODFaultTreeItem::prepareForShutdown()
{
    applMgr()->EMServer()->askUserToSave(emid_);
    if ( faultdisplay_ )
    {
	faultdisplay_->unRef();
	faultdisplay_->materialChange()->remove(
	    mCB(this,uiODFaultTreeItem,colorChCB));
    }

    faultdisplay_ = 0;
}


void uiODFaultTreeItem::createMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::createMenuCB(cb);
    mDynamicCastGet(uiMenuHandler*,menu,cb);

    if ( menu->menuID()!=displayID() )
	return;

    mDynamicCastGet(visSurvey::FaultDisplay*,fd,
	    ODMainWin()->applMgr().visServer()->getObject(displayID()));
    if ( !fd )
	return;

    mAddMenuItem( menu, &savemnuitem_,
		  applMgr()->EMServer()->isChanged(emid_) &&
		  applMgr()->EMServer()->isFullyLoaded(emid_) &&
		  !applMgr()->EMServer()->isShifted(emid_), false );
    mAddMenuItem( menu, &saveasmnuitem_, true, false );
}


void uiODFaultTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(uiMenuHandler*,menu,caller);
    if ( menu->menuID()!=displayID() || mnuid==-1 || menu->isHandled() )
	return;

    if ( mnuid==saveasmnuitem_.id )
    {
	menu->setIsHandled(true);
	applMgr()->EMServer()->storeObject(emid_,true);

	if ( faultdisplay_ && !applMgr()->EMServer()->getName(emid_).isEmpty() )
	{
	    faultdisplay_->setName( applMgr()->EMServer()->getName(emid_));
	    updateColumnText( uiODSceneMgr::cNameColumn() );
	}
    }
    else if ( mnuid==savemnuitem_.id )
    {
	menu->setIsHandled(true);
	applMgr()->EMServer()->storeObject(emid_,false);
    }
}
