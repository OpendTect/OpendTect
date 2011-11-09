/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          June 2010
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uipseventstreeitem.cc,v 1.3 2011-11-09 04:42:23 cvsranojay Exp $";

#include "uipseventstreeitem.h"

#include "binidvalset.h"
#include "ctxtioobj.h"
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
#include "uitaskrunner.h"
#include "uivispartserv.h"
#include "visseedpolyline.h"


PSEventsParentTreeItem::PSEventsParentTreeItem()
    : uiODTreeItem("PreStackEvents")
    , child_(0)
    , psem_(new PreStack::EventManager)
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
	if ( !loadPSEvent(eventname) )
	    return false;

	child_ = new PSEventsTreeItem( *psem_, eventname );
	addChild( child_, true ); 
    }

    handleStandardItems( mnusel );
    return true;
}


bool PSEventsParentTreeItem::loadPSEvent( BufferString& eventname )
{
    CtxtIOObj context = PSEventTranslatorGroup::ioContext();
    context.ctxt.forread = true;
    uiIOObjSelDlg dlg(  getUiParent(), context,
			"Select prestack events", false );
    if ( !dlg.go() )
	return false;

    eventname = dlg.ioObj()->name();
    const MultiID& key = dlg.selected( 0 );
    if ( !psem_->setStorageID(key,true) )
    {
	 BufferString errmsg = "Failed to load prestack event \"";
	 errmsg += eventname;
	 errmsg += "\"";
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

PSEventsTreeItem::PSEventsTreeItem( const PreStack::EventManager& psem,
					const char* eventname )
    : psem_(psem)
    , eventname_(eventname)
    , eventlinedisplay_(0)
    , sticksfromsection_(new MenuItem("Sticks from section"))
    , zerooffset_(new MenuItem("Zero offset"))
    , properties_(new MenuItem("Properties"))
{}


PSEventsTreeItem::~PSEventsTreeItem()
{
    if ( eventlinedisplay_ )
    {
	uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
	visserv->removeObject( displayid_, sceneID() );
	eventlinedisplay_->unRef();
	eventlinedisplay_ = 0;
    }
}


bool PSEventsTreeItem::init()
{
    updateDisplay();
    return uiODDisplayTreeItem::init();
}

#define mADDDisplayMenuItems( name )\
    mAddMenuItem( &displaymnuitem_, new MenuItem( name ), true, false ); \

void PSEventsTreeItem::createMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::createMenuCB(cb);
    mDynamicCastGet(MenuHandler*,menu,cb);
    if ( !eventlinedisplay_ || menu->menuID()!=displayID() )
	return;

   mAddMenuItem( menu, &displaymnuitem_, true, false );
   mAddMenuItem( &displaymnuitem_, sticksfromsection_, true, false );
   mAddMenuItem( &displaymnuitem_, zerooffset_, true, false );
   mAddMenuItem( &displaymnuitem_, properties_, true, false );
}


void PSEventsTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, menuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( menu->isHandled() || menu->menuID()!=displayID() || menuid==-1 )
	return;

    if ( menuid == sticksfromsection_->id )
    {
	menu->setIsHandled(true);
	uiMSG().message( "To be implemented" );
    }
    else if ( menuid == zerooffset_->id )
    {
	menu->setIsHandled(true);
	uiMSG().message( "To be implemented" );
    }
    else if ( menuid == properties_->id )
    {
	menu->setIsHandled(true);
	uiMSG().message( "To be implemented" );
    }

}


void PSEventsTreeItem::updateDisplay()
{
    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    if ( !eventlinedisplay_ )
    {
	eventlinedisplay_ = visSurvey::visSeedPolyLine::create();
	eventlinedisplay_->ref();
    }

    eventlinedisplay_->setName( eventname_ );
    visserv->addObject( eventlinedisplay_, sceneID(), false );
    displayid_ = eventlinedisplay_->id();
    BinIDValueSet locations( 0, false );
    psem_.getLocations( locations );
    TypeSet<Coord3> finalcoords;
    TypeSet<int> cii;
    int ci = 0;
    for ( int lidx=0; lidx<locations.totalSize(); lidx++ )
    {
	const BinID bid = locations.getBinID( locations.getPos(lidx) );
	RefMan<const PreStack::EventSet> eventset = psem_.getEvents( bid, true );
	if ( !eventset )
	    return eventlinedisplay_->clearDisplay();

	const int size = eventset->events_.size();
	eventlinedisplay_->setLineColor( psem_.getColor() );
	for ( int idx=0; idx<size; idx++ )
	{
	    const PreStack::Event* psevent = eventset->events_[idx];
	    if ( !psevent->sz_ )
		continue;
	    const int sz = psevent->sz_;
	    TypeSet<Coord3> coords;
	    TypeSet<float> offsets;
	    for ( int idy=0; idy<sz; idy++ )
	    {
		const float offset = psevent->offsetazimuth_[idy].offset();
		offsets += offset;
		Coord3 pos( bid.inl, bid.crl, psevent->pick_[idy] );
		const Coord dir( 1, 0 );
		const Coord offs = dir * offset;
		pos.x += offs.x / SI().inlDistance();
		pos.y += offs.y / SI().crlDistance();
		coords += pos;
		cii += ci++;
	    }

	    sort_coupled( offsets.arr(), coords.arr(), sz );
	    finalcoords.append( coords );
	    cii += -1;
	}

    }

    eventlinedisplay_->updateCoords( cii, finalcoords );
}



