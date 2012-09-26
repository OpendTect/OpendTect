/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author: 	K. Tingdahl
 Date: 		Jul 2003
___________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiodbodydisplaytreeitem.h"

#include "arrayndimpl.h"
#include "embody.h"
#include "empolygonbody.h"
#include "emmarchingcubessurface.h"
#include "emmanager.h"
#include "emrandomposbody.h"
#include "datapointset.h"
#include "ioman.h"
#include "ioobj.h"
#include "marchingcubes.h"
#include "mousecursor.h"
#include "randcolor.h"

#include "uiempartserv.h"
#include "uiimpbodycaldlg.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uitaskrunner.h"
#include "uivispartserv.h"
#include "vismarchingcubessurface.h"
#include "vismarchingcubessurfacedisplay.h"
#include "vispolygonbodydisplay.h"
#include "visrandomposbodydisplay.h"


/*test*/
#include "cubesampling.h"
#include "ranges.h"
#include "seisbuf.h"
#include "seisbufadapters.h"
#include "seisread.h"
#include "seiswrite.h"
#include "seisselectionimpl.h"
#include "seistrc.h"
#include "houghtransform.h"
#include "iodir.h"
#include "embodytr.h"
#include "emfault3d.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emmarchingcubessurface.h"
#include "emsurfacetr.h"
#include "explfaultsticksurface.h"
#include "explplaneintersection.h"
#include "executor.h"
#include "survinfo.h"



uiODBodyDisplayParentTreeItem::uiODBodyDisplayParentTreeItem()
   : uiODTreeItem( "Body" )
{}


bool uiODBodyDisplayParentTreeItem::showSubMenu()
{
    mDynamicCastGet(visSurvey::Scene*,scene,
		    ODMainWin()->applMgr().visServer()->getObject(sceneID()));
    if ( scene && scene->getZAxisTransform() )
    {
	uiMSG().message( "Cannot add Bodies to this scene" );
	return false;
    }

    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("&Add ..."), 0 );
    mnu.insertItem( new uiMenuItem("&New polygon body..."), 1 );
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
    else if ( mnuid==1 )
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
    else
	handleStandardItems( mnuid );

    return true;
}


