/*+
___________________________________________________________________

 CopyRight: 	(C) dGB Beheer B.V.
 Author: 	K. Tingdahl
 Date: 		Jul 2003
 RCS:		$Id: uiodfaulttreeitem.cc,v 1.16 2008-10-07 10:21:24 cvsnanne Exp $
___________________________________________________________________

-*/

#include "uiodfaulttreeitem.h"

#include "uimpepartserv.h"
#include "visfaultdisplay.h"
#include "visfault2ddisplay.h"
#include "emfault2d.h"
#include "emfault3d.h"
#include "emmanager.h"
#include "ioman.h"
#include "ioobj.h"

#include "mousecursor.h"
#include "randcolor.h"
#include "selector.h"
#include "uiempartserv.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uimsg.h"
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
	applMgr()->EMServer()->selectFaults( emids, false );
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
	    EM::EMM().createTempObject( EM::Fault3D::typeStr() );
	if ( !emo )
	    return false;

	emo->setPreferredColor( getRandomColor(false) );
	BufferString newname = "<New fault ";
	static int faultnr = 1;
	newname += faultnr++;
	newname += ">";
	emo->setName( newname.buf() );
	emo->setFullyLoaded( true );
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
    , displaymnuitem_( "Display ..." ) \
    , displayplanemnuitem_ ( "Fault planes" ) \
    , displaystickmnuitem_ ( "Fault sticks" ) \
    , displayintersectionmnuitem_( "At sections only" ) \
    , singlecolmnuitem_( "Use single &color" ) \
    , removeselectedmnuitem_( "&Remove selection" )



uiODFaultTreeItem::uiODFaultTreeItem( const EM::ObjectID& oid )
    : uiODDisplayTreeItem()
    , emid_( oid )
    mCommonInit
{
    displayplanemnuitem_.checkable = true;
    displaystickmnuitem_.checkable = true;
    displayintersectionmnuitem_.checkable = true;
    singlecolmnuitem_.checkable = true;
}


uiODFaultTreeItem::uiODFaultTreeItem( int id, bool dummy )
    : uiODDisplayTreeItem()
    , emid_(-1)
    , faultdisplay_(0)
    mCommonInit
{
    displayplanemnuitem_.checkable = true;
    displaystickmnuitem_.checkable = true;
    displayintersectionmnuitem_.checkable = true;
    singlecolmnuitem_.checkable = true;
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

	fd->setEMID( emid_ );
	visserv_->addObject( fd, sceneID(), true );
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

    mAddMenuItem( menu, &singlecolmnuitem_, faultdisplay_->arePanelsDisplayed(),
		  !faultdisplay_->usesTexture() );
    mAddMenuItem( &displaymnuitem_, &displayplanemnuitem_, true,
		  faultdisplay_->arePanelsDisplayed() );
    mAddMenuItem( &displaymnuitem_, &displaystickmnuitem_, true,
		  !faultdisplay_->arePanelsDisplayed() &&
		   faultdisplay_->areSticksDisplayed() );
    mAddMenuItem( &displaymnuitem_, &displayintersectionmnuitem_, true,
		  faultdisplay_->areIntersectionsDisplayed() );
    mAddMenuItem( menu, &displaymnuitem_, true, true );

    const Selector<Coord3>* sel = visserv_->getCoordSelector( sceneID() );
    mAddMenuItem( menu, &removeselectedmnuitem_, sel, true );

    const bool enablesave = applMgr()->EMServer()->isChanged(emid_) &&
			    applMgr()->EMServer()->isFullyLoaded(emid_);

    mAddMenuItem( menu, &savemnuitem_, enablesave, false );
    mAddMenuItem( menu, &saveasmnuitem_, true, false );
}


void uiODFaultTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(uiMenuHandler*,menu,caller);
    if ( menu->isHandled() || menu->menuID()!=displayID() || mnuid==-1 )
	return;

    if ( mnuid==saveasmnuitem_.id ||  mnuid==savemnuitem_.id )
    {
	menu->setIsHandled(true);
	bool saveas = mnuid==saveasmnuitem_.id ||
		      applMgr()->EMServer()->getStorageID(emid_).isEmpty();
	if ( !saveas )
	{
	    PtrMan<IOObj> ioobj =
		IOM().get( applMgr()->EMServer()->getStorageID(emid_) );
	    saveas = !ioobj;
	}

	applMgr()->EMServer()->storeObject( emid_, saveas );

	if ( saveas && faultdisplay_ &&
	     !applMgr()->EMServer()->getName(emid_).isEmpty() )
	{
	    faultdisplay_->setName( applMgr()->EMServer()->getName(emid_));
	    updateColumnText( uiODSceneMgr::cNameColumn() );
	}
    }
    else if ( mnuid==displayplanemnuitem_.id )
    {
	menu->setIsHandled(true);
	faultdisplay_->display( false, true );
	faultdisplay_->displayIntersections( false );
    }
    else if ( mnuid==displaystickmnuitem_.id )
    {
	menu->setIsHandled(true);
	faultdisplay_->display( true, false );
	faultdisplay_->displayIntersections( false );
    }
    else if ( mnuid==displayintersectionmnuitem_.id )
    {
	menu->setIsHandled(true);
	faultdisplay_->display( false, false );
	faultdisplay_->displayIntersections( true );
    }
    else if ( mnuid==singlecolmnuitem_.id )
    {
	menu->setIsHandled(true);
	faultdisplay_->useTexture( !faultdisplay_->usesTexture(), true );
	visserv_->triggerTreeUpdate();
    }
    else if ( mnuid==removeselectedmnuitem_.id )
    {
	menu->setIsHandled(true);
	const Selector<Coord3>* sel = visserv_->getCoordSelector( sceneID() );
	if ( sel->isOK() )
	    faultdisplay_->removeSelection( *sel );
	else
	    uiMSG().error( "Invalid selection : self-intersecting polygon" );
    }
}



uiODFault2DParentTreeItem::uiODFault2DParentTreeItem()
   : uiODTreeItem( "2D Fault" )
{}


bool uiODFault2DParentTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("Load ..."), mLoadMnuID );
    mnu.insertItem( new uiMenuItem("New ..."), mNewMnuID );
    addStandardItems( mnu );

    const int mnuid = mnu.exec();
    if ( mnuid==mLoadMnuID )
    {
	TypeSet<EM::ObjectID> emids;
	applMgr()->EMServer()->selectFaults( emids, true );
	MouseCursorChanger uics( MouseCursor::Wait );
	for ( int idx=0; idx<emids.size(); idx++ )
	{
	    if ( emids[idx]<0 ) continue;
	    addChild( new uiODFault2DTreeItem(emids[idx]), false );
	}
    }
    else if ( mnuid == mNewMnuID )
    {
	RefMan<EM::EMObject> emo =
	    EM::EMM().createTempObject( EM::Fault2D::typeStr() );
	if ( !emo )
	    return false;

	emo->setPreferredColor( getRandomColor(false) );
	BufferString newname = "<New fault ";
	static int faultnr = 1;
	newname += faultnr++;
	newname += ">";
	emo->setName( newname.buf() );
	emo->setFullyLoaded( true );
	addChild( new uiODFault2DTreeItem( emo->id() ), false );

	uiVisPartServer* visserv = applMgr()->visServer();
	visserv->showMPEToolbar();
	visserv->turnSeedPickingOn( true );
	return true;
    }
    else
	handleStandardItems( mnuid );

    return true;
}


uiTreeItem* uiODFault2DTreeItemFactory::create( int visid, uiTreeItem* ) const
{
    mDynamicCastGet(visSurvey::Fault2DDisplay*,fd,
	    ODMainWin()->applMgr().visServer()->getObject(visid));
    return fd ? new uiODFault2DTreeItem( visid, true ) : 0;
}


#undef mCommonInit
#define mCommonInit \
    , fault2ddisplay_(0) \
    , savemnuitem_("Save") \
    , saveasmnuitem_("Save as ...") \
    , removeselectedmnuitem_( "&Remove selection" )



uiODFault2DTreeItem::uiODFault2DTreeItem( const EM::ObjectID& oid )
    : uiODDisplayTreeItem()
    , emid_( oid )
    mCommonInit
{
}


