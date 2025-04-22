/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodfaultsettreeitem.h"

#include "datapointset.h"
#include "emfaultset3d.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "ioobj.h"
#include "mousecursor.h"

#include "uiempartserv.h"
#include "uiioobjseldlg.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uistrings.h"
#include "uitaskrunner.h"
#include "uivispartserv.h"


uiODFaultSetParentTreeItem::uiODFaultSetParentTreeItem()
    : uiODParentTreeItem( uiStrings::sFaultSet() )
{
}


uiODFaultSetParentTreeItem::~uiODFaultSetParentTreeItem()
{
}


const char* uiODFaultSetParentTreeItem::iconName() const
{ return "tree-fltset"; }


#define mAddMnuID	0
#define mNewMnuID	1

#define mDispInFull	2
#define mDispAtSect	3
#define mDispAtHors	4
#define mDispAtBoth	5
#define mDispPlanes	6


#define mInsertItm( menu, name, id, enable ) \
{ \
    uiAction* itm = new uiAction( name ); \
    menu->insertAction( itm, id ); \
    itm->setEnabled( enable ); \
}

bool uiODFaultSetParentTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertAction( new uiAction(m3Dots(uiStrings::sAdd())), mAddMnuID );
/*
    uiAction* newmenu = new uiAction( uiStrings::sNew() );
    mnu.insertAction( newmenu, mNewMnuID );
    newmenu->setEnabled( !hastransform && SI().has3D() );

    if ( children_.size() )
    {
	bool candispatsect = false;
	bool candispathors = false;
	for ( int idx=0; idx<children_.size(); idx++ )
	{
	    mDynamicCastGet( uiODFaultSetTreeItem*, itm, children_[idx] );
	    mDynamicCastGet( visSurvey::FaultSetDisplay*, fd,
			applMgr()->visServer()->getObject(itm->displayID()) );

	    if ( fd && fd->canDisplayIntersections() )
		candispatsect = true;
	    if ( fd && fd->canDisplayHorizonIntersections() )
		candispathors = true;
	}
	mnu.insertSeparator();
	uiMenu* dispmnu = new uiMenu( getUiParent(), tr("Display all") );

	mInsertItm( dispmnu, tr("In full"), mDispInFull, true );
	mInsertItm( dispmnu, tr("Only at Sections"), mDispAtSect,
		    candispatsect );
	mInsertItm( dispmnu, tr("Only at Horizons"), mDispAtHors,
		    candispathors );
	mInsertItm( dispmnu, tr("At Sections && Horizons"), mDispAtBoth,
					    candispatsect && candispathors );
	mInsertItm( dispmnu, tr("Fault Planes"), mDispPlanes, true );
	mnu.addMenu( dispmnu );
    }
*/

    addStandardItems( mnu );

    const int mnuid = mnu.exec();
    if ( mnuid==mAddMnuID )
    {
	RefMan<visSurvey::Scene> scene =
		    ODMainWin()->applMgr().visServer()->getScene( sceneID() );
	RefMan<ZAxisTransform> transform = scene
				? scene->getZAxisTransform() : nullptr;
	const ZDomain::Info* zinfo = nullptr;
	if ( transform )
	    zinfo = &transform->toZDomainInfo();
	else
	    zinfo = &SI().zDomainInfo();

	ObjectSet<EM::EMObject> objs;
	applMgr()->EMServer()->selectFaultSets( objs, getUiParent(), zinfo );
	MouseCursorChanger mcc( MouseCursor::Wait );
	for ( int idx=0; idx<objs.size(); idx++ )
	{
	    setMoreObjectsToDoHint( idx<objs.size()-1 );
	    addChild( new uiODFaultSetTreeItem(objs[idx]->id()), false );
	}

	deepUnRef( objs );
    }
    else if ( mnuid>=mDispInFull && mnuid<=mDispAtBoth )
    {
	for ( int idx=0; idx<children_.size(); idx++ )
	{
	    mDynamicCastGet( uiODFaultSetTreeItem*, itm, children_[idx] );
	    mDynamicCastGet( visSurvey::FaultSetDisplay*, fd,
			applMgr()->visServer()->getObject(itm->displayID()) );
	    if ( !fd ) continue;

	    fd->displayIntersections( mnuid==mDispAtSect );
	}
    }
    else
	handleStandardItems( mnuid );

    return true;
}


uiTreeItem* uiODFaultSetTreeItemFactory::createForVis( const VisID& visid,
							uiTreeItem* ) const
{
    mDynamicCastGet(visSurvey::FaultSetDisplay*,fd,
	    ODMainWin()->applMgr().visServer()->getObject(visid));
    return fd ? new uiODFaultSetTreeItem( visid, true ) : nullptr;
}


