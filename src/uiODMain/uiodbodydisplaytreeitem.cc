/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodbodydisplaytreeitem.h"

#include "ascstream.h"
#include "conn.h"
#include "embody.h"
#include "emmanager.h"
#include "emmarchingcubessurface.h"
#include "empolygonbody.h"
#include "emrandomposbody.h"
#include "ioman.h"
#include "ioobj.h"
#include "mousecursor.h"
#include "randcolor.h"
#include "streamconn.h"
#include "threadwork.h"

#include "uimain.h"
#include "uibodyoperatordlg.h"
#include "uiempartserv.h"
#include "uiimpbodycaldlg.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uinotsaveddlg.h"
#include "uiodapplmgr.h"
#include "uistrings.h"
#include "uiodscenemgr.h"
#include "uitaskrunner.h"
#include "uivispartserv.h"

#include "vismarchingcubessurfacedisplay.h"
#include "vispolygonbodydisplay.h"
#include "visrandomposbodydisplay.h"




CNotifier<uiODBodyDisplayParentTreeItem,uiMenu*>&
	uiODBodyDisplayParentTreeItem::showMenuNotifier()
{
    static CNotifier<uiODBodyDisplayParentTreeItem,uiMenu*> notif( nullptr );
    return notif;
}


uiODBodyDisplayParentTreeItem::uiODBodyDisplayParentTreeItem()
    : uiODParentTreeItem( uiStrings::sGeobody() )
{}


uiODBodyDisplayParentTreeItem::~uiODBodyDisplayParentTreeItem()
{}


const char* uiODBodyDisplayParentTreeItem::iconName() const
{ return "tree-body"; }


bool uiODBodyDisplayParentTreeItem::showSubMenu()
{
    mDynamicCastGet(visSurvey::Scene*,scene,
		    ODMainWin()->applMgr().visServer()->getObject(sceneID()));
    if ( scene && scene->getZAxisTransform() )
    {
	uiMSG().message( tr("Cannot add Geobodies to this scene") );
	return false;
    }

    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertAction( new uiAction(m3Dots(uiStrings::sAdd())), 0 );
    mnu.insertAction( new uiAction(tr("New polygon based Geobody")), 1 );
    showMenuNotifier().trigger( &mnu, this );

    if ( children_.size() )
    {
	mnu.insertSeparator();
	uiMenu* displaymnu =
		new uiMenu( getUiParent(), tr("Display All") );
	displaymnu->insertAction( new uiAction(tr("Only at Sections")), 2 );
	displaymnu->insertAction( new uiAction(tr("In Full")), 3 );
	mnu.addMenu( displaymnu );
    }

    addStandardItems( mnu );

    const int mnuid = mnu.exec();
    if ( mnuid==0 )
	loadBodies();
    else if ( mnuid==1 )
    {
	RefMan<EM::EMObject> plg =
	    EM::EMM().createTempObject( EM::PolygonBody::typeStr() );
	if ( !plg )
	    return false;

	plg->setPreferredColor( OD::getRandomColor(false) );
	plg->setNewName();
	plg->setFullyLoaded( true );
	addChild( new uiODBodyDisplayTreeItem( plg->id() ), false );
    }
    else if ( mnuid==2 || mnuid==3 )
    {
	MouseCursorChanger mcc( MouseCursor::Wait );
	const bool displayatsections = mnuid==2;
	for ( int idx=0; idx<children_.size(); idx++ )
	{
	    mDynamicCastGet(uiODBodyDisplayTreeItem*,itm,children_[idx])
	    if ( itm )
		itm->setOnlyAtSectionsDisplay( displayatsections );
	}
    }
    else
	handleStandardItems( mnuid );

    return true;
}


