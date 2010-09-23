/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author: 	K. Tingdahl
 Date: 		Jul 2003
___________________________________________________________________

-*/
static const char* rcsID = "$Id: uiodbodydisplaytreeitem.cc,v 1.27 2010-09-23 04:46:25 cvsnanne Exp $";

#include "uiodbodydisplaytreeitem.h"

#include "arrayndimpl.h"
#include "marchingcubes.h"
#include "vismarchingcubessurface.h"
#include "empolygonbody.h"
#include "emmarchingcubessurface.h"
#include "emmanager.h"
#include "emrandomposbody.h"

#include "ioman.h"
#include "ioobj.h"
#include "mousecursor.h"
#include "randcolor.h"
#include "selector.h"
#include "uibodyoperatordlg.h"
#include "uiempartserv.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uivispartserv.h"
#include "vismarchingcubessurfacedisplay.h"
#include "visrandomposbodydisplay.h"
#include "vispolygonbodydisplay.h"
#include "uitaskrunner.h"


uiODBodyDisplayParentTreeItem::uiODBodyDisplayParentTreeItem()
   : uiODTreeItem( "Body" )
{}


bool uiODBodyDisplayParentTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("&Load ..."), 0 );
    mnu.insertItem( new uiMenuItem("&New polygon body..."), 1 );
    //mnu.insertItem( new uiMenuItem("&New body combination..."), 2 );
    addStandardItems( mnu );

    const int mnuid = mnu.exec();
    if ( mnuid==0 )
    {
	ObjectSet<EM::EMObject> objs;
	applMgr()->EMServer()->selectBodies( objs );
	MouseCursorChanger uics( MouseCursor::Wait );
	for ( int idx=0; idx<objs.size(); idx++ )
	    addChild( new uiODBodyDisplayTreeItem(objs[idx]->id()), false );
	deepUnRef( objs );
    }
    else if ( mnuid == 1 )
    {
	RefMan<EM::EMObject> plg =
	    EM::EMM().createTempObject( EM::PolygonBody::typeStr() );
	if ( !plg )
	    return false;

	plg->setPreferredColor( getRandomColor(false) );
	plg->setNewName();
	plg->setFullyLoaded( true );
	addChild( new uiODBodyDisplayTreeItem( plg->id() ), false );
	
	uiVisPartServer* visserv = applMgr()->visServer();
	visserv->showMPEToolbar();
	visserv->turnSeedPickingOn( false );
    }
    else if ( mnuid == 2 )
    {
	static bool ask = false;
	static bool confirmed = true;
	if ( !ask )
	{
	    confirmed = 
		uiMSG().askContinue( "The body operation is still under testing.\n"
		     "It may not be stable or may take long time to process.\n"
		     "Do you want to continue?");
	    ask = true;
	}

	if ( !confirmed ) return false;

	RefMan<EM::EMObject> emobj = EM::EMM().createTempObject( 
		EM::MarchingCubesSurface::typeStr() );
	if ( !emobj ) return false;
	
	mDynamicCastGet( EM::MarchingCubesSurface*, mcs, emobj.ptr() );
	if ( !mcs ) return false;

	mcs->setPreferredColor( getRandomColor(false) );
	
	BufferString nm = "<New body "; 
	static int surfnr = 1;
	nm += surfnr++; 
	nm += ">";
	mcs->setName( nm.buf() );
	mcs->setFullyLoaded( true );

	if ( !mcs->getBodyOperator() )
	    mcs->createBodyOperator();
	
	uiBodyOperatorDlg dlg( getUiParent(), *mcs->getBodyOperator() ); 
	const int res = dlg.go();

	MouseCursorChanger bodyopration( MouseCursor::Wait );
	uiTaskRunner taskrunner( getUiParent() );
	if ( !res || !mcs->regenerateMCBody( &taskrunner ) )
	    return false;

	addChild( new uiODBodyDisplayTreeItem( mcs->id() ), false );
    }
    else
	handleStandardItems( mnuid );

    return true;
}


uiTreeItem* uiODBodyDisplayTreeItemFactory::create( int visid,
						    uiTreeItem* ) const
{
    mDynamicCastGet(visSurvey::PolygonBodyDisplay*,plg,
	    ODMainWin()->applMgr().visServer()->getObject(visid));
    if ( plg )
	return new uiODBodyDisplayTreeItem( visid, true );
    
    mDynamicCastGet(visSurvey::MarchingCubesDisplay*,mcd,
	    ODMainWin()->applMgr().visServer()->getObject(visid));
    if ( mcd )
    	return new uiODBodyDisplayTreeItem( visid, true );

    return 0;
}