uiODFaultSetTreeItem::uiODFaultSetTreeItem()
    : uiODDisplayTreeItem()
    , savemnuitem_( uiStrings::sSave() )
    , saveasmnuitem_( m3Dots(uiStrings::sSaveAs() ))
    , displayplanemnuitem_( uiODFaultSetTreeItem::sFaultPlanes() )
    , displayintersectionmnuitem_( uiODFaultSetTreeItem::sOnlyAtSections() )
    , displayintersecthorizonmnuitem_( uiODFaultSetTreeItem::sOnlyAtHorizons() )
    , singlecolmnuitem_( uiStrings::sUseSingleColor() )
{
    displayplanemnuitem_.checkable = true;
    displayintersectionmnuitem_.checkable = true;
    displayintersecthorizonmnuitem_.checkable = true;
    singlecolmnuitem_.checkable = true;
    savemnuitem_.iconfnm = "save";
    saveasmnuitem_.iconfnm = "saveas";
}


uiODFaultSetTreeItem::uiODFaultSetTreeItem( const EM::ObjectID& oid )
    : uiODFaultSetTreeItem()
{
    emid_ = oid;
}


uiODFaultSetTreeItem::uiODFaultSetTreeItem( const VisID& id, bool /* dummy */ )
    : uiODFaultSetTreeItem()
{
    displayid_ = id;
}


uiODFaultSetTreeItem::~uiODFaultSetTreeItem()
{
    detachAllNotifiers();
    visserv_->removeObject( displayid_, sceneID() );
}


uiODDataTreeItem* uiODFaultSetTreeItem::createAttribItem(
					const Attrib::SelSpec* as ) const
{
    const char* parenttype = typeid(*this).name();
    uiODDataTreeItem* res = as
	? uiODDataTreeItem::factory().create( 0, *as, parenttype, false)
	: nullptr;

    if ( !res )
	res = new uiODFaultSetDataTreeItem( emid_, parenttype );

    return res;
}


bool uiODFaultSetTreeItem::init()
{
    if ( !displayid_.isValid() )
    {
	RefMan<visSurvey::FaultSetDisplay> fd = new visSurvey::FaultSetDisplay;
	displayid_ = fd->id();
	{ // TODO Should not be needed yet
	    ConstRefMan<visSurvey::Scene> scene =
		ODMainWin()->applMgr().visServer()->getScene( sceneID() );
	    if ( scene )
	    {
		ConstRefMan<ZAxisTransform> transform =
						scene->getZAxisTransform();
		fd->setZAxisTransform( transform.getNonConstPtr(), nullptr );
	    }

	    fd->setEMObjectID( emid_ );
	}

	visserv_->addObject( fd.ptr(), sceneID(), true);
    }

    mDynamicCastGet( visSurvey::FaultSetDisplay*, fd,
		     visserv_->getObject(displayid_) );
    if ( !fd )
	return false;

    if ( emid_.isValid() && fd->getEMObjectID() != emid_ )
	fd->setEMObjectID( emid_ );

    mAttachCB( fd->materialChange(), uiODFaultSetTreeItem::colorChCB );

    faultsetdisplay_ = fd;
    return uiODDisplayTreeItem::init();
}


ConstRefMan<visSurvey::FaultSetDisplay> uiODFaultSetTreeItem::getDisplay() const
{
    return faultsetdisplay_.get();
}


RefMan<visSurvey::FaultSetDisplay> uiODFaultSetTreeItem::getDisplay()
{
    return faultsetdisplay_.get();
}


void uiODFaultSetTreeItem::colorChCB( CallBacker* )
{
    updateColumnText( uiODSceneMgr::cColorColumn() );
}


bool uiODFaultSetTreeItem::askContinueAndSaveIfNeeded( bool withcancel )
{
    return applMgr()->EMServer()->askUserToSave( emid_, withcancel );
}


void uiODFaultSetTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODDisplayTreeItem::createMenu( menu, istb );
    if ( !menu || !isDisplayID(menu->menuID()) )
	return;

    ConstRefMan<visSurvey::FaultSetDisplay> faultsetdisplay = getDisplay();
    if ( !faultsetdisplay )
	return;

    if ( !istb )
    {
	mAddMenuItem( &displaymnuitem_, &displayintersectionmnuitem_,
		      faultsetdisplay->canDisplayIntersections(),
		      faultsetdisplay->areIntersectionsDisplayed() );
/*	mAddMenuItem( &displaymnuitem_, &displayintersecthorizonmnuitem_,
		      faultsetdisplay->canDisplayHorizonIntersections(),
		      faultsetdisplay->areHorizonIntersectionsDisplayed() );
	mAddMenuItem( &displaymnuitem_, &displayplanemnuitem_, true,
		      faultsetdisplay->arePanelsDisplayed() );
*/
	mAddMenuItem( menu, &displaymnuitem_, true, true );

	mAddMenuItem( &displaymnuitem_, &singlecolmnuitem_,
		      faultsetdisplay->canShowTexture(),
		      !faultsetdisplay->showsTexture() );
    }