void uiODBodyDisplayParentTreeItem::loadBodies()
{
    ObjectSet<EM::EMObject> objs;
    applMgr()->EMServer()->selectBodies( objs );
    TypeSet<EM::ObjectID> oids;

    for ( int idx=0; idx<objs.size(); idx++ )
    {
	oids += objs[idx]->id();
	const StringView stype = objs[idx]->getTypeStr();
	if ( stype != EM::MarchingCubesSurface::typeStr() &&
	     stype != "MarchingCubesSurface" )
	    continue;

	const MultiID& mid = objs[idx]->multiID();
	PtrMan<IOObj> ioobj = IOM().get( mid );
	PtrMan<Conn> conn = ioobj ? ioobj->getConn( Conn::Read ) : 0;
	if ( !conn )
	    continue;

	od_istream& strm = ((StreamConn*)conn.ptr())->iStream();
	ascistream astream( strm );
	const int majorversion = astream.majorVersion();
	const int minorversion = astream.minorVersion();
	if ( majorversion>4 || (majorversion==4 && minorversion>2) )
	    continue;

	uiString msg = tr("The geobody '%1' is made in OpendTect V%2, "
			  "do you want to convert it to current version?")
		     .arg(ioobj->name()).arg(astream.version());
	if ( !uiMSG().askGoOn(msg) )
	    continue;

	uiImplicitBodyValueSwitchDlg dlg( getUiParent(), ioobj );
	if ( !dlg.go() )
	    continue;

	EM::EMObject* emobj =
	    EM::EMM().loadIfNotFullyLoaded( dlg.getBodyMid() );
	if ( emobj )
	{
	    emobj->ref();
	    objs.replace( idx, emobj )->unRef();
	    oids[idx] = emobj->id();
	}
    }

    MouseCursorChanger uics( MouseCursor::Wait );
    for ( int idx=0; idx<oids.size(); idx++ )
    {
	setMoreObjectsToDoHint( idx<oids.size()-1 );
	addChild( new uiODBodyDisplayTreeItem(oids[idx]), false );
    }

    deepUnRef( objs );
}

uiTreeItem* uiODBodyDisplayTreeItemFactory::createForVis( VisID visid,
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
    , savemnuitem_(uiStrings::sSave()) \
    , saveasmnuitem_(uiStrings::sSaveAs()) \
    , displaybodymnuitem_(uiStrings::sGeobody()) \
    , displaypolygonmnuitem_(uiODBodyDisplayTreeItem::sPickedPolygons()) \
    , displayintersectionmnuitem_(uiStrings::sOnlyAtSections()) \
    , singlecolormnuitem_(uiStrings::sUseSingleColor()) \
    , volcalmnuitem_(m3Dots(uiODBodyDisplayTreeItem::sCalcVolume()))

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

    mAttachCB( NotSavedPrompter::NSP().promptSaving,
	       uiODBodyDisplayTreeItem::askSaveCB );
}


uiODBodyDisplayTreeItem::uiODBodyDisplayTreeItem( VisID id, bool dummy )
    : uiODDisplayTreeItem()
    mCommonInit
{
    displayid_ = id;
    mCommonInit2

    mAttachCB( NotSavedPrompter::NSP().promptSaving,
	       uiODBodyDisplayTreeItem::askSaveCB );
}