uiTreeItem* uiODBodyDisplayTreeItemFactory::createForVis( int visid,
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


#define mCommonInit \
    , savemnuitem_("&Save") \
    , saveasmnuitem_("Save &as ...") \
    , volcalmnuitem_("Calculate &volume ...") \
    , displaybodymnuitem_("&Body") \
    , displaypolygonmnuitem_("&Picked polygons") \
    , displayintersectionmnuitem_("&Only at sections") \
    , singlecolormnuitem_("Use single &color") \
    , mcd_(0) \
    , plg_(0) \
    , rpb_(0)

#define mCommonInit2 \
    displaybodymnuitem_.checkable = true; \
    displaypolygonmnuitem_.checkable = true; \
    displayintersectionmnuitem_.checkable = true; \
    singlecolormnuitem_.checkable = true; \
    savemnuitem_.iconfnm = "save"; \
    saveasmnuitem_.iconfnm = "saveas";


uiODBodyDisplayTreeItem::uiODBodyDisplayTreeItem( const EM::ObjectID& oid )
    : uiODDisplayTreeItem()
    , emid_(oid)
    mCommonInit
{
    mCommonInit2
}


uiODBodyDisplayTreeItem::uiODBodyDisplayTreeItem( int id, bool dummy )
    : uiODDisplayTreeItem()
    , emid_(-1)
    mCommonInit
{
    displayid_ = id;
    mCommonInit2
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


uiODDataTreeItem* uiODBodyDisplayTreeItem::createAttribItem(
	const Attrib::SelSpec* as ) const
{
    const char* parenttype = typeid(*this).name();
    uiODDataTreeItem* res = as
	? uiODDataTreeItem::factory().create( 0, *as, parenttype, false) : 0;
    if ( !res ) 
	res = new uiODBodyDisplayDataTreeItem( parenttype );

    return res;
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
	    uiTaskRunner taskrunner( getUiParent() );
	    mcd_->setEMID( emid_, &taskrunner );
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
    uiODDisplayTreeItem::prepareForShutdown();
}


void uiODBodyDisplayTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODDisplayTreeItem::createMenu( menu, istb );
    if ( !menu || menu->menuID()!=displayID() || istb )
	return;

    mDynamicCastGet(visSurvey::MarchingCubesDisplay*,mcd,
	    ODMainWin()->applMgr().visServer()->getObject(displayID()));
    mDynamicCastGet(visSurvey::PolygonBodyDisplay*,plg,
	    ODMainWin()->applMgr().visServer()->getObject(displayID()));
    mDynamicCastGet(visSurvey::RandomPosBodyDisplay*,rpb,
	    ODMainWin()->applMgr().visServer()->getObject(displayID()));
    if ( !mcd && !plg && !rpb )
	return;
	
    const bool enablesave = applMgr()->EMServer()->isChanged(emid_) &&
			    applMgr()->EMServer()->isFullyLoaded(emid_);
    if ( mcd )
    {
	mAddMenuItem( menu, &displaymnuitem_, true, true );
	mAddMenuItem( &displaymnuitem_, &displaybodymnuitem_, true, true );
	mAddMenuItem( &displaymnuitem_, &displayintersectionmnuitem_, true, 
	       mcd_->areIntersectionsDisplayed()	);
	mAddMenuItem( &displaymnuitem_, &singlecolormnuitem_, true, 
		!mcd->usesTexture() );
	mAddMenuItem( menu, &volcalmnuitem_, true, true );
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
    }

    mAddMenuItem( menu, &savemnuitem_, enablesave, false );
    mAddMenuItem( menu, &saveasmnuitem_, true, false );
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
    else if ( mnuid==volcalmnuitem_.id )
    {
	menu->setIsHandled(true);
	if ( mcd_ && mcd_->getMCSurface() )
	{
	    uiImplBodyCalDlg dlg(ODMainWin(),*mcd_->getMCSurface());
    	    dlg.go();
	}
	else if ( plg_ && plg_->getEMPolygonBody() )
	{
	    uiImplBodyCalDlg dlg(ODMainWin(),*plg_->getEMPolygonBody());
    	    dlg.go();
	}
	else if ( rpb_ && rpb_->getEMBody() )
	{
	    uiImplBodyCalDlg dlg(ODMainWin(),*rpb_->getEMBody());
    	    dlg.go();
	}
    }
    else if ( mnuid==displaybodymnuitem_.id )
    {
	menu->setIsHandled(true);
	const bool bodydisplayed = displaybodymnuitem_.checked;
	if ( plg_ )
	{
    	    const bool polygdisplayed = displaypolygonmnuitem_.checked;
    	    plg_->display( polygdisplayed, !bodydisplayed );
	}
    }
    else if ( mnuid==displaypolygonmnuitem_.id )
    {
	menu->setIsHandled(true);
	const bool polygdisplayed = displaypolygonmnuitem_.checked;
	const bool bodydisplayed = displaybodymnuitem_.checked;
	plg_->display( !polygdisplayed, bodydisplayed );
    }
    else if ( mnuid==displayintersectionmnuitem_.id )
    {
	menu->setIsHandled(true);
	const bool intersectdisplayed = displayintersectionmnuitem_.checked;
	if ( plg_ )
	{
    	    const bool polygdisplayed = displaypolygonmnuitem_.checked;
    	    plg_->display( polygdisplayed, intersectdisplayed );
    	    plg_->displayIntersections( !intersectdisplayed );
	}
	else if ( mcd_ )
	    mcd_->displayIntersections( !intersectdisplayed );
    }
    else if ( mnuid==singlecolormnuitem_.id )
    {
	menu->setIsHandled(true);
	mcd_->useTexture( !mcd_->usesTexture() );
    }
}


uiODBodyDisplayDataTreeItem::uiODBodyDisplayDataTreeItem( const char* ptype )
    : uiODAttribTreeItem( ptype )
    , depthattribmnuitem_("Z values")
    , isopatchmnuitem_("Z isopach")  
{}


void uiODBodyDisplayDataTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODAttribTreeItem::createMenu( menu, istb );
    if ( istb ) return;

    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    const Attrib::SelSpec* as = visserv->getSelSpec( displayID(), attribNr() );
    const bool islocked = visserv->isLocked( displayID() );
    const bool yn = as->id().asInt()==Attrib::SelSpec::cNoAttrib().asInt();

    mAddMenuItem( &selattrmnuitem_, &depthattribmnuitem_, !islocked, yn );
    mAddMenuItem( &selattrmnuitem_, &isopatchmnuitem_, !islocked, yn );
}


void uiODBodyDisplayDataTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODAttribTreeItem::handleMenuCB(cb);

    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( mnuid==-1 || menu->isHandled() )
	return;
    
    mDynamicCastGet(visSurvey::MarchingCubesDisplay*,mcd,
	    ODMainWin()->applMgr().visServer()->getObject(displayID()));
    if ( !mcd )
	return;

    if ( mnuid==depthattribmnuitem_.id )
    {
	menu->setIsHandled( true );
	mcd->setDepthAsAttrib( attribNr() );
    }
    else if ( mnuid==isopatchmnuitem_.id )
    {
	menu->setIsHandled( true );
	mcd->setIsoPatch( attribNr() );
    }
	
    updateColumnText( uiODSceneMgr::cNameColumn() );
}


BufferString uiODBodyDisplayDataTreeItem::createDisplayName() const
{
    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    const Attrib::SelSpec* as = visserv->getSelSpec( displayID(), attribNr() );
    
    if ( as->id().asInt()==Attrib::SelSpec::cNoAttrib().asInt() )
	return as->userRef();
    
    return uiODAttribTreeItem::createDisplayName();
}

