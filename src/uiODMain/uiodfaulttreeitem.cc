/*+
___________________________________________________________________

 CopyRight: 	(C) dGB Beheer B.V.
 Author: 	K. Tingdahl
 Date: 		Jul 2003
 RCS:		$Id: uiodfaulttreeitem.cc,v 1.4 2008-05-09 09:12:01 cvsnanne Exp $
___________________________________________________________________

-*/

#include "uiodfaulttreeitem.h"

#include "uimpepartserv.h"
#include "visfaultdisplay.h"
#include "emfault.h"
#include "emmanager.h"

#include "mousecursor.h"
#include "uiempartserv.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uivispartserv.h"


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
	MouseCursorChanger uics( MouseCursor::Wait );
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

	uiMPEPartServer* mps = applMgr()->mpeServer();
	mps->addTracker( emo->id(), Coord3::udf() );
	emo->setName( "<Fault>" );
	addChild( new uiODFaultTreeItem( emo->id() ), false );

	uiVisPartServer* visserv = applMgr()->visServer();
	visserv->showMPEToolbar();
	visserv->turnSeedPickingOn( true );
	return true;
    }
    else
	handleStandardItems( mnuid );

    return true;
}


uiTreeItem* uiODFaultTreeItemFactory::create( int visid, uiTreeItem* ) const
{
    mDynamicCastGet(visSurvey::FaultDisplay*,fd,
	    ODMainWin()->applMgr().visServer()->getObject(visid));
    return fd ? new uiODFaultTreeItem( visid, true ) : 0;
}


#define mCommonInit \
    , savemnuitem_("Save") \
    , saveasmnuitem_("Save as ...") \
    , dispsticksonlymnuitem_("Display sticks only")

uiODFaultTreeItem::uiODFaultTreeItem( const EM::ObjectID& oid )
    : uiODDisplayTreeItem()
    , emid_( oid )
    mCommonInit
{
    dispsticksonlymnuitem_.checkable = true;
}


uiODFaultTreeItem::uiODFaultTreeItem( int id, bool dummy )
    : uiODDisplayTreeItem()
    , emid_(-1)
    , faultdisplay_(0)
    mCommonInit
{
    dispsticksonlymnuitem_.checkable = true;
    displayid_ = id;
}


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
	fd->setEMID( emid_ );
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

    mAddMenuItem( menu, &dispsticksonlymnuitem_, true,
		  !faultdisplay_->arePanelsDisplayed() );
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
    else if ( mnuid==dispsticksonlymnuitem_.id )
    {
	menu->setIsHandled(true);
	faultdisplay_->display( true, dispsticksonlymnuitem_.checked );
    }
}
