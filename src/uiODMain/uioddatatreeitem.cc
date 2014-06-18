/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jul 2003
___________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uioddatatreeitem.h"

#include "uiamplspectrum.h"
#include "uifkspectrum.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uiodapplmgr.h"
#include "uioddisplaytreeitem.h"
#include "uiodscenemgr.h"
#include "uiodviewer2dmgr.h"
#include "uistatsdisplay.h"
#include "uistatsdisplaywin.h"
#include "uitreeview.h"
#include "uivispartserv.h"

#include "attribsel.h"
#include "pixmap.h"

//TODO:remove when Flattened scene ok for 2D Viewer
#include "emhorizonztransform.h"
#include "vissurvscene.h"


mImplFactory2Param( uiODDataTreeItem, const Attrib::SelSpec&,
		     const char*, uiODDataTreeItem::factory );

uiODDataTreeItem::uiODDataTreeItem( const char* parenttype )
    : uiTreeItem("")
    , parenttype_(parenttype)
    , menu_(0)
    , statswin_(0)
    , ampspectrumwin_(0)
    , fkspectrumwin_(0)
    , movemnuitem_("&Move")
    , movetotopmnuitem_("to &top")
    , movetobottommnuitem_("to &bottom")
    , moveupmnuitem_("&up")
    , movedownmnuitem_("&down")
    , displaymnuitem_("&Display")
    , removemnuitem_("&Remove",-1000)
    , changetransparencyitem_("Change &transparency ...")
    , statisticsitem_("Show &Histogram ...")
    , amplspectrumitem_("Show &Amplitude Spectrum ...")
    , fkspectrumitem_("Show &F-K Spectrum ...")
    , view2dwvaitem_("2D Viewer - &Wiggle")
    , view2dvditem_("2D Viewer")
{
    statisticsitem_.iconfnm = "histogram";
    removemnuitem_.iconfnm = "trashcan";
    view2dwvaitem_.iconfnm = "wva";
    view2dvditem_.iconfnm = "vd";
    amplspectrumitem_.iconfnm = "amplspectrum";

    movetotopmnuitem_.iconfnm = "totop";
    moveupmnuitem_.iconfnm = "uparrow";
    movedownmnuitem_.iconfnm = "downarrow";
    movetobottommnuitem_.iconfnm = "tobottom";
}


uiODDataTreeItem::~uiODDataTreeItem()
{
    if ( menu_ )
    {
	menu_->createnotifier.remove( mCB(this,uiODDataTreeItem,createMenuCB) );
	menu_->handlenotifier.remove( mCB(this,uiODDataTreeItem,handleMenuCB) );
	menu_->unRef();
    }

    delete statswin_;
    delete ampspectrumwin_;

    uiVisPartServer* visserv = applMgr()->visServer();
    MenuHandler* tb = visserv->getToolBarHandler();
    tb->createnotifier.remove( mCB(this,uiODDataTreeItem,addToToolBarCB) );
    tb->handlenotifier.remove( mCB(this,uiODDataTreeItem,handleMenuCB) );
}


int uiODDataTreeItem::uiTreeViewItemType() const
{
    uiVisPartServer* visserv = applMgr()->visServer();
    if ( visserv->canHaveMultipleAttribs( displayID() ) )
	return uiTreeViewItem::CheckBox;
    else
	return uiTreeItem::uiTreeViewItemType();
}


uiODApplMgr* uiODDataTreeItem::applMgr() const
{
    void* res = 0;
    getPropertyPtr( uiODTreeTop::applmgrstr(), res );
    return reinterpret_cast<uiODApplMgr*>( res );
}


bool uiODDataTreeItem::init()
{
    uiVisPartServer* visserv = applMgr()->visServer();
    if ( visserv->canHaveMultipleAttribs(displayID()) )
    {
	getItem()->stateChanged.notify( mCB(this,uiODDataTreeItem,checkCB) );
	uitreeviewitem_->setChecked( visserv->isAttribEnabled(displayID(),
				     attribNr() ) );
    }

    MenuHandler* tb = visserv->getToolBarHandler();
    tb->createnotifier.notify( mCB(this,uiODDataTreeItem,addToToolBarCB) );
    tb->handlenotifier.notify( mCB(this,uiODDataTreeItem,handleMenuCB) );

    return uiTreeItem::init();
}


