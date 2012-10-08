/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          June 2010
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uiodpseventstreeitem.h"

#include "ctxtioobj.h"
#include "enums.h"
#include "menuhandler.h"
#include "pixmap.h"
#include "prestackevents.h"
#include "prestackeventtransl.h"
#include "prestackeventio.h"
#include "ptrman.h"
#include "survinfo.h"
#include "uiioobjsel.h"
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


uiODPSEventsParentTreeItem::uiODPSEventsParentTreeItem()
    : uiODTreeItem("PreStackEvents")
    , child_(0)
{}


uiODPSEventsParentTreeItem::~uiODPSEventsParentTreeItem()
{}


bool uiODPSEventsParentTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("Add ..."), 0 );
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
    context.ctxt.forread = true;
    uiIOObjSelDlg dlg(  getUiParent(), context,
			"Select prestack events", false );
    if ( !dlg.go() )
	return false;

    eventname = dlg.ioObj()->name();
    key = dlg.selected( 0 );
    if ( key.isEmpty() || eventname.isEmpty() )
    {
	BufferString errmsg = "Failed to load prestack event";
	uiMSG().error( errmsg ); 
	return false;
    }

    return true;
}


int uiODPSEventsParentTreeItem::sceneID() const
{
    int sceneid;
    if ( !getProperty<int>(uiODTreeTop::sceneidkey(),sceneid) )
	return -1;
    return sceneid;
}


bool uiODPSEventsParentTreeItem::init()
{
    bool ret = uiTreeItem::init();
    if ( !ret ) return false;
    
    return true;
}


const char* uiODPSEventsParentTreeItem::parentType() const
{ return typeid(uiODTreeTop).name(); }



// uiODPSEventsTreeItem

#define mPixmapWidth	16
#define mPixmapHeight	10

uiODPSEventsTreeItem::uiODPSEventsTreeItem( const MultiID& key,
					    const char* eventname )
    : key_(key)
    , psem_(*new PreStack::EventManager) 
    , eventname_(eventname)
    , eventdisplay_(0)
    , dir_(Coord(1,0))
    , scalefactor_(1)
    , coloritem_(new MenuItem("Colors"))
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


#define mAddPSMenuItems( mnu, func, midx ) \
    mnu->removeItems(); \
    items = eventdisplay_->func(); \
    if ( items.isEmpty() ) return; \
    mnu->createItems( items ); \
    for ( int idx=0; idx<items.size(); idx++ ) \
    {  mnu->getItem(idx)->checkable = true; } \
    mnu->getItem(midx)->checked = true; \
    items.erase(); \

void uiODPSEventsTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODDisplayTreeItem::createMenu( menu, istb );
    if ( !eventdisplay_ || !menu || menu->menuID()!=displayID() )
	return;

    mAddMenuItem( menu, coloritem_, true, false );
    BufferStringSet items;
    mAddPSMenuItems( coloritem_, markerColorNames, coloridx_ )
    if ( eventdisplay_->hasParents() )
    {
	mAddMenuItem( menu, &displaymnuitem_, true, false );
	MenuItem* item = &displaymnuitem_;
	mAddPSMenuItems( item, displayModeNames, dispidx_ )
    }
}


void uiODPSEventsTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, menuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( !eventdisplay_ || menu->isHandled()
	|| menu->menuID()!=displayID() || menuid==-1 )
	return;

    if ( displaymnuitem_.id != -1 && displaymnuitem_.itemIndex(menuid) != -1 )
    {
	dispidx_ = displaymnuitem_.itemIndex( menuid );
	eventdisplay_->setDisplayMode(
	    (visSurvey::PSEventDisplay::DisplayMode) dispidx_ );
	menu->setIsHandled( true );
    }
    else if ( coloritem_->id!=-1 && coloritem_->itemIndex(menuid)!=-1 )
    {
	coloridx_ = coloritem_->itemIndex( menuid );
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
	eventdisplay_ = visSurvey::PSEventDisplay::create();
	eventdisplay_->ref();
        uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
	visserv->addObject( eventdisplay_, sceneID(), false );
	displayid_ = eventdisplay_->id();
	eventdisplay_->setName( eventname_ );
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


bool uiODPSEventsTreeItem::anyButtonClick( uiTreeViewItem* lvm )
{
    applMgr()->updateColorTable( displayid_, 0 );
    displayMiniColTab();
    return true;
}


void uiODPSEventsTreeItem::coltabChangeCB( CallBacker* cb )
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
    if ( !seq )
	return;
    ioPixmap pixmap( *seq, mPixmapWidth, mPixmapHeight, true );
    uitreeviewitem_->setPixmap( uiODSceneMgr::cColorColumn(), pixmap );
}

