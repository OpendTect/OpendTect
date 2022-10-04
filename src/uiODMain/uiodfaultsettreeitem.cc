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
#include "ioman.h"
#include "ioobj.h"
#include "mousecursor.h"
#include "randcolor.h"

#include "uiempartserv.h"
#include "uiioobjseldlg.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uiodviewer2dmgr.h"
#include "uistrings.h"
#include "uitaskrunner.h"
#include "uitreeview.h"
#include "uivispartserv.h"

#include "visfaultsetdisplay.h"


uiODFaultSetParentTreeItem::uiODFaultSetParentTreeItem()
    : uiODParentTreeItem( uiStrings::sFaultSet() )
{
}


uiODFaultSetParentTreeItem::~uiODFaultSetParentTreeItem()
{
}


const char* uiODFaultSetParentTreeItem::iconName() const
{ return "faultplanes"; }


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
    mDynamicCastGet(visSurvey::Scene*,scene,
		    ODMainWin()->applMgr().visServer()->getObject(sceneID()));
    const bool hastransform = scene && scene->getZAxisTransform();
    if ( hastransform )
    {
	uiMSG().message( tr("Cannot add Faults to this scene") );
	return false;
    }

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
	CtxtIOObj ctio( mIOObjContext(EMFaultSet3D) );
	ctio.ctxt_.forread_ = true;
	uiIOObjSelDlg dlg( getUiParent(), ctio );
	dlg.setCaption( tr("Load FaultSet") );
	if ( !dlg.go() )
	    return false;

	const IOObj* ioobj = dlg.ioObj();
	if ( !ioobj )
	    return false;

	PtrMan<EMFaultSet3DTranslator> transl =
			(EMFaultSet3DTranslator*)ioobj->createTranslator();
	if ( !transl )
	    return false;

	mDynamicCastGet( EM::FaultSet3D*, fltset,
			 EM::EMM().createTempObject(EM::FaultSet3D::typeStr()));
	PtrMan<Executor> loader = transl->reader( *fltset, *ioobj );
	uiTaskRunner tskr( getUiParent() );
	if ( !tskr.execute(*loader) )
	    return false;

	MouseCursorChanger mcc( MouseCursor::Wait );
	addChild( new uiODFaultSetTreeItem(fltset->id()), false );
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


uiTreeItem* uiODFaultSetTreeItemFactory::createForVis(
						VisID visid, uiTreeItem* ) const
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


uiODFaultSetTreeItem::uiODFaultSetTreeItem( VisID id, bool dummy )
    : uiODFaultSetTreeItem()
{
    displayid_ = id;
}


uiODFaultSetTreeItem::~uiODFaultSetTreeItem()
{
    if ( faultsetdisplay_ )
    {
	faultsetdisplay_->materialChange()->remove(
	    mCB(this,uiODFaultSetTreeItem,colorChCB));
	faultsetdisplay_->unRef();
    }
}


uiODDataTreeItem* uiODFaultSetTreeItem::createAttribItem(
	const Attrib::SelSpec* as ) const
{
    const char* parenttype = typeid(*this).name();
    uiODDataTreeItem* res = as
	? uiODDataTreeItem::factory().create( 0, *as, parenttype, false) : 0;
    if ( !res )
	res = new uiODFaultSetDataTreeItem( emid_, parenttype );
    return res;
}


bool uiODFaultSetTreeItem::init()
{
    if ( !displayid_.isValid() )
    {
	visSurvey::FaultSetDisplay* fd = new visSurvey::FaultSetDisplay;
	displayid_ = fd->id();
	faultsetdisplay_ = fd;
	faultsetdisplay_->ref();

	fd->setEMObjectID( emid_ );
	visserv_->addObject( fd, sceneID(), true );
    }
    else
    {
	mDynamicCastGet( visSurvey::FaultSetDisplay*, fd,
			 visserv_->getObject(displayid_) );
	if ( !fd )
	    return false;

	faultsetdisplay_ = fd;
	faultsetdisplay_->ref();
	emid_ = fd->getEMObjectID();
    }

    faultsetdisplay_->materialChange()->notify(
	    mCB(this,uiODFaultSetTreeItem,colorChCB));

    return uiODDisplayTreeItem::init();
}