/*
    const MultiID mid = EM::EMM().getMultiID( emid_ );
    const bool enablesave = applMgr()->EMServer()->isChanged(emid_) &&
			    applMgr()->EMServer()->isFullyLoaded(emid_) &&
			    EM::canOverwrite(mid);

    mAddMenuOrTBItem( istb, menu, menu, &savemnuitem_, enablesave, false );
    mAddMenuOrTBItem( istb, 0, menu, &saveasmnuitem_, true, false ); */
}


void uiODFaultSetTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( menu->isHandled() || !isDisplayID(menu->menuID()) || mnuid==-1 )
	return;

    RefMan<visSurvey::FaultSetDisplay> faultsetdisplay = getDisplay();
    if ( !faultsetdisplay )
	return;

    if ( mnuid==displayplanemnuitem_.id )
    {
	menu->setIsHandled(true);
	const bool planechecked = displayplanemnuitem_.checked;
	faultsetdisplay->displayPanels( planechecked );
    }
    else if ( mnuid==displayintersectionmnuitem_.id )
    {
	menu->setIsHandled(true);
	const bool interchecked = displayintersectionmnuitem_.checked;
	faultsetdisplay->displayIntersections( !interchecked );
    }
    else if ( mnuid==displayintersecthorizonmnuitem_.id )
    {
	menu->setIsHandled(true);
	const bool interchecked = displayintersecthorizonmnuitem_.checked;
	faultsetdisplay->displayHorizonIntersections( !interchecked );
    }
    else if ( mnuid==singlecolmnuitem_.id )
    {
	menu->setIsHandled(true);
	faultsetdisplay->useTexture( !faultsetdisplay->showsTexture(), true );
	visserv_->triggerTreeUpdate();
    }
}


void uiODFaultSetTreeItem::setOnlyAtSectionsDisplay( bool yn )
{
    uiODDisplayTreeItem::setOnlyAtSectionsDisplay( yn );
}


bool uiODFaultSetTreeItem::isOnlyAtSections() const
{
    return uiODDisplayTreeItem::displayedOnlyAtSections();
}


// uiODFaultSetDataTreeItem

uiODFaultSetDataTreeItem::uiODFaultSetDataTreeItem( EM::ObjectID objid,
	const char* parenttype )
    : uiODAttribTreeItem(parenttype)
    , depthattribmnuitem_( uiStrings::sZValue(mPlural) )
    , emid_(objid)
{}


uiODFaultSetDataTreeItem::~uiODFaultSetDataTreeItem()
{}


void uiODFaultSetDataTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODAttribTreeItem::createMenu( menu, istb );
    if ( istb )
	return;

    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    const Attrib::SelSpec* as = visserv->getSelSpec( displayID(), attribNr() );
    const bool islocked = visserv->isLocked( displayID() );
    mAddMenuItem( &selattrmnuitem_, &depthattribmnuitem_, !islocked,
	    as->id().asInt()==Attrib::SelSpec::cNoAttrib().asInt() );
}


void uiODFaultSetDataTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODAttribTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( mnuid==-1 || menu->isHandled() )
	return;

    const VisID visid = displayID();
    const int attribnr = attribNr();

    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    if ( mnuid==depthattribmnuitem_.id )
    {
	menu->setIsHandled( true );

	mDynamicCastGet( visSurvey::FaultSetDisplay*, fltdisp,
		visserv->getObject(visid) );
	fltdisp->setDepthAsAttrib( attribnr );
	updateColumnText( uiODSceneMgr::cNameColumn() );
	updateColumnText( uiODSceneMgr::cColorColumn() );
	applMgr()->updateColorTable( displayID(), attribnr );
	changed_ = false;
    }
}


void uiODFaultSetDataTreeItem::setDataPointSet( const DataPointSet& vals )
{
    const VisID visid = displayID();
    const int attribnr = attribNr();
    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    StringView attrnm = vals.nrCols()>1 ? vals.colName(1) : "";
    visserv->setSelSpec( visid, attribnr,
	    Attrib::SelSpec(attrnm,Attrib::SelSpec::cOtherAttrib()) );
    visserv->setRandomPosData( visid, attribnr, &vals );
    visserv->selectTexture( visid, attribnr, 0 );
    updateColumnText( uiODSceneMgr::cNameColumn() );
}


uiString uiODFaultSetDataTreeItem::createDisplayName() const
{
    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    const Attrib::SelSpec* as = visserv->getSelSpec( displayID(), attribNr() );

    if ( as->id().asInt()==Attrib::SelSpec::cNoAttrib().asInt() )
	return uiStrings::sZValue(mPlural);

    return uiODAttribTreeItem::createDisplayName();
}
