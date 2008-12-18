/*+
___________________________________________________________________

 CopyRight: 	(C) dGB Beheer B.V.
 Author: 	K. Tingdahl
 Date: 		Jul 2003
___________________________________________________________________

-*/
static const char* rcsID = "$Id: uiodbodydisplaytreeitem.cc,v 1.5 2008-12-18 09:51:03 cvsbert Exp $";

#include "uiodbodydisplaytreeitem.h"

#include "arrayndimpl.h"
#include "marchingcubes.h"
#include "vismarchingcubessurface.h"
#include "empolygonbody.h"
#include "emmarchingcubessurface.h"
#include "emmanager.h"

#include "ioman.h"
#include "ioobj.h"
#include "mousecursor.h"
#include "randcolor.h"
#include "selector.h"
#include "uiempartserv.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uipolygonsurfbezierdlg.h"
#include "uivispartserv.h"
#include "vismarchingcubessurfacedisplay.h"
#include "vispolygonsurfdisplay.h"


uiODBodyDisplayParentTreeItem::uiODBodyDisplayParentTreeItem()
   : uiODTreeItem( "Body" )
{
}


bool uiODBodyDisplayParentTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("&Load ..."), 0 );
    mnu.insertItem( new uiMenuItem("&New ..."), 1 );
    addStandardItems( mnu );

    const int mnuid = mnu.exec();
    if ( mnuid==0 )
    {
	TypeSet<EM::ObjectID> emids;
	applMgr()->EMServer()->selectBodies( emids );
	MouseCursorChanger uics( MouseCursor::Wait );
	for ( int idx=0; idx<emids.size(); idx++ )
	{
	    if ( emids[idx]<0 ) continue;
	    addChild( new uiODBodyDisplayTreeItem(emids[idx]), false );
	}
    }
    else if ( mnuid == 1 )
    {
	RefMan<EM::EMObject> plg =
	    EM::EMM().createTempObject( EM::PolygonBody::typeStr() );
	if ( !plg )
	    return false;

	if ( plg )
	{
	    plg->setPreferredColor( getRandomColor(false) );
	    BufferString newname = "<New body ";
	    static int polygonsurfnr = 1;
	    newname += polygonsurfnr++;
	    newname += ">";
	    plg->setName( newname.buf() );
	    plg->setFullyLoaded( true );
	    addChild( new uiODBodyDisplayTreeItem( plg->id() ), false );
	    
	    uiVisPartServer* visserv = applMgr()->visServer();
	    visserv->showMPEToolbar();
	    visserv->turnSeedPickingOn( true );
	}
	
	return true;
    }
    else
	handleStandardItems( mnuid );

    return true;
}