void uiODDataTreeItem::checkCB( CallBacker* cb )
{
    uiVisPartServer* visserv = applMgr()->visServer();
    visserv->enableAttrib( displayID(), attribNr(), isChecked() );
}


bool uiODDataTreeItem::shouldSelect( int selid ) const
{
    const uiVisPartServer* visserv = applMgr()->visServer();
    return selid!=-1 && selid==displayID() &&
	   visserv->getSelAttribNr()==attribNr();
}


int uiODDataTreeItem::sceneID() const
{
    int sceneid=-1;
    getProperty<int>( uiODTreeTop::sceneidkey(), sceneid );
    return sceneid;
}


int uiODDataTreeItem::displayID() const
{
    mDynamicCastGet( uiODDisplayTreeItem*, odti, parent_ );
    return odti ? odti->displayID() : -1;
}


int uiODDataTreeItem::attribNr() const
{
    const uiVisPartServer* visserv = applMgr()->visServer();
    const int nrattribs = visserv->getNrAttribs( displayID() );
    const int attribnr = nrattribs-siblingIndex()-1;
    return attribnr<0 || attribnr>=nrattribs ? 0 : attribnr;
}


void uiODDataTreeItem::addToToolBarCB( CallBacker* cb )
{
    mDynamicCastGet(uiTreeItemTBHandler*,tb,cb);
    if ( !tb || tb->menuID() != displayID() || !isSelected() )
	return;

    createMenu( tb, true );
}


bool uiODDataTreeItem::showSubMenu()
{
    if ( !menu_ )
    {
	menu_ = new uiMenuHandler( getUiParent(), -1 );
	menu_->ref();
	menu_->createnotifier.notify( mCB(this,uiODDataTreeItem,createMenuCB) );
	menu_->handlenotifier.notify( mCB(this,uiODDataTreeItem,handleMenuCB) );
    }

    return menu_->executeMenu( uiMenuHandler::fromTree() );
}


void uiODDataTreeItem::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(MenuHandler*,menu,cb);
    createMenu( menu, false );
}


void uiODDataTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiVisPartServer* visserv = applMgr()->visServer();
    const bool isfirst = !siblingIndex();
    const bool islast = siblingIndex()==visserv->getNrAttribs( displayID())-1;

    const bool islocked = visserv->isLocked( displayID() );

    if ( !islocked && (!isfirst || !islast) )
    {
	mAddMenuOrTBItem( istb, 0, &movemnuitem_, &movetotopmnuitem_,
		      !islocked && !isfirst, false );
	mAddMenuOrTBItem( istb, 0, &movemnuitem_, &moveupmnuitem_,
		      !islocked && !isfirst, false );
	mAddMenuOrTBItem( istb, 0, &movemnuitem_, &movedownmnuitem_,
		      !islocked && !islast, false );
	mAddMenuOrTBItem( istb, 0, &movemnuitem_, &movetobottommnuitem_,
		      !islocked && !islast, false );

	mAddMenuOrTBItem( istb, 0, menu, &movemnuitem_, true, false );
    }
    else
    {
	mResetMenuItem( &movetotopmnuitem_ );
	mResetMenuItem( &moveupmnuitem_ );
	mResetMenuItem( &movedownmnuitem_ );
	mResetMenuItem( &movetobottommnuitem_ );

	mResetMenuItem( &movemnuitem_ );
    }

    mAddMenuOrTBItem( istb, 0, menu, &displaymnuitem_, true, false );
    const DataPack::ID dpid =
	visserv->getDisplayedDataPackID( displayID(), attribNr() );
    const bool hasdatapack = dpid>DataPack::cNoID();
    const bool isvert = visserv->isVerticalDisp( displayID() );
    if ( hasdatapack )
	mAddMenuOrTBItem( istb, menu, &displaymnuitem_,
			  &statisticsitem_, true, false)
    else
	mResetMenuItem( &statisticsitem_ )

    mDynamicCastGet(visSurvey::Scene*,scene,visserv->getObject(sceneID()))
    const bool hastransform = scene && scene->getZAxisTransform();
    if ( hasdatapack && isvert )
    {
	mAddMenuOrTBItem( istb, menu, &displaymnuitem_, &amplspectrumitem_,
			  true, false )
	mAddMenuOrTBItem( istb, 0, &displaymnuitem_, &fkspectrumitem_,
			  !hastransform, false )
    }
    else
    {
	mResetMenuItem( &amplspectrumitem_ )
	mResetMenuItem( &fkspectrumitem_ )
    }

    mAddMenuOrTBItem( istb, menu, menu, &removemnuitem_,
		  !islocked && visserv->canRemoveAttrib( displayID()), false );
    if ( visserv->canHaveMultipleAttribs(displayID()) && hasTransparencyMenu() )
	mAddMenuOrTBItem( istb, 0, &displaymnuitem_,
			  &changetransparencyitem_, true, false )
    else
	mResetMenuItem( &changetransparencyitem_ );

    if ( visserv->canBDispOn2DViewer(displayID()) && hasdatapack )
    {
	const Attrib::SelSpec* as =
	    visserv->getSelSpec( displayID(), attribNr() );
	const bool hasattrib =
	    as && as->id().asInt()!=Attrib::SelSpec::cAttribNotSel().asInt();

	mAddMenuOrTBItem( istb, menu, &displaymnuitem_, &view2dvditem_,
			  hasattrib, false )
	mResetMenuItem( &view2dwvaitem_ );
    }
    else
    {
	mResetMenuItem( &view2dwvaitem_ );
	mResetMenuItem( &view2dvditem_ );
    }
}


bool uiODDataTreeItem::select()
{
    applMgr()->visServer()->setSelObjectId( displayID(), attribNr() );
    return true;
}


