/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          June 2010
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uipseventstreeitem.cc,v 1.5 2011-11-16 04:55:51 cvsranojay Exp $";

#include "uipseventstreeitem.h"

#include "ctxtioobj.h"
#include "enums.h"
#include "menuhandler.h"
#include "prestackevents.h"
#include "prestackeventtransl.h"
#include "prestackeventio.h"
#include "ptrman.h"
#include "survinfo.h"
#include "uiioobjsel.h"
#include "uilistview.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodmain.h"
#include "uiodapplmgr.h"
#include "uipseventspropdlg.h"
#include "uitaskrunner.h"
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
    , zerooffset_(new MenuItem("Zero offset"))
    , sticksfromsection_(new MenuItem("Sticks from section"))
    , zerooffsetonsection_(new MenuItem("Zero offset on section"))
    , properties_(new MenuItem("Properties"))
{
    psem_.setStorageID( key, true );
}


PSEventsTreeItem::~PSEventsTreeItem()
{
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


void PSEventsTreeItem::createMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::createMenuCB(cb);
    mDynamicCastGet(MenuHandler*,menu,cb);
    if ( !eventdisplay_ || menu->menuID()!=displayID() )
	return;

   mAddMenuItem( menu, &displaymnuitem_, true, false );
   mAddMenuItem( &displaymnuitem_, zerooffset_, true, false );
   mAddMenuItem( &displaymnuitem_, sticksfromsection_, true, false );
   mAddMenuItem( &displaymnuitem_, zerooffsetonsection_, true, false );
   mAddMenuItem( &displaymnuitem_, properties_, true, false );
}


void PSEventsTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, menuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( !eventdisplay_ || menu->isHandled() || menu->menuID()!=displayID() || menuid==-1 )
	return;

    if ( menuid == zerooffset_->id  )
    {
	menu->setIsHandled(true);
	eventdisplay_->setDisplayMode(
	    visSurvey::PSEventDisplay::ZeroOffset );
    }
    else if ( menuid == sticksfromsection_->id )
    {
	menu->setIsHandled(true);
	eventdisplay_->setDisplayMode(
	    visSurvey::PSEventDisplay::FullOnSections );
    }
    else if ( menuid == zerooffsetonsection_->id )
    {
	menu->setIsHandled(true);
	eventdisplay_->setDisplayMode(
	    visSurvey::PSEventDisplay::ZeroOffsetOnSections );
    }
    else if ( menuid == properties_->id )
    {
	menu->setIsHandled(true);
	uiPSEventsPropertyDlg dlg( getUiParent(), this,
	    eventdisplay_->markerColorNames() );
	dlg.go();
    }

}


void PSEventsTreeItem::updateColorMode( int mode )
{
    eventdisplay_->setMarkerColor(
	(visSurvey::PSEventDisplay::MarkerColor) mode );
}


void PSEventsTreeItem::updateDisplay()
{
    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    if ( !eventdisplay_ )
    {
	eventdisplay_ = visSurvey::PSEventDisplay::create();
	eventdisplay_->ref();
	visserv->addObject( eventdisplay_, sceneID(), false );
	displayid_ = eventdisplay_->id();
	eventdisplay_->setName( eventname_ );
	eventdisplay_->setEventManager( &psem_ );
    }
}



