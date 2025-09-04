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

#include "ctxtioobj.h"
#include "enums.h"
#include "menuhandler.h"
#include "prestackeventtransl.h"
#include "prestackeventio.h"
#include "survinfo.h"


CNotifier<uiODPSEventsParentTreeItem,uiMenu*>&
	uiODPSEventsParentTreeItem::showMenuNotifier()
{
    static CNotifier<uiODPSEventsParentTreeItem,uiMenu*> notif( nullptr );
    return notif;
}


uiTreeItem* uiODPSEventsTreeItemFactory::create() const
{
    return new uiODPSEventsParentTreeItem;
}


uiTreeItem* uiODPSEventsTreeItemFactory::createForVis(
	const VisID& visid, uiTreeItem* itm ) const
{
    return nullptr;
}



uiODPSEventsParentTreeItem::uiODPSEventsParentTreeItem()
    : uiODParentTreeItem( uiStrings::sPreStackEvents() )
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
    IOObjContext context = PSEventTranslatorGroup::ioContext();
    context.forread_ = true;
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
    int sceneid = SceneID::udf().asInt();
    if ( !getProperty<int>(uiODTreeTop::sceneidkey(),sceneid) )
	return SceneID::udf();

    return SceneID( sceneid );
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
    , psem_(new PreStack::EventManager)
    , eventname_(eventname)
    , dir_(Coord(1,0))
    , coloritem_(new MenuItem(uiStrings::sColor(mPlural)))
{
    psem_->setStorageID( key, true );
}


uiODPSEventsTreeItem::~uiODPSEventsTreeItem()
{
    detachAllNotifiers();
    visserv_->removeObject( displayid_, sceneID() );
}


bool uiODPSEventsTreeItem::init()
{
    updateDisplay();
    RefMan<visSurvey::PSEventDisplay> eventdisplay = getDisplay();
    if ( eventdisplay )
	eventdisplay->setDisplayMode( visSurvey::PSEventDisplay::ZeroOffset );

    return uiODDisplayTreeItem::init();
}


#define mAddPSMenuItems( mnu, func, midx, enab ) \
    mnu->removeItems(); \
    items.setEmpty(); \
    items.add( eventdisplay->func() ); \
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
    ConstRefMan<visSurvey::PSEventDisplay> eventdisplay = getDisplay();
    if ( istb || !eventdisplay || !menu || !isDisplayID(menu->menuID()) )
	return;

    mAddMenuItem( menu, coloritem_, true, false );
    BufferStringSet items;
    mAddPSMenuItems( coloritem_, markerColorNames, coloridx_, true );
    mAddMenuItem( menu, &displaymnuitem_, true, false );
    MenuItem* item = &displaymnuitem_;
    const bool enabled = eventdisplay->supportsDisplay();
    mAddPSMenuItems( item, displayModeNames, dispidx_, enabled )
}


void uiODPSEventsTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, menuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
    RefMan<visSurvey::PSEventDisplay> eventdisplay = getDisplay();
    if ( !eventdisplay || menu->isHandled() ||
	 !isDisplayID(menu->menuID()) || menuid==-1 )
	return;

    if ( displaymnuitem_.id != -1 && displaymnuitem_.itemIndex(menuid) != -1 )
    {
	dispidx_ = displaymnuitem_.itemIndex( menuid );
	MouseCursorChanger cursorchanger( MouseCursor::Wait );
	eventdisplay->setDisplayMode(
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
    RefMan<visSurvey::PSEventDisplay> eventdisplay = getDisplay();
    if ( !eventdisplay )
	return;

    eventdisplay->setMarkerColor(
			(visSurvey::PSEventDisplay::MarkerColor) mode );
    updateColumnText( uiODSceneMgr::cColorColumn() );
}


void uiODPSEventsTreeItem::updateDisplay()
{
    if ( !displayid_.isValid() )
    {
	RefMan<visSurvey::PSEventDisplay> eventdisplay =
						new visSurvey::PSEventDisplay;
	visserv_->addObject( eventdisplay.ptr(), sceneID(), false);
	displayid_ = eventdisplay->id();
	eventdisplay->setName( eventname_ );
	eventdisplay->setLineStyle( OD::LineStyle(OD::LineStyle::Solid,4) );
	eventdisplay->setEventManager( psem_.ptr() );

	auto* cseq = const_cast<ColTab::Sequence*>(
		       &ODMainWin()->colTabEd().getColTabSequence() );
	if ( cseq )
	{
	    mAttachCB( cseq->colorChanged,uiODPSEventsTreeItem::coltabChangeCB);
	    eventdisplay->setColTabSequence( *cseq );
	}
    }

    RefMan<visSurvey::PSEventDisplay> eventdisplay =
	dCast(visSurvey::PSEventDisplay*,visserv_->getObject(displayid_));
    eventdisplay_ = eventdisplay;
}


ConstRefMan<visSurvey::PSEventDisplay> uiODPSEventsTreeItem::getDisplay() const
{
    return eventdisplay_.get();
}


RefMan<visSurvey::PSEventDisplay> uiODPSEventsTreeItem::getDisplay()
{
    return eventdisplay_.get();
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
    ConstRefMan<visSurvey::PSEventDisplay> eventdisplay = getDisplay();
    if ( !eventdisplay )
	return;

    if ( eventdisplay->getMarkerColor() == visSurvey::PSEventDisplay::Single )
	return;

    const ColTab::Sequence* seq = eventdisplay->getColTabSequence();
    if ( !seq )
	return;

    uitreeviewitem_->setPixmap( uiODSceneMgr::cColorColumn(), *seq );
}
