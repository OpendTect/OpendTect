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
    RefMan<visSurvey::Scene> scene =
		    ODMainWin()->applMgr().visServer()->getScene( sceneID() );
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
	auto* displaymnu = new uiMenu( getUiParent(), tr("Display All") );
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

	uiImplicitBodyValueSwitchDlg dlg( getUiParent(), ioobj.ptr() );
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


uiTreeItem* uiODBodyDisplayTreeItemFactory::createForVis( const VisID& visid,
							  uiTreeItem* ) const
{
    const visBase::DataObject* visobj =
		ODMainWin()->applMgr().visServer()->getObject(visid);
    if ( !visobj )
	return nullptr;

    mDynamicCastGet(const visSurvey::PolygonBodyDisplay*,plg,visobj);
    if ( plg )
	return new uiODBodyDisplayTreeItem( visid, true );

    mDynamicCastGet(const visSurvey::MarchingCubesDisplay*,mcd,visobj);
    if ( mcd )
	return new uiODBodyDisplayTreeItem( visid, true );

    mDynamicCastGet(const visSurvey::RandomPosBodyDisplay*,rpbd,visobj);
    if ( rpbd )
	return new uiODBodyDisplayTreeItem( visid, true );

    return nullptr;
}


uiODBodyDisplayTreeItem::uiODBodyDisplayTreeItem()
    : uiODDisplayTreeItem()
    , savemnuitem_(uiStrings::sSave())
    , saveasmnuitem_(uiStrings::sSaveAs())
    , displaybodymnuitem_(uiStrings::sGeobody())
    , displaypolygonmnuitem_(uiODBodyDisplayTreeItem::sPickedPolygons())
    , displayintersectionmnuitem_(uiStrings::sOnlyAtSections())
    , singlecolormnuitem_(uiStrings::sUseSingleColor())
    , volcalmnuitem_(m3Dots(uiODBodyDisplayTreeItem::sCalcVolume()))
{
    displaybodymnuitem_.checkable = true;
    displaypolygonmnuitem_.checkable = true;
    displayintersectionmnuitem_.checkable = true;
    singlecolormnuitem_.checkable = true;
    savemnuitem_.iconfnm = "save";
    saveasmnuitem_.iconfnm = "saveas";
    mAttachCB( NotSavedPrompter::NSP().promptSaving,
	       uiODBodyDisplayTreeItem::askSaveCB );
}


uiODBodyDisplayTreeItem::uiODBodyDisplayTreeItem( const EM::ObjectID& oid )
    : uiODBodyDisplayTreeItem()
{
    emid_ = oid;
}


uiODBodyDisplayTreeItem::uiODBodyDisplayTreeItem( const VisID& id, bool dummy )
    : uiODBodyDisplayTreeItem()
{
    displayid_ = id;
}


uiODBodyDisplayTreeItem::~uiODBodyDisplayTreeItem()
{
    detachAllNotifiers();
    visserv_->removeObject( displayid_, sceneID() );
}


bool uiODBodyDisplayTreeItem::isOK() const
{
    return plg_ || mcd_ || rpb_;
}