uiODFault2DTreeItem::uiODFault2DTreeItem( int id, bool dummy )
    : uiODDisplayTreeItem()
    , emid_(-1)
    mCommonInit
{
    displayid_ = id;
}


uiODFault2DTreeItem::~uiODFault2DTreeItem()
{
    if ( fault2ddisplay_ )
    {
	fault2ddisplay_->unRef();
	fault2ddisplay_->materialChange()->remove(
	    mCB(this,uiODFault2DTreeItem,colorChCB) );
    }
}


bool uiODFault2DTreeItem::init()
{
    if ( displayid_==-1 )
    {
	visSurvey::Fault2DDisplay* fd = visSurvey::Fault2DDisplay::create();
	displayid_ = fd->id();
	fault2ddisplay_ = fd;
	fault2ddisplay_->ref();

	fd->setEMID( emid_ );
	visserv_->addObject( fd, sceneID(), true );
    }
    else
    {
	mDynamicCastGet(visSurvey::Fault2DDisplay*,fd,
			visserv_->getObject(displayid_));
	if ( !fd )
	    return false;

	fault2ddisplay_ = fd;
	fault2ddisplay_->ref();
	emid_ = fd->getEMID();
    }

    fault2ddisplay_->materialChange()->notify(
	    mCB(this,uiODFault2DTreeItem,colorChCB) );

    return uiODDisplayTreeItem::init();
}


void uiODFault2DTreeItem::colorChCB( CallBacker* )
{
    updateColumnText( uiODSceneMgr::cColorColumn() );
}


void uiODFault2DTreeItem::prepareForShutdown()
{
    applMgr()->EMServer()->askUserToSave(emid_);
    if ( fault2ddisplay_ )
    {
	fault2ddisplay_->unRef();
	fault2ddisplay_->materialChange()->remove(
	    mCB(this,uiODFault2DTreeItem,colorChCB) );
    }

    fault2ddisplay_ = 0;
}


void uiODFault2DTreeItem::createMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::createMenuCB(cb);
    mDynamicCastGet(uiMenuHandler*,menu,cb);
    if ( menu->menuID()!=displayID() )
	return;

    mDynamicCastGet(visSurvey::Fault2DDisplay*,fd,
	    ODMainWin()->applMgr().visServer()->getObject(displayID()));
    if ( !fd )
	return;

    const Selector<Coord3>* sel = visserv_->getCoordSelector( sceneID() );
    mAddMenuItem( menu, &removeselectedmnuitem_, sel, true );

    const bool enablesave = applMgr()->EMServer()->isChanged(emid_) &&
			    applMgr()->EMServer()->isFullyLoaded(emid_);
    mAddMenuItem( menu, &savemnuitem_, enablesave, false );
    mAddMenuItem( menu, &saveasmnuitem_, true, false );
}


void uiODFault2DTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(uiMenuHandler*,menu,caller);
    if ( menu->isHandled() || menu->menuID()!=displayID() || mnuid==-1 )
	return;

    if ( mnuid==saveasmnuitem_.id ||  mnuid==savemnuitem_.id )
    {
	menu->setIsHandled(true);
	bool saveas = mnuid==saveasmnuitem_.id ||
		      applMgr()->EMServer()->getStorageID(emid_).isEmpty();
	if ( !saveas )
	{
	    PtrMan<IOObj> ioobj =
		IOM().get( applMgr()->EMServer()->getStorageID(emid_) );
	    saveas = !ioobj;
	}

	applMgr()->EMServer()->storeObject( emid_, saveas );

	if ( saveas && fault2ddisplay_ &&
	     !applMgr()->EMServer()->getName(emid_).isEmpty() )
	{
	    fault2ddisplay_->setName( applMgr()->EMServer()->getName(emid_));
	    updateColumnText( uiODSceneMgr::cNameColumn() );
	}
    }
    else if ( mnuid==removeselectedmnuitem_.id )
    {
	menu->setIsHandled(true);
	const Selector<Coord3>* sel = visserv_->getCoordSelector( sceneID() );
	if ( sel->isOK() )
	    fault2ddisplay_->removeSelection( *sel );
	else
	    uiMSG().error( "Invalid selection : self-intersecting polygon" );
    }
}
