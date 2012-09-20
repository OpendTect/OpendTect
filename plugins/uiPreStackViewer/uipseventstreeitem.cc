/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          June 2010
________________________________________________________________________

-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "uipseventstreeitem.h"

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


PSEventsParentTreeItem::PSEventsParentTreeItem()
    : uiODTreeItem("PreStackEvents")
    , child_(0)
{}


PSEventsParentTreeItem::~PSEventsParentTreeItem()
{}


bool PSEventsParentTreeItem::showSubMenu()
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

	child_ = new PSEventsTreeItem( key, eventname );
	addChild( child_, true ); 
    }

    handleStandardItems( mnusel );
    return true;
}


bool PSEventsParentTreeItem::loadPSEvent( MultiID& key,BufferString& eventname )
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


int PSEventsParentTreeItem::sceneID() const
{
    int sceneid;
    if ( !getProperty<int>(uiODTreeTop::sceneidkey(),sceneid) )
	return -1;
    return sceneid;
}


bool PSEventsParentTreeItem::init()
{
    bool ret = uiTreeItem::init();
    if ( !ret ) return false;
    
    return true;
}


const char* PSEventsParentTreeItem::parentType() const
{ return typeid(uiODTreeTop).name(); }


// Child Item

PSEventsTreeItem::PSEventsTreeItem( MultiID key, const char* eventname )
    : key_(key)
    , psem_(*new PreStack::EventManager) 
    , eventname_(eventname)
    , eventdisplay_(0)
    , dir_(Coord(1,0))
    , scalefactor_(1)
    , coloritem_(new MenuItem("Colors"))
    , clridx_(0)
    , dispidx_(0)
{
    psem_.setStorageID( key, true );
}


PSEventsTreeItem::~PSEventsTreeItem()
{
    ColTab::Sequence* cseq = const_cast<ColTab::Sequence*>( 
	   &ODMainWin()->colTabEd().getColTabSequence() );
    if ( cseq )
    {
	cseq->colorChanged.remove(
			mCB(this,PSEventsTreeItem,coltabChangeCB) );
    }

    if ( eventdisplay_ )
    {
	uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
	visserv->removeObject( displayid_, sceneID() );
	eventdisplay_->unRef();
	eventdisplay_ = 0;
    }
}


bool PSEventsTreeItem::init()
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

void PSEventsTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODDisplayTreeItem::createMenu( menu, istb );
    if ( !eventdisplay_ || !menu || menu->menuID()!=displayID() )
	return;

    mAddMenuItem( menu, coloritem_, true, false );
    BufferStringSet items;
    mAddPSMenuItems( coloritem_, markerColorNames, clridx_ )
    if ( eventdisplay_->hasParents() )
    {
	mAddMenuItem( menu, &displaymnuitem_, true, false );
	MenuItem* item = &displaymnuitem_;
	mAddPSMenuItems( item, displayModeNames, dispidx_ )
    }
}


void PSEventsTreeItem::handleMenuCB( CallBacker* cb )
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
	clridx_ = coloritem_->itemIndex( menuid );
	updateColorMode( clridx_ );
	menu->setIsHandled( true );
    }

}


void PSEventsTreeItem::updateColorMode( int mode )
{
    eventdisplay_->setMarkerColor(
	(visSurvey::PSEventDisplay::MarkerColor) mode );
    updateColumnText( uiODSceneMgr::cColorColumn() );
}


void PSEventsTreeItem::updateDisplay()
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
			    mCB(this,PSEventsTreeItem,coltabChangeCB) );
	    eventdisplay_->setColTabSequence( *cseq );
	}
    }
}


bool PSEventsTreeItem::anyButtonClick( uiTreeViewItem* lvm )
{
    applMgr()->updateColorTable( displayid_, 0 );
    displayMiniColTab();
    return true;
}


void PSEventsTreeItem::coltabChangeCB( CallBacker* cb )
{
    displayMiniColTab();
}


void PSEventsTreeItem::updateColumnText( int col )
{
    uiODDisplayTreeItem::updateColumnText( col );
    displayMiniColTab();
}


void PSEventsTreeItem::displayMiniColTab()
{
    if ( eventdisplay_->getMarkerColor() == visSurvey::PSEventDisplay::Single )
	return;

    const ColTab::Sequence* seq = eventdisplay_->getColTabSequence();
    if ( !seq )
	return;
    ioPixmap pixmap( *seq, cPixmapWidth(), cPixmapHeight(), true );
    uitreeviewitem_->setPixmap( uiODSceneMgr::cColorColumn(), pixmap );
}