void uiODFaultSetTreeItem::colorChCB( CallBacker* )
{
    updateColumnText( uiODSceneMgr::cColorColumn() );
}


bool uiODFaultSetTreeItem::askContinueAndSaveIfNeeded( bool withcancel )
{
    return applMgr()->EMServer()->askUserToSave( emid_, withcancel );
}


void uiODFaultSetTreeItem::prepareForShutdown()
{
    if ( faultsetdisplay_ )
    {
	faultsetdisplay_->materialChange()->remove(
	    mCB(this,uiODFaultSetTreeItem,colorChCB));
	faultsetdisplay_->unRef();
    }

    faultsetdisplay_ = 0;
    uiODDisplayTreeItem::prepareForShutdown();
}


void uiODFaultSetTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODDisplayTreeItem::createMenu( menu, istb );
    if ( !menu || !isDisplayID(menu->menuID()) )
	return;

    mDynamicCastGet(visSurvey::FaultSetDisplay*,fd,
	    ODMainWin()->applMgr().visServer()->getObject(displayID()));
    if ( !fd )
	return;

    if ( !istb )
    {
	mAddMenuItem( &displaymnuitem_, &displayintersectionmnuitem_,
		      faultsetdisplay_->canDisplayIntersections(),
		      faultsetdisplay_->areIntersectionsDisplayed() );
/*	mAddMenuItem( &displaymnuitem_, &displayintersecthorizonmnuitem_,
		      faultsetdisplay_->canDisplayHorizonIntersections(),
		      faultsetdisplay_->areHorizonIntersectionsDisplayed() );
	mAddMenuItem( &displaymnuitem_, &displayplanemnuitem_, true,
		      faultsetdisplay_->arePanelsDisplayed() );
*/
	mAddMenuItem( menu, &displaymnuitem_, true, true );

	mAddMenuItem( &displaymnuitem_, &singlecolmnuitem_,
		      faultsetdisplay_->canShowTexture(),
		      !faultsetdisplay_->showsTexture() );
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

    if ( mnuid==displayplanemnuitem_.id )
    {
	menu->setIsHandled(true);
	const bool planechecked = displayplanemnuitem_.checked;
	faultsetdisplay_->displayPanels( planechecked );
    }
    else if ( mnuid==displayintersectionmnuitem_.id )
    {
	menu->setIsHandled(true);
	const bool interchecked = displayintersectionmnuitem_.checked;
	faultsetdisplay_->displayIntersections( !interchecked );
    }
    else if ( mnuid==displayintersecthorizonmnuitem_.id )
    {
	menu->setIsHandled(true);
	const bool interchecked = displayintersecthorizonmnuitem_.checked;
	faultsetdisplay_->displayHorizonIntersections( !interchecked );
    }
    else if ( mnuid==singlecolmnuitem_.id )
    {
	menu->setIsHandled(true);
	faultsetdisplay_->useTexture( !faultsetdisplay_->showsTexture(), true );
	visserv_->triggerTreeUpdate();
    }
}


void uiODFaultSetTreeItem::setOnlyAtSectionsDisplay( bool yn )
{ uiODDisplayTreeItem::setOnlyAtSectionsDisplay( yn ); }


bool uiODFaultSetTreeItem::isOnlyAtSections() const
{ return uiODDisplayTreeItem::displayedOnlyAtSections(); }


// uiODFaultSetDataTreeItem
uiODFaultSetDataTreeItem::uiODFaultSetDataTreeItem( EM::ObjectID objid,
	const char* parenttype )
    : uiODAttribTreeItem(parenttype)
    , depthattribmnuitem_( uiStrings::sZValue(mPlural) )
    , changed_(false)
    , emid_(objid)
{}


uiODFaultSetDataTreeItem::~uiODFaultSetDataTreeItem()
{}


void uiODFaultSetDataTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODAttribTreeItem::createMenu( menu, istb );
    if ( istb ) return;

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