uiODDataTreeItem* uiODBodyDisplayTreeItem::createAttribItem(
					const Attrib::SelSpec* as ) const
{
    const char* parenttype = typeid(*this).name();
    uiODDataTreeItem* res = as
	? uiODDataTreeItem::factory().create( 0, *as, parenttype, false)
	: nullptr;

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
	    RefMan<visSurvey::PolygonBodyDisplay> plg =
					    new visSurvey::PolygonBodyDisplay;
	    if ( !plg->setEMID(emid_) )
		return false;

	    displayid_ = plg->id();
	    visserv_->addObject( plg.ptr(), sceneID(), true);
	}
	else if ( emmcs )
	{
	    RefMan<visSurvey::MarchingCubesDisplay> mcd =
					new visSurvey::MarchingCubesDisplay;
	    uiTaskRunner taskrunner( getUiParent() );
	    if ( !mcd->setEMID(emid_,&taskrunner) )
		return false;

	    displayid_ = mcd->id();
	    visserv_->addObject( mcd.ptr(), sceneID(), true);
	}
	else if ( emrpb )
	{
	    RefMan<visSurvey::RandomPosBodyDisplay> rpb =
					new visSurvey::RandomPosBodyDisplay;
	    if ( !rpb->setEMID(emid_) )
		return false;

	    displayid_ = rpb->id();
	    visserv_->addObject( rpb.ptr(), sceneID(), true);
	}
    }

    visBase::DataObject* visobj = visserv_->getObject( displayid_ );
    RefMan<visSurvey::PolygonBodyDisplay> plg =
			    dCast( visSurvey::PolygonBodyDisplay*, visobj );
    RefMan<visSurvey::MarchingCubesDisplay> mcd =
			    dCast( visSurvey::MarchingCubesDisplay*, visobj );
    RefMan<visSurvey::RandomPosBodyDisplay> rpb =
			    dCast( visSurvey::RandomPosBodyDisplay*, visobj );
    if ( plg )
    {
	//emid_ = plg->getEMID();
	mAttachCB( plg->materialChange(),uiODBodyDisplayTreeItem::colorChgCB );
	plg_ = plg;
    }
    else if ( mcd )
    {
	//emid_ = mcd->getEMID();
	mAttachCB( mcd->materialChange(),uiODBodyDisplayTreeItem::colorChgCB );
	mcd_ = mcd;
    }
    else if ( rpb )
    {
	//emid_ = rpb->getEMID();
	mAttachCB( rpb->materialChange(),uiODBodyDisplayTreeItem::colorChgCB );
	rpb_ = rpb;
    }
    else
	return false;

    mAttachCB( uiMain::keyboardEventHandler().keyPressed,
	       uiODBodyDisplayTreeItem::keyPressedCB );

    return uiODDisplayTreeItem::init();
}


ConstRefMan<visSurvey::PolygonBodyDisplay>
				uiODBodyDisplayTreeItem::getPBDisplay() const
{
    return plg_.get();
}


ConstRefMan<visSurvey::MarchingCubesDisplay>
				uiODBodyDisplayTreeItem::getMCDisplay() const
{
    return mcd_.get();
}


ConstRefMan<visSurvey::RandomPosBodyDisplay>
				uiODBodyDisplayTreeItem::getRPBDisplay() const
{
    return rpb_.get();
}


RefMan<visSurvey::PolygonBodyDisplay> uiODBodyDisplayTreeItem::getPBDisplay()
{
    return plg_.get();
}


RefMan<visSurvey::MarchingCubesDisplay> uiODBodyDisplayTreeItem::getMCDisplay()
{
    return mcd_.get();
}


RefMan<visSurvey::RandomPosBodyDisplay> uiODBodyDisplayTreeItem::getRPBDisplay()
{
    return rpb_.get();
}