uiODBodyDisplayTreeItem::uiODBodyDisplayTreeItem( const EM::ObjectID& oid )
    : uiODDisplayTreeItem()
    , emid_( oid )
    , savemnuitem_("&Save")
    , saveasmnuitem_("Save &as ...")
    , displaymnuitem_( "&Display ..." )
    , displaybodymnuitem_ ( "&Body" )
    , displaypolygonmnuitem_( "&Picked polygons" )			    
    , displayintersectionmnuitem_( "&Intersections" )
    , removeselectedmnuitem_( "&Remove selection" )
    , singlecolormnuitem_( "Use Single &color" )
    , mcd_( 0 )
    , plg_( 0 )
    , rpb_( 0 ) 	       
{
    displaybodymnuitem_.checkable = true;
    displaypolygonmnuitem_.checkable = true;
    displayintersectionmnuitem_.checkable = true;
    singlecolormnuitem_.checkable = true;
}


uiODBodyDisplayTreeItem::uiODBodyDisplayTreeItem( int id, bool dummy )
    : uiODDisplayTreeItem()
    , emid_( -1 )
    , savemnuitem_("Save")
    , saveasmnuitem_("Save as ...")
    , displaymnuitem_( "Display ..." )				   
    , displaybodymnuitem_ ( "Body" )
    , displaypolygonmnuitem_( "Picked polygons" )			    
    , displayintersectionmnuitem_( "Intersections" )
    , removeselectedmnuitem_( "&Remove selection" )
    , singlecolormnuitem_( "Use Single &color" )
    , mcd_( 0 )
    , plg_( 0 )	       
    , rpb_( 0 ) 	       
{
    displayid_ = id;
    displaybodymnuitem_.checkable = true;
    displaypolygonmnuitem_.checkable = true;
    displayintersectionmnuitem_.checkable = true;
    singlecolormnuitem_.checkable = true;
}


uiODBodyDisplayTreeItem::~uiODBodyDisplayTreeItem()
{
    if ( mcd_ )
    {
	mcd_->materialChange()->remove(
	    mCB(this,uiODBodyDisplayTreeItem,colorChCB) );
	mcd_->unRef();
    }

    if ( plg_ )
    {
	plg_->materialChange()->remove(
	    mCB(this,uiODBodyDisplayTreeItem,colorChCB) );
	plg_->unRef();
    }

    if ( rpb_ ) 
    {
	rpb_->materialChange()->remove(
		mCB(this,uiODBodyDisplayTreeItem,colorChCB) );
	rpb_->unRef();
    }
}


bool uiODBodyDisplayTreeItem::init()
{
    if ( displayid_==-1 )
    {
	EM::EMObject* object = EM::EMM().getObject( emid_ );
	mDynamicCastGet( EM::PolygonBody*, plg0, object );
	mDynamicCastGet( EM::MarchingCubesSurface*, mcd0, object );
	mDynamicCastGet( EM::RandomPosBody*, rpb0, object );
	if ( plg0 )
	{
	    visSurvey::PolygonBodyDisplay* plg =
		visSurvey::PolygonBodyDisplay::create();
	    displayid_ = plg->id();
	    plg_ = plg;
	    plg_->ref();
	    
	    plg->setEMID(emid_);
	    visserv_->addObject( plg, sceneID(), true );
	}
	else if ( mcd0 ) 
	{
	    visSurvey::MarchingCubesDisplay* mcd =
		visSurvey::MarchingCubesDisplay::create();
	    displayid_ = mcd->id();
	    mcd_ = mcd;
	    mcd_->ref();
	    mcd_->setEMID( emid_ );
	    visserv_->addObject( mcd, sceneID(), true );
	}
	else if ( rpb0 )
	{
	    visSurvey::RandomPosBodyDisplay* rpb = 
		visSurvey::RandomPosBodyDisplay::create();
	    displayid_ = rpb->id();
	    rpb_ = rpb;
	    rpb_->ref();
	    rpb_->setEMID( emid_ );
	    visserv_->addObject( rpb, sceneID(), true );
	}
    }
    else
    {
	mDynamicCastGet( visSurvey::PolygonBodyDisplay*, plg,
			 visserv_->getObject(displayid_) );
	mDynamicCastGet( visSurvey::MarchingCubesDisplay*, mcd,
			 visserv_->getObject(displayid_) );
	mDynamicCastGet( visSurvey::RandomPosBodyDisplay*, rpb, 
			 visserv_->getObject(displayid_) );
	if ( plg )
	{
	    plg_ = plg;
	    plg_->ref();
	    emid_ = plg->getEMID();
	}
	else if ( mcd )
	{
	    mcd_ = mcd;
	    mcd_->ref();
	    emid_ = mcd->getEMID();
	}
	else if ( rpb )
	{
	    rpb_ = rpb;
	    rpb_->ref();
	    emid_ = rpb->getEMID();
	}	
    }

    if ( plg_ )
    {
	plg_->materialChange()->notify(
		mCB(this,uiODBodyDisplayTreeItem,colorChCB) );
    }
    
    if ( mcd_ )
    {
	mcd_->materialChange()->notify(
		mCB(this,uiODBodyDisplayTreeItem,colorChCB) );
    }

    if ( rpb_ )
    {
	rpb_->materialChange()->notify(
		mCB(this,uiODBodyDisplayTreeItem,colorChCB) );
    }
    
    return uiODDisplayTreeItem::init();
}