uiODBodyDisplayTreeItem::~uiODBodyDisplayTreeItem()
{
    detachAllNotifiers();
    unRefAndNullPtr( mcd_ );
    unRefAndNullPtr( plg_ );
    unRefAndNullPtr( rpb_ );

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
    if ( !displayid_.isValid() )
    {
	EM::EMObject* object = EM::EMM().getObject( emid_ );
	mDynamicCastGet( EM::PolygonBody*, emplg, object );
	mDynamicCastGet( EM::MarchingCubesSurface*, emmcs, object );
	mDynamicCastGet( EM::RandomPosBody*, emrpb, object );
	if ( emplg )
	{
	    visSurvey::PolygonBodyDisplay* plg =
		new visSurvey::PolygonBodyDisplay;
	    plg_ = plg;
	    plg_->ref();
	    if ( !plg_->setEMID( emid_ ) )
	    {
		plg_->unRef();
		plg_ = 0;
		return false;
	    }

	    displayid_ = plg->id();
	    visserv_->addObject( plg, sceneID(), true );
	}
	else if ( emmcs )
	{
	    visSurvey::MarchingCubesDisplay* mcd =
		new visSurvey::MarchingCubesDisplay;
	    mcd_ = mcd;
	    mcd_->ref();
	    uiTaskRunner taskrunner( getUiParent() );
	    if ( !mcd_->setEMID( emid_, &taskrunner ) )
	    {
		mcd_->unRef();
		mcd_ = 0;
		return false;
	    }

	    displayid_ = mcd->id();
	    visserv_->addObject( mcd, sceneID(), true );
	}
	else if ( emrpb )
	{
	    visSurvey::RandomPosBodyDisplay* rpb =
		new visSurvey::RandomPosBodyDisplay;
	    rpb_ = rpb;
	    rpb_->ref();
	    if ( !rpb_->setEMID( emid_ ) )
	    {
		rpb_->unRef();
		rpb_ = 0;
		return false;
	    }

	    displayid_ = rpb->id();
	    visserv_->addObject( rpb, sceneID(), true );
	}
    }
    else
    {
	mDynamicCastGet(visSurvey::PolygonBodyDisplay*,plg,
			visserv_->getObject(displayid_));
	mDynamicCastGet(visSurvey::MarchingCubesDisplay*,mcd,
			visserv_->getObject(displayid_));
	mDynamicCastGet(visSurvey::RandomPosBodyDisplay*,rpb,
			visserv_->getObject(displayid_));
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
	mAttachCB( plg_->materialChange(),uiODBodyDisplayTreeItem::colorChgCB );

    if ( mcd_ )
	mAttachCB( mcd_->materialChange(),uiODBodyDisplayTreeItem::colorChgCB );

    if ( rpb_ )
	mAttachCB( rpb_->materialChange(),uiODBodyDisplayTreeItem::colorChgCB );

    mAttachCB( uiMain::keyboardEventHandler().keyPressed,
	uiODBodyDisplayTreeItem::keyPressedCB );
    return uiODDisplayTreeItem::init();
}



void uiODBodyDisplayTreeItem::keyPressedCB( CallBacker* )
{
    mDynamicCastGet( visSurvey::PolygonBodyDisplay*,pbd,
	visserv_->getObject(displayid_) )
    if ( !pbd || !pbd->isSelected() )
	    return;

    mDynamicCastGet( EM::EMUndo*,emundo,&EM::EMM().undo(emObjectID()) );
    if ( !emundo || !uiMain::keyboardEventHandler().hasEvent() )
	return;

    const KeyboardEvent& kbe = uiMain::keyboardEventHandler().event();

    bool update = false;
    if ( KeyboardEvent::isUnDo(kbe) )
    {
	EM::EMM().burstAlertToAll( true );
	update = emundo->unDo( 1, true );
	EM::EMM().burstAlertToAll( false );
    }
    else if ( KeyboardEvent::isReDo(kbe) )
    {
	EM::EMM().burstAlertToAll( true );
	update = emundo->reDo( 1, true );
	EM::EMM().burstAlertToAll( false );
    }

    if ( update )
	pbd->requestSingleRedraw();
}


void uiODBodyDisplayTreeItem::askSaveCB( CallBacker* )
{
    uiEMPartServer* ems = applMgr()->EMServer();
    if ( !ems->isChanged(emid_) )
	return;

    bool savewithname = EM::EMM().getMultiID( emid_ ).isUdf();
    if ( !savewithname )
	savewithname = !IOM().implExists( EM::EMM().getMultiID(emid_) );

    const uiString obj = toUiString("%1 \"%2\"")
	.arg( ems->getType(emid_) ).arg( ems->getUiName(emid_) );
    NotSavedPrompter::NSP().addObject( obj,
		mCB(this,uiODBodyDisplayTreeItem,saveCB),
		savewithname, 0 );

    Threads::WorkManager::twm().addWork(
	    Threads::Work( *new uiTreeItemRemover(parent_, this), true ), 0,
	    NotSavedPrompter::NSP().queueID(), false );
}


void uiODBodyDisplayTreeItem::saveCB( CallBacker* )
{
    const bool issaved =
	applMgr()->EMServer()->storeObject( emid_, true );

    if ( issaved && !applMgr()->EMServer()->getUiName(emid_).isEmpty() )
    {
	const uiEMPartServer* emps = applMgr()->EMServer();
	applMgr()->visServer()->setUiObjectName( displayid_,
						 emps->getUiName(emid_) );
	updateColumnText( uiODSceneMgr::cNameColumn() );
	NotSavedPrompter::NSP().reportSuccessfullSave();
    }
}


bool uiODBodyDisplayTreeItem::askContinueAndSaveIfNeeded( bool withcancel )
{
    return applMgr()->EMServer()->askUserToSave( emObjectID(), withcancel );
}


void uiODBodyDisplayTreeItem::prepareForShutdown()
{
    detachAllNotifiers();
    unRefAndNullPtr( mcd_ );
    unRefAndNullPtr( plg_ );
    unRefAndNullPtr( rpb_ );


    uiODDisplayTreeItem::prepareForShutdown();
}


void uiODBodyDisplayTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODDisplayTreeItem::createMenu( menu, istb );
    if ( !menu || menu->menuID()!=displayID().asInt() || istb )
	return;

    if ( !mcd_ && !plg_ && !rpb_ )
	return;

    const bool enablesave = applMgr()->EMServer()->isChanged(emid_) &&
			    applMgr()->EMServer()->isFullyLoaded(emid_);
    if ( mcd_ )
    {
	const bool intersectdisplay = mcd_->displayedOnlyAtSections();
	mAddMenuItem( menu, &displaymnuitem_, true, true );
	mAddMenuItem( &displaymnuitem_, &displaybodymnuitem_, true,
		!intersectdisplay );
	mAddMenuItem( &displaymnuitem_, &displayintersectionmnuitem_, true,
		intersectdisplay );
	mAddMenuItem( &displaymnuitem_, &singlecolormnuitem_,
		      mcd_->canShowTexture(), !mcd_->showsTexture() );
    }

    if ( plg_ )
    {
	mAddMenuItem( &displaymnuitem_, &displaybodymnuitem_, true,
		      plg_->isBodyDisplayed() );
	mAddMenuItem( &displaymnuitem_, &displaypolygonmnuitem_, true,
		      plg_->arePolygonsDisplayed() );
	mAddMenuItem( &displaymnuitem_, &displayintersectionmnuitem_, true,
		      plg_->displayedOnlyAtSections() );
	mAddMenuItem( menu, &displaymnuitem_, true, true );
    }

    mAddMenuItem( menu, &volcalmnuitem_, true, true );
    mAddMenuItem( menu, &savemnuitem_, enablesave, false );
    mAddMenuItem( menu, &saveasmnuitem_, true, false );
}


void uiODBodyDisplayTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( menu->isHandled() || menu->menuID()!=displayID().asInt() || mnuid==-1 )
	return;

    if ( mnuid==saveasmnuitem_.id || mnuid==savemnuitem_.id )
    {
	bool saveas = mnuid==saveasmnuitem_.id ||
	    applMgr()->EMServer()->getStorageID(emid_).isUdf();
	if ( !saveas )
	{
	    PtrMan<IOObj> ioobj =
		IOM().get( applMgr()->EMServer()->getStorageID(emid_) );
	    saveas = !ioobj;
	}

	applMgr()->EMServer()->storeObject( emid_, saveas );
	const bool notempty =
	    !applMgr()->EMServer()->getUiName(emid_).isEmpty();
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
	const bool bodydisplay = !displaybodymnuitem_.checked;
	if ( plg_ )
	{
	    const bool polygondisplayed = displaypolygonmnuitem_.checked;
	    plg_->display( polygondisplayed, bodydisplay );
	    plg_->setOnlyAtSectionsDisplay( !polygondisplayed && !bodydisplay );
	}
	else if ( mcd_ )
	    mcd_->setOnlyAtSectionsDisplay( !bodydisplay );
    }
    else if ( mnuid==displaypolygonmnuitem_.id )
    {
	const bool polygondisplay = !displaypolygonmnuitem_.checked;
	const bool bodydisplayed = displaybodymnuitem_.checked;
	plg_->display( polygondisplay, bodydisplayed );
	plg_->setOnlyAtSectionsDisplay( !polygondisplay && !bodydisplayed );
    }
    else if ( mnuid==displayintersectionmnuitem_.id )
    {
	const bool intersectdisplay = !displayintersectionmnuitem_.checked;
	setOnlyAtSectionsDisplay( intersectdisplay );
    }
    else if ( mnuid==singlecolormnuitem_.id )
    {
	mcd_->useTexture( !mcd_->showsTexture(), true );
    }
    else
	return;

    menu->setIsHandled( true );
}


void uiODBodyDisplayTreeItem::setOnlyAtSectionsDisplay( bool yn )
{
    if ( plg_ )
    {
	plg_->display( false, !yn );
	plg_->setOnlyAtSectionsDisplay( yn );
    }
    else if ( mcd_ )
	mcd_->setOnlyAtSectionsDisplay( yn );
}



// uiODBodyDisplayDataTreeItem
uiODBodyDisplayDataTreeItem::uiODBodyDisplayDataTreeItem( const char* ptype )
    : uiODAttribTreeItem( ptype )
    , depthattribmnuitem_(tr("Z values"))
    , isochronmnuitem_(tr("Vertical thickness"))
{}


uiODBodyDisplayDataTreeItem::~uiODBodyDisplayDataTreeItem()
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
    mAddMenuItem( &selattrmnuitem_, &isochronmnuitem_, !islocked, yn );
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
    else if ( mnuid==isochronmnuitem_.id )
    {
	menu->setIsHandled( true );
	mcd->setIsopach( attribNr() );
    }

    updateColumnText( uiODSceneMgr::cNameColumn() );
    updateColumnText( uiODSceneMgr::cColorColumn() );
    applMgr()->updateColorTable( displayID(), attribNr() );
}


uiString uiODBodyDisplayDataTreeItem::createDisplayName() const
{
    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    const Attrib::SelSpec* as = visserv->getSelSpec( displayID(), attribNr() );

    if ( as->id().asInt()==Attrib::SelSpec::cNoAttrib().asInt() )
	return toUiString(as->userRef());

    return uiODAttribTreeItem::createDisplayName();
}