void uiODBodyDisplayTreeItem::keyPressedCB( CallBacker* )
{
    RefMan<visSurvey::PolygonBodyDisplay> pbd = getPBDisplay();
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


void uiODBodyDisplayTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODDisplayTreeItem::createMenu( menu, istb );
    if ( !menu || menu->menuID()!=displayID().asInt() || istb )
	return;

    if ( !isOK() )
	return;

    const bool enablesave = applMgr()->EMServer()->isChanged(emid_) &&
			    applMgr()->EMServer()->isFullyLoaded(emid_);

    ConstRefMan<visSurvey::PolygonBodyDisplay> plg = getPBDisplay();
    if ( plg )
    {
	mAddMenuItem( &displaymnuitem_, &displaybodymnuitem_, true,
		      plg->isBodyDisplayed() );
	mAddMenuItem( &displaymnuitem_, &displaypolygonmnuitem_, true,
		      plg->arePolygonsDisplayed() );
	mAddMenuItem( &displaymnuitem_, &displayintersectionmnuitem_, true,
		      plg->displayedOnlyAtSections() );
	mAddMenuItem( menu, &displaymnuitem_, true, true );
    }

    ConstRefMan<visSurvey::MarchingCubesDisplay> mcd = getMCDisplay();
    if ( mcd )
    {
	const bool intersectdisplay = mcd->displayedOnlyAtSections();
	mAddMenuItem( menu, &displaymnuitem_, true, true );
	mAddMenuItem( &displaymnuitem_, &displaybodymnuitem_, true,
		!intersectdisplay );
	mAddMenuItem( &displaymnuitem_, &displayintersectionmnuitem_, true,
		intersectdisplay );
	mAddMenuItem( &displaymnuitem_, &singlecolormnuitem_,
		      mcd->canShowTexture(), !mcd->showsTexture() );
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

    if ( !isOK() )
	return;

    RefMan<visSurvey::PolygonBodyDisplay> plg = getPBDisplay();
    RefMan<visSurvey::MarchingCubesDisplay> mcd = getMCDisplay();
    RefMan<visSurvey::RandomPosBodyDisplay> rpb = getRPBDisplay();

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
	    if ( plg )
		plg->setName( applMgr()->EMServer()->getName(emid_) );

	    if ( mcd )
		mcd->setName( applMgr()->EMServer()->getName(emid_) );

	    if ( rpb )
		rpb->setName( applMgr()->EMServer()->getName(emid_) );

	    updateColumnText( uiODSceneMgr::cNameColumn() );
	}
    }
    else if ( mnuid==volcalmnuitem_.id )
    {
	if ( plg && plg->getEMPolygonBody() )
	{
	    uiImplBodyCalDlg dlg(ODMainWin(),*plg->getEMPolygonBody());
	    dlg.go();
	}
	else if ( mcd && mcd->getMCSurface() )
	{
	    uiImplBodyCalDlg dlg( ODMainWin(), *mcd->getMCSurface() );
	    dlg.go();
	}
	else if ( rpb && rpb->getEMBody() )
	{
	    uiImplBodyCalDlg dlg( ODMainWin(), *rpb->getEMBody() );
	    dlg.go();
	}
    }
    else if ( mnuid==displaybodymnuitem_.id )
    {
	const bool bodydisplay = !displaybodymnuitem_.checked;
	if ( plg )
	{
	    const bool polygondisplayed = displaypolygonmnuitem_.checked;
	    plg->display( polygondisplayed, bodydisplay );
	    plg->setOnlyAtSectionsDisplay( !polygondisplayed && !bodydisplay );
	}
	else if ( mcd )
	    mcd->setOnlyAtSectionsDisplay( !bodydisplay );
    }
    else if ( mnuid==displaypolygonmnuitem_.id )
    {
	const bool polygondisplay = !displaypolygonmnuitem_.checked;
	const bool bodydisplayed = displaybodymnuitem_.checked;
	plg->display( polygondisplay, bodydisplayed );
	plg->setOnlyAtSectionsDisplay( !polygondisplay && !bodydisplayed );
    }
    else if ( mnuid==displayintersectionmnuitem_.id )
    {
	const bool intersectdisplay = !displayintersectionmnuitem_.checked;
	setOnlyAtSectionsDisplay( intersectdisplay );
    }
    else if ( mnuid==singlecolormnuitem_.id )
    {
	mcd->useTexture( !mcd->showsTexture(), true );
    }
    else
	return;

    menu->setIsHandled( true );
}


void uiODBodyDisplayTreeItem::setOnlyAtSectionsDisplay( bool yn )
{
    RefMan<visSurvey::PolygonBodyDisplay> plg = getPBDisplay();
    RefMan<visSurvey::MarchingCubesDisplay> mcd = getMCDisplay();
    if ( plg )
    {
	plg->display( false, !yn );
	plg->setOnlyAtSectionsDisplay( yn );
    }
    else if ( mcd )
	mcd->setOnlyAtSectionsDisplay( yn );
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
    if ( istb )
	return;

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

    RefMan<visSurvey::MarchingCubesDisplay> mcd =
	dCast( visSurvey::MarchingCubesDisplay*,
	       ODMainWin()->applMgr().visServer()->getObject(displayID()) );
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