void uiODBodyDisplayTreeItem::colorChCB( CallBacker* )
{
    updateColumnText( uiODSceneMgr::cColorColumn() );
}


bool uiODBodyDisplayTreeItem::askContinueAndSaveIfNeeded( bool withcancel )
{
    return applMgr()->EMServer()->askUserToSave( emObjectID(), withcancel );
}


void uiODBodyDisplayTreeItem::prepareForShutdown()
{
    if ( mcd_ )
    {
	mcd_->materialChange()->remove(
	    mCB(this,uiODBodyDisplayTreeItem,colorChCB) );
	mcd_->unRef();
    }

    mcd_ = 0;

    if ( plg_ )
    {
	plg_->materialChange()->remove(
		mCB(this,uiODBodyDisplayTreeItem,colorChCB) );
	plg_->unRef();
    }

    plg_ = 0;

    if ( rpb_ ) 
    {
	rpb_->materialChange()->remove( 
		mCB(this,uiODBodyDisplayTreeItem,colorChCB) );
	rpb_->unRef();
    }

    rpb_ = 0;
}


void uiODBodyDisplayTreeItem::createMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::createMenuCB(cb);
    mDynamicCastGet(MenuHandler*,menu,cb);
    if ( menu->menuID()!=displayID() )
	return;

    mDynamicCastGet(visSurvey::MarchingCubesDisplay*,mcd,
	    ODMainWin()->applMgr().visServer()->getObject(displayID()));
    mDynamicCastGet(visSurvey::PolygonBodyDisplay*,plg,
	    ODMainWin()->applMgr().visServer()->getObject(displayID()));
    mDynamicCastGet(visSurvey::RandomPosBodyDisplay*,rpb,
	    ODMainWin()->applMgr().visServer()->getObject(displayID()));
	
    const bool enablesave = applMgr()->EMServer()->isChanged(emid_) &&
			    applMgr()->EMServer()->isFullyLoaded(emid_);
    if ( mcd )
    {
	mAddMenuItem( menu, &savemnuitem_, enablesave, false );
	mAddMenuItem( menu, &saveasmnuitem_, true, false );
	mAddMenuItem( menu, &singlecolormnuitem_, true, !mcd->usesTexture() );
    }

    if ( plg )
    {
	mAddMenuItem( &displaymnuitem_, &displaybodymnuitem_, true,
		      plg_->isBodyDisplayed() );
	mAddMenuItem( &displaymnuitem_, &displaypolygonmnuitem_, true,
		      plg_->arePolygonsDisplayed() );
	mAddMenuItem( &displaymnuitem_, &displayintersectionmnuitem_, true,
		      plg_->areIntersectionsDisplayed() );
	mAddMenuItem( menu, &displaymnuitem_, true, true );
	
	const Selector<Coord3>* sel = visserv_->getCoordSelector( sceneID() );
	mAddMenuItem( menu, &removeselectedmnuitem_, sel && sel->isOK(), true );
	
	mAddMenuItem( menu, &savemnuitem_, enablesave, false );
	mAddMenuItem( menu, &saveasmnuitem_, true, false );
    }

    if ( rpb )
    {
	mAddMenuItem( menu, &savemnuitem_, enablesave, false );
	mAddMenuItem( menu, &saveasmnuitem_, true, false );
    }
}


void uiODBodyDisplayTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( menu->isHandled() || menu->menuID()!=displayID() || mnuid==-1 )
	return;

    if ( mnuid==saveasmnuitem_.id || mnuid==savemnuitem_.id )
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
	const bool notempty = !applMgr()->EMServer()->getName(emid_).isEmpty();
	if ( saveas && notempty )
	{
	    if ( plg_ )
		plg_->setName( applMgr()->EMServer()->getName(emid_) );

	    if ( rpb_ )
		rpb_->setName( applMgr()->EMServer()->getName(emid_) );

	    if ( mcd_ )
		mcd_->setName( applMgr()->EMServer()->getName(emid_) );

	    updateColumnText( uiODSceneMgr::cNameColumn() );
	}
    }
    else if ( mnuid==displaybodymnuitem_.id )
    {
	menu->setIsHandled(true);
	plg_->display( true, true );
	plg_->displayIntersections( false );
    }
    if ( mnuid==singlecolormnuitem_.id )
    {
	menu->setIsHandled(true);
	mcd_->useTexture( !mcd_->usesTexture() );
    }
    else if ( mnuid==displaypolygonmnuitem_.id )
    {
	menu->setIsHandled(true);
	plg_->display( true, false );
	plg_->displayIntersections( false );
    }
    else if ( mnuid==displayintersectionmnuitem_.id )
    {
	menu->setIsHandled(true);
	plg_->display( false, false );
	plg_->displayIntersections( true );
    }
    else if ( mnuid==removeselectedmnuitem_.id )
    {
	menu->setIsHandled(true);
	plg_->removeSelection( *visserv_->getCoordSelector(sceneID()), 0 );
    }
}



