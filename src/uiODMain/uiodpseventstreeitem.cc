/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodpseventstreeitem.h"

#include "uiioobjseldlg.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodmain.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uitaskrunner.h"
#include "uitreeview.h"
#include "uiviscoltabed.h"
#include "uivispartserv.h"
#include "vispseventdisplay.h"

#include "ctxtioobj.h"
#include "enums.h"
#include "menuhandler.h"
#include "prestackevents.h"
#include "prestackeventtransl.h"
#include "prestackeventio.h"
#include "ptrman.h"
#include "survinfo.h"


CNotifier<uiODPSEventsParentTreeItem,uiMenu*>&
	uiODPSEventsParentTreeItem::showMenuNotifier()
{
    static CNotifier<uiODPSEventsParentTreeItem,uiMenu*> notif( nullptr );
    return notif;
}


uiODPSEventsParentTreeItem::uiODPSEventsParentTreeItem()
    : uiODParentTreeItem( uiStrings::sPreStackEvents() )
    , child_(0)
{}


uiODPSEventsParentTreeItem::~uiODPSEventsParentTreeItem()
{}


bool uiODPSEventsParentTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertAction( new uiAction(m3Dots(uiStrings::sAdd())), 0 );
    showMenuNotifier().trigger( &mnu, this );

    addStandardItems( mnu );

    const int mnusel = mnu.exec();
    if ( mnusel == 0 )
    {
	BufferString eventname;
	MultiID key;
	if ( !loadPSEvent(key,eventname) )
	    return false;

	child_ = new uiODPSEventsTreeItem( key, eventname );
	addChild( child_, true );
    }

    handleStandardItems( mnusel );
    return true;
}


bool uiODPSEventsParentTreeItem::loadPSEvent( MultiID& key,
					      BufferString& eventname )
{
    CtxtIOObj context = PSEventTranslatorGroup::ioContext();
    context.ctxt_.forread_ = true;
    uiIOObjSelDlg dlg( getUiParent(), context );
    if ( !dlg.go() )
	return false;

    eventname = dlg.ioObj()->name();
    key = dlg.chosenID();
    if ( key.isUdf() || eventname.isEmpty() )
    {
	uiString errmsg = tr("Failed to load prestack event");
	uiMSG().error( errmsg );
	return false;
    }

    return true;
}


SceneID uiODPSEventsParentTreeItem::sceneID() const
{
    int sceneid;
    if ( !getProperty<int>(uiODTreeTop::sceneidkey(),sceneid) )
	return SceneID::udf();

    return SceneID(sceneid);
}


bool uiODPSEventsParentTreeItem::init()
{
    bool ret = uiODTreeItem::init();
    if ( !ret ) return false;

    return true;
}


const char* uiODPSEventsParentTreeItem::iconName() const
{ return "tree-psevents"; }


const char* uiODPSEventsParentTreeItem::parentType() const
{ return typeid(uiODTreeTop).name(); }



// uiODPSEventsTreeItem

uiODPSEventsTreeItem::uiODPSEventsTreeItem( const MultiID& key,
					    const char* eventname )
    : key_(key)
    , psem_(*new PreStack::EventManager)
    , eventname_(eventname)
    , eventdisplay_(0)
    , dir_(Coord(1,0))
    , scalefactor_(1)
    , coloritem_(new MenuItem(uiStrings::sColor(mPlural)))
    , coloridx_(0)
    , dispidx_(0)
{
    psem_.setStorageID( key, true );
}


uiODPSEventsTreeItem::~uiODPSEventsTreeItem()
{
    ColTab::Sequence* cseq = const_cast<ColTab::Sequence*>(
	   &ODMainWin()->colTabEd().getColTabSequence() );
    if ( cseq )
    {
	cseq->colorChanged.remove(
			mCB(this,uiODPSEventsTreeItem,coltabChangeCB) );
    }

    if ( eventdisplay_ )
    {
	uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
	visserv->removeObject( displayid_, sceneID() );
	eventdisplay_->unRef();
	eventdisplay_ = 0;
    }
}


bool uiODPSEventsTreeItem::init()
{
    updateDisplay();
    eventdisplay_->setDisplayMode( visSurvey::PSEventDisplay::ZeroOffset );
    return uiODDisplayTreeItem::init();
}