void uiODDataTreeItem::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( mnuid==-1 || menu->isHandled() )
	return;

    uiVisPartServer* visserv = applMgr()->visServer();

    if ( mnuid==movetotopmnuitem_.id )
    {
	const int nrattribs = visserv->getNrAttribs( displayID() );
	for ( int idx=attribNr(); idx<nrattribs-1; idx++ )
	    visserv->swapAttribs( displayID(), idx, idx+1 );

	moveItemToTop();
	select();
	menu->setIsHandled( true );
	applMgr()->updateColorTable( displayID(), attribNr() );
    }
    else if ( mnuid==movetobottommnuitem_.id )
    {
	for ( int idx=attribNr(); idx; idx-- )
	    visserv->swapAttribs( displayID(), idx, idx-1 );

	moveItem( parent_->lastChild() );
	select();
	menu->setIsHandled( true );
	applMgr()->updateColorTable( displayID(), attribNr() );
    }
    else if ( mnuid==moveupmnuitem_.id )
    {
	const int attribnr = attribNr();
	if ( attribnr<visserv->getNrAttribs( displayID() )-1 )
	{
	    const int targetattribnr = attribnr+1;
	    visserv->swapAttribs( displayID(), attribnr, targetattribnr );
	}

	moveItem( siblingAbove() );
	select();
	menu->setIsHandled(true);
	applMgr()->updateColorTable( displayID(), attribNr() );
    }
    else if ( mnuid==movedownmnuitem_.id )
    {
	const int attribnr = attribNr();
	if ( attribnr )
	{
	    const int targetattribnr = attribnr-1;
	    visserv->swapAttribs( displayID(), attribnr, targetattribnr );
	}

	moveItem( siblingBelow() );
	select();
	menu->setIsHandled( true );
	applMgr()->updateColorTable( displayID(), attribNr() );
    }
    else if ( mnuid==changetransparencyitem_.id )
    {
	visserv->showAttribTransparencyDlg( displayID(), attribNr() );
	menu->setIsHandled( true );
    }
    else if ( mnuid==statisticsitem_.id || mnuid==amplspectrumitem_.id
	      || mnuid==fkspectrumitem_.id )
    {
	const int visid = displayID();
	const int attribid = attribNr();
	DataPack::ID dpid = visserv->getDataPackID( visid, attribid );
	const DataPackMgr::ID dmid = visserv->getDataPackMgrID( visid );

	const Attrib::SelSpec* as = visserv->getSelSpec( visid, attribid );
	const FixedString dpname = DPM(DataPackMgr::FlatID()).nameOf( dpid );
	if ( as && dpname != as->userRef() )
	{
	    const int nrpacks = DPM(DataPackMgr::FlatID()).packs().size();
	    for ( int idx=0; idx<nrpacks; idx++ )
	    {
		const int tmpdtpackid =
				DPM(DataPackMgr::FlatID()).packs()[idx]->id();
		const FixedString tmpnm =
				DPM(DataPackMgr::FlatID()).nameOf(tmpdtpackid);
		if ( tmpnm == as->userRef() )
		{
		    dpid = tmpdtpackid;
		    break;
		}
	    }
	}
	if ( mnuid==statisticsitem_.id )
	{
	    uiStatsDisplay::Setup su; su.countinplot( false );
	    delete statswin_;
	    statswin_ =
		new uiStatsDisplayWin( applMgr()->applService().parent(), su,
				       1, false );
	    statswin_->statsDisplay()->setDataPackID( dpid, dmid );
	    statswin_->setDataName( DPM(dmid).nameOf(dpid)  );
	    statswin_->show();
	    menu->setIsHandled( true );
	}
	else if ( mnuid==amplspectrumitem_.id || mnuid==fkspectrumitem_.id )
	{
	    const bool isselmodeon = visserv->isSelectionModeOn();
	    if ( !isselmodeon )
	    {
		if ( mnuid==amplspectrumitem_.id )
		{
		    delete ampspectrumwin_;
		    ampspectrumwin_ =
			new uiAmplSpectrum( applMgr()->applService().parent() );
		    ampspectrumwin_->setDataPackID( dpid, dmid );
		    ampspectrumwin_->show();
		}
		else
		{
		    fkspectrumwin_ =
			new uiFKSpectrum( applMgr()->applService().parent() );
		    fkspectrumwin_->setDataPackID( dpid, dmid );
		    fkspectrumwin_->show();
		}
	    }

	    menu->setIsHandled( true );
	}
    }
    else if ( mnuid==view2dwvaitem_.id || mnuid==view2dvditem_.id )
    {
	ODMainWin()->viewer2DMgr().displayIn2DViewer( displayID(), attribNr(),
						   mnuid==view2dwvaitem_.id );
	menu->setIsHandled( true );
    }
    else if ( mnuid==removemnuitem_.id )
    {
	const int attribnr = attribNr();
	visserv->removeAttrib( displayID(), attribnr );
	applMgr()->updateColorTable( displayID(), attribnr ? attribnr-1 : 0 );

	prepareForShutdown();
	parent_->removeChild( this );
	menu->setIsHandled( true );
    }
}


void uiODDataTreeItem::prepareForShutdown()
{
    uiTreeItem::prepareForShutdown();
    applMgr()->updateColorTable( -1, -1 );
}


void uiODDataTreeItem::updateColumnText( int col )
{
    if ( col==uiODSceneMgr::cNameColumn() )
	name_ = createDisplayName();

    uiTreeItem::updateColumnText( col );
}


void uiODDataTreeItem::displayMiniCtab( const ColTab::Sequence* seq )
{
    if ( !seq )
    {
	uiTreeItem::updateColumnText( uiODSceneMgr::cColorColumn() );
	return;
    }

    PtrMan<ioPixmap> pixmap = new ioPixmap( *seq, cPixmapWidth(),
					    cPixmapHeight(), true );
    uitreeviewitem_->setPixmap( uiODSceneMgr::cColorColumn(), *pixmap );
}