uiTreeItem* uiODBodyDisplayTreeItemFactory::create( int visid,
						      uiTreeItem* ) const
{
    mDynamicCastGet(visSurvey::PolygonSurfDisplay*,plg,
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
    , beziernrmnuitem_( "Polygon smo&oth" )			      
    , displaybodymnuitem_ ( "&Body" )
    , displaypolygonmnuitem_( "&Picked polygons" )			    
    , displayintersectionmnuitem_( "&Intersections" )
    , removeselectedmnuitem_( "&Remove selection" )
    , newellipsoidmnuitem_("&Create body")				   
    , mcd_( 0 )
    , plg_( 0 )	       
{
    displaybodymnuitem_.checkable = true;
    displaypolygonmnuitem_.checkable = true;
    displayintersectionmnuitem_.checkable = true;
}


uiODBodyDisplayTreeItem::uiODBodyDisplayTreeItem( int id, bool dummy )
    : uiODDisplayTreeItem()
    , emid_( -1 )
    , savemnuitem_("Save")
    , saveasmnuitem_("Save as ...")
    , displaymnuitem_( "Display ..." )				   
    , beziernrmnuitem_( "Polygon smooth" )			      
    , displaybodymnuitem_ ( "Body" )
    , displaypolygonmnuitem_( "Picked polygons" )			    
    , displayintersectionmnuitem_( "Intersections" )
    , removeselectedmnuitem_( "&Remove selection" )
    , newellipsoidmnuitem_("Make body")				   
    , mcd_( 0 )
    , plg_( 0 )	       
{
    displayid_ = id;
    displaybodymnuitem_.checkable = true;
    displaypolygonmnuitem_.checkable = true;
    displayintersectionmnuitem_.checkable = true;
}


uiODBodyDisplayTreeItem::~uiODBodyDisplayTreeItem()
{
    if ( mcd_ )
    {
	mcd_->unRef();
	mcd_->materialChange()->remove(
	    mCB(this,uiODBodyDisplayTreeItem,colorChCB));
    }

    if ( plg_ )
    {
	plg_->unRef();
	plg_->materialChange()->remove(
	    mCB(this,uiODBodyDisplayTreeItem,colorChCB));
    }
}


bool uiODBodyDisplayTreeItem::init()
{
    if ( displayid_==-1 )
    {
	EM::EMObject* object = EM::EMM().getObject( emid_ );
	mDynamicCastGet( EM::PolygonBody*, plg0, object );
	mDynamicCastGet( EM::MarchingCubesSurface*, mcd0, object );
	if ( plg0 )
	{
	    visSurvey::PolygonSurfDisplay* plg =
		visSurvey::PolygonSurfDisplay::create();
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
	    mcd_ =mcd;
	    mcd_->ref();
	    mcd_->setEMID( emid_ );
	    visserv_->addObject( mcd, sceneID(), true );
	}
    }
    else
    {
	mDynamicCastGet( visSurvey::PolygonSurfDisplay*, plg,
			 visserv_->getObject(displayid_) );
	if ( plg )
	{
	    plg_ = plg;
	    plg_->ref();
	    emid_ = plg->getEMID();
	}
	else
	{
	    mDynamicCastGet( visSurvey::MarchingCubesDisplay*, mcd,
		    visserv_->getObject(displayid_) );
	    if ( mcd )
	    {
		mcd_ =mcd;
		mcd_->ref();
		emid_ = mcd->getEMID();
    
		if ( mcd_ && mcd_->materialChange() )
	    	    mcd_->materialChange()->notify(
	    		    mCB(this,uiODBodyDisplayTreeItem,colorChCB));
	    }
	}
    }

    if ( plg_ )
    {
	plg_->materialChange()->notify(
		mCB(this,uiODBodyDisplayTreeItem,colorChCB) );
    }
    
    return uiODDisplayTreeItem::init();
}


void uiODBodyDisplayTreeItem::colorChCB( CallBacker* )
{
    updateColumnText( uiODSceneMgr::cColorColumn() );
}


void uiODBodyDisplayTreeItem::prepareForShutdown()
{
    applMgr()->EMServer()->askUserToSave(emid_);
    if ( mcd_ )
    {
	mcd_->unRef();
	mcd_->materialChange()->remove(
	    mCB(this,uiODBodyDisplayTreeItem,colorChCB));
    }

    mcd_ = 0;

    if ( plg_ )
    {
	if ( plg_->materialChange() )
    	    plg_->materialChange()->remove(
		    mCB(this,uiODBodyDisplayTreeItem,colorChCB));
	plg_->unRef();
    }

    plg_ = 0;
}


void uiODBodyDisplayTreeItem::createMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::createMenuCB(cb);
    mDynamicCastGet(uiMenuHandler*,menu,cb);

    if ( menu->menuID()!=displayID() )
	return;

    mDynamicCastGet(visSurvey::MarchingCubesDisplay*,mcd,
	    ODMainWin()->applMgr().visServer()->getObject(displayID()));
    mDynamicCastGet(visSurvey::PolygonSurfDisplay*,plg,
	    ODMainWin()->applMgr().visServer()->getObject(displayID()));
    if ( mcd )
    {
	if ( mcd->hasInitialShape() )
	{
	    mAddMenuItem( menu, &newellipsoidmnuitem_, true, false );
	    mResetMenuItem( &savemnuitem_ );
	    mResetMenuItem( &saveasmnuitem_ );
	}
	else
	{
	    mAddMenuItem( menu, &savemnuitem_,
			  applMgr()->EMServer()->isChanged(emid_) &&
			  applMgr()->EMServer()->isFullyLoaded(emid_) &&
			  !applMgr()->EMServer()->isShifted(emid_), false );
	    mAddMenuItem( menu, &saveasmnuitem_, true, false );
	    mResetMenuItem( &newellipsoidmnuitem_ );
	}
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
	mAddMenuItem( menu, &beziernrmnuitem_, true, true );
	
	const Selector<Coord3>* sel = visserv_->getCoordSelector( sceneID() );
	mAddMenuItem( menu, &removeselectedmnuitem_, sel && sel->isOK(), true );
	
	const bool enablesave = applMgr()->EMServer()->isChanged(emid_) &&
	    applMgr()->EMServer()->isFullyLoaded(emid_);
	
	mAddMenuItem( menu, &savemnuitem_, enablesave, false );
	mAddMenuItem( menu, &saveasmnuitem_, true, false );
    }
}


void uiODBodyDisplayTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(uiMenuHandler*,menu,caller);
    if ( menu->menuID()!=displayID() || mnuid==-1 || menu->isHandled() )
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
	
	if (saveas && plg_ && !applMgr()->EMServer()->getName(emid_).isEmpty())
	{
	    plg_->setName( applMgr()->EMServer()->getName(emid_));
	    updateColumnText( uiODSceneMgr::cNameColumn() );
	}
	
	if (saveas && mcd_ && !applMgr()->EMServer()->getName(emid_).isEmpty())
	{
	    mcd_->setName( applMgr()->EMServer()->getName(emid_));
	    updateColumnText( uiODSceneMgr::cNameColumn() );
	}
    }
    else if ( mnuid==newellipsoidmnuitem_.id )
    {
	menu->setIsHandled(true);
	MouseCursorChanger cursorchanger( MouseCursor::Wait );

	if ( mcd_->createInitialBody( true ) )
	    mcd_->removeInitialDragger();
    }
    else if ( mnuid==displaybodymnuitem_.id )
    {
	menu->setIsHandled(true);
	plg_->display( true, true );
	plg_->displayIntersections( false );
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
	plg_->removeSelection( *visserv_->getCoordSelector(sceneID()) );
    }
    else if ( mnuid==beziernrmnuitem_.id )
    {
	menu->setIsHandled(true);
	Geometry::PolygonSurface* body = !plg_->getEMPolygonBody() ? 0 :
	    plg_->getEMPolygonBody()->geometry().sectionGeometry( 0 );
	if ( body )
	{
    	    uiPolygonSurfBezierDlg dlg( getUiParent(), body );
    	    dlg.go();
	
	    plg_->touchAll( false, true );
	}
    }
}