#define mAddPSMenuItems( mnu, func, midx, enab ) \
    mnu->removeItems(); \
    items.setEmpty(); \
    items.add( eventdisplay_->func() ); \
    if ( items.isEmpty() ) return; \
    mnu->createItems( items ); \
    for ( int idx=0; idx<items.size(); idx++ ) \
    {  mnu->getItem(idx)->checkable = true; \
       mnu->getItem(idx)->enabled = !idx || enab; \
    } \
    mnu->getItem(midx)->checked = true; \
    items.erase(); \

void uiODPSEventsTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODDisplayTreeItem::createMenu( menu, istb );
    if ( istb || !eventdisplay_ || !menu || !isDisplayID(menu->menuID()) )
	return;

    mAddMenuItem( menu, coloritem_, true, false );
    BufferStringSet items;
    mAddPSMenuItems( coloritem_, markerColorNames, coloridx_, true );
    mAddMenuItem( menu, &displaymnuitem_, true, false );
    MenuItem* item = &displaymnuitem_;
    const bool enabled = eventdisplay_->supportsDisplay();
    mAddPSMenuItems( item, displayModeNames, dispidx_, enabled )
}


void uiODPSEventsTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, menuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( !eventdisplay_ || menu->isHandled()
	|| !isDisplayID(menu->menuID()) || menuid==-1 )
	return;

    if ( displaymnuitem_.id != -1 && displaymnuitem_.itemIndex(menuid) != -1 )
    {
	dispidx_ = displaymnuitem_.itemIndex( menuid );
	MouseCursorChanger cursorchanger( MouseCursor::Wait );
	eventdisplay_->setDisplayMode(
	    (visSurvey::PSEventDisplay::DisplayMode) dispidx_ );
	menu->setIsHandled( true );
    }
    else if ( coloritem_->id!=-1 && coloritem_->itemIndex(menuid)!=-1 )
    {
	coloridx_ = coloritem_->itemIndex( menuid );
	MouseCursorChanger cursorchanger( MouseCursor::Wait );
	updateColorMode( coloridx_ );
	menu->setIsHandled( true );
    }
}


void uiODPSEventsTreeItem::updateColorMode( int mode )
{
    eventdisplay_->setMarkerColor(
	(visSurvey::PSEventDisplay::MarkerColor) mode );
    updateColumnText( uiODSceneMgr::cColorColumn() );
}


void uiODPSEventsTreeItem::updateDisplay()
{
    if ( !eventdisplay_ )
    {
	eventdisplay_ = new visSurvey::PSEventDisplay;
	eventdisplay_->ref();
        uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
	visserv->addObject( eventdisplay_, sceneID(), false );
	displayid_ = eventdisplay_->id();
	eventdisplay_->setName( eventname_ );
	eventdisplay_->setLineStyle( OD::LineStyle(OD::LineStyle::Solid,4) );
	eventdisplay_->setEventManager( &psem_ );

	ColTab::Sequence* cseq = const_cast<ColTab::Sequence*>(
	   &ODMainWin()->colTabEd().getColTabSequence() );
	if ( cseq )
	{
	    cseq->colorChanged.notify(
			    mCB(this,uiODPSEventsTreeItem,coltabChangeCB) );
	    eventdisplay_->setColTabSequence( *cseq );
	}
    }
}


bool uiODPSEventsTreeItem::anyButtonClick( uiTreeViewItem* )
{
    applMgr()->updateColorTable( displayid_, 0 );
    displayMiniColTab();
    return true;
}


void uiODPSEventsTreeItem::coltabChangeCB( CallBacker* )
{
    displayMiniColTab();
}


void uiODPSEventsTreeItem::updateColumnText( int col )
{
    uiODDisplayTreeItem::updateColumnText( col );
    displayMiniColTab();
}


void uiODPSEventsTreeItem::displayMiniColTab()
{
    if ( eventdisplay_->getMarkerColor() == visSurvey::PSEventDisplay::Single )
	return;

    const ColTab::Sequence* seq = eventdisplay_->getColTabSequence();
    if ( !seq ) return;

    uitreeviewitem_->setPixmap( uiODSceneMgr::cColorColumn(), *seq );
}
