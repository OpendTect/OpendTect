/*+
___________________________________________________________________

 CopyRight: 	(C) dGB Beheer B.V.
 Author: 	K. Tingdahl
 Date: 		May 2006
 RCS:		$Id: uiodwelltreeitem.cc,v 1.20 2008-04-10 05:24:11 cvssatyaki Exp $
___________________________________________________________________

-*/

#include "uiodwelltreeitem.h"

#include "uimenu.h"
#include "uiodapplmgr.h"
#include "uivispartserv.h"

#include "draw.h"

#include "uiattribpartserv.h"
#include "uicreateattriblogdlg.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uiodscenemgr.h"
#include "uiwellattribpartserv.h"
#include "uiwellpartserv.h"
#include "uiwellpropdlg.h"
#include "mousecursor.h"

#include "viswelldisplay.h"


uiODWellParentTreeItem::uiODWellParentTreeItem()
    : uiODTreeItem( "Well" )
{
}


bool uiODWellParentTreeItem::showSubMenu()
{
    mDynamicCastGet(visSurvey::Scene*,scene,
	    	    ODMainWin()->applMgr().visServer()->getObject(sceneID()));
    if ( scene && scene->getDataTransform() )
    {
	uiMSG().message( "Cannot add Wells to this scene" );
	return false;
    }

    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("&Load ..."), 0 );
    mnu.insertItem( new uiMenuItem("&New WellTrack ..."), 1 );
    if ( children_.size() > 1 )
    {
	mnu.insertItem( new uiMenuItem( "Create Attribute Log ..." ), 2 );
	mnu.insertItem( new uiMenuItem("&Properties ..."), 3 );

	mnu.insertSeparator( 30 );
	uiPopupMenu* showmnu = new uiPopupMenu( getUiParent(), "&Show all" );
	showmnu->insertItem( new uiMenuItem("Well names (&Top)"), 31 );
	showmnu->insertItem( new uiMenuItem("Well names (&Bottom)"), 32 );
	showmnu->insertItem( new uiMenuItem("&Markers"), 33 );
	showmnu->insertItem( new uiMenuItem("Marker &Names"), 34 );
	showmnu->insertItem( new uiMenuItem("&Logs"), 35 );
	mnu.insertItem( showmnu );

	uiPopupMenu* hidemnu = new uiPopupMenu( getUiParent(), "&Hide all" );
	hidemnu->insertItem( new uiMenuItem("Well names (&Top)"), 41 );
	hidemnu->insertItem( new uiMenuItem("Well names (&Bottom)"), 42 );
	hidemnu->insertItem( new uiMenuItem("&Markers"), 43 );
	hidemnu->insertItem( new uiMenuItem("Marker &Names"), 44 );
	hidemnu->insertItem( new uiMenuItem("&Logs"), 45 );
	mnu.insertItem( hidemnu );
    }
    addStandardItems( mnu );

    const int mnuid = mnu.exec();
    return mnuid<0 ? false : handleSubMenu( mnuid );
}


bool uiODWellParentTreeItem::handleSubMenu( int mnuid )
{
    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    if ( mnuid == 0 )
    {
	ObjectSet<MultiID> emwellids;
	applMgr()->selectWells( emwellids );
	if ( emwellids.isEmpty() )
	    return false;

	for ( int idx=emwellids.size()-1; idx>=0; idx-- )
	    addChild( new uiODWellTreeItem(*emwellids[idx]), false );

	deepErase( emwellids );
    }
    else if ( mnuid == 1 )
    {
	visSurvey::WellDisplay* wd = visSurvey::WellDisplay::create();
	wd->setupPicking(true);
	BufferString wellname;
	Color color;
	if ( !applMgr()->wellServer()->setupNewWell(wellname,color) )
	    return false;
	wd->setLineStyle( LineStyle(LineStyle::Solid,1,color) );
	wd->setName( wellname );
	visserv->addObject( wd, sceneID(), true );
	addChild( new uiODWellTreeItem(wd->id()), false );
    }
    else if ( mnuid == 2 )
    {
	BufferStringSet wellnames;
	for ( int idx=0; idx<children_.size(); idx++ )
	    wellnames.add( children_[idx]->name() );

	uiCreateAttribLogDlg dlg( getUiParent(), wellnames,
		    applMgr()->attrServer()->curDescSet(false),
		    ODMainWin()->applMgr().wellAttribServer()->getNLAModel());
	dlg.go();
    }
    else if ( mnuid == 3 )
    {
	TypeSet<int> wdids;
	visserv->findObject( typeid(visSurvey::WellDisplay), wdids );
	ObjectSet<visSurvey::WellDisplay> wds;
	for ( int idx=0; idx<wdids.size(); idx++ )
	{
	    mDynamicCastGet(visSurvey::WellDisplay*,wd,
			    visserv->getObject(wdids[idx]))
	    wds += wd;
	}

	if ( wds.isEmpty() ) return false;

	uiWellPropDlg dlg( getUiParent(), wds );
	dlg.go();

	for ( int idx=0; idx<children_.size(); idx++ )
	{
	    mDynamicCastGet(uiODWellTreeItem*,itm,children_[idx])
	    if ( itm )
		itm->updateColumnText(uiODSceneMgr::cColorColumn());
	}
    }
    else if ( ( mnuid>30 && mnuid<36 ) || ( mnuid>40 && mnuid<46 ) )
    {
	for ( int idx=0; idx<children_.size(); idx++ )
	{
	    mDynamicCastGet(uiODWellTreeItem*,itm,children_[idx]);
	    if ( !itm ) continue;

	    mDynamicCastGet(visSurvey::WellDisplay*,wd,
				    visserv->getObject(itm->displayID()));
	    if ( !wd ) continue;

	    switch ( mnuid )
	    {
		case 31: wd->showWellTopName( true ); break;
		case 32: wd->showWellBotName( true ); break;
		case 33: wd->showMarkers( true ); break;
		case 34: wd->showMarkerName( true ); break;
		case 35: wd->showLogs( true ); break;
		case 41: wd->showWellTopName( false ); break;
		case 42: wd->showWellBotName( false ); break;
		case 43: wd->showMarkers( false ); break;
		case 44: wd->showMarkerName( false ); break;
		case 45: wd->showLogs( false ); break;
	    }
	}
    }
    else
	handleStandardItems( mnuid );

    return true;
}


uiTreeItem* uiODWellTreeItemFactory::create( int visid, uiTreeItem* ) const
{
    mDynamicCastGet(visSurvey::WellDisplay*,wd, 
		    ODMainWin()->applMgr().visServer()->getObject(visid));
    return wd ? new uiODWellTreeItem(visid) : 0;
}


uiODWellTreeItem::uiODWellTreeItem( int did )
{
    displayid_ = did;
    initMenuItems();
}


uiODWellTreeItem::uiODWellTreeItem( const MultiID& mid_ )
{
    mid = mid_;
    initMenuItems();
}


uiODWellTreeItem::~uiODWellTreeItem()
{
}


void uiODWellTreeItem::initMenuItems()
{
    attrmnuitem_.text = "&Create attribute log...";
    sellogmnuitem_.text = "Select logs ...";
    propertiesmnuitem_.text = "&Properties ...";
    nametopmnuitem_.text = "Well name (&Top)";
    namebotmnuitem_.text = "Well name (&Bottom)";
    markermnuitem_.text = "&Markers";
    markernamemnuitem_.text = "Marker &names";
    showlogmnuitem_.text = "&Logs" ;
    showmnuitem_.text = "&Show" ;
    editmnuitem_.text = "&Edit Welltrack" ;
    storemnuitem_.text = "St&ore ...";

    nametopmnuitem_.checkable = true;
    namebotmnuitem_.checkable = true;
    markermnuitem_.checkable = true;
    markernamemnuitem_.checkable = true;
    showlogmnuitem_.checkable = true;
    editmnuitem_.checkable = true;
}


bool uiODWellTreeItem::init()
{
    if ( displayid_==-1 )
    {
	visSurvey::WellDisplay* wd = visSurvey::WellDisplay::create();
	displayid_ = wd->id();
	visserv_->addObject( wd, sceneID(), true );
	if ( !wd->setMultiID(mid) )
	{
	    visserv_->removeObject( wd, sceneID() );
	    return false;
	}
    }
    else
    {
	mDynamicCastGet(visSurvey::WellDisplay*,wd,
			visserv_->getObject(displayid_));
	if ( !wd )
	    return false;

	mDynamicCastGet(visSurvey::Scene*,scene,
			visserv_->getObject(sceneID()));
	if ( scene )
	    wd->setDisplayTransformation( scene->getUTM2DisplayTransform() );
    }

    return uiODDisplayTreeItem::init();
}
	    
	
void uiODWellTreeItem::createMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::createMenuCB(cb);
    mDynamicCastGet(uiMenuHandler*,menu,cb);
    if ( menu->menuID()!=displayID() )
	return;

    mDynamicCastGet(visSurvey::WellDisplay*,wd,visserv_->getObject(displayid_));
    const bool islocked = visserv_->isLocked( displayid_ );
    mAddMenuItem( menu, &sellogmnuitem_, !islocked, false );
    mAddMenuItem( menu, &attrmnuitem_, true, false );
    mAddMenuItem( menu, &propertiesmnuitem_, true, false );
    mAddMenuItem( menu, &editmnuitem_, !islocked, wd->isHomeMadeWell() );
    mAddMenuItem( menu, &storemnuitem_, wd->hasChanged(), false );
    mAddMenuItem( menu, &showmnuitem_, true, false );
    mAddMenuItem( &showmnuitem_, &nametopmnuitem_, true,  
	    					wd->wellTopNameShown() );
    mAddMenuItem( &showmnuitem_, &namebotmnuitem_, true,  
	    					wd->wellBotNameShown() );

    mAddMenuItem( &showmnuitem_, &markermnuitem_, wd->canShowMarkers(),
		 wd->markersShown() );
    mAddMenuItem( &showmnuitem_, &markernamemnuitem_, wd->canShowMarkers(),
		  wd->canShowMarkers() && wd->markerNameShown() );
    mAddMenuItem( &showmnuitem_, &showlogmnuitem_,
		  applMgr()->wellServer()->hasLogs(wd->getMultiID()), 
		  wd->logsShown() );
}


void uiODWellTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(uiMenuHandler*,menu,caller);
    if ( menu->menuID()!=displayID() || mnuid==-1 || menu->isHandled() )
	return;

    mDynamicCastGet(visSurvey::WellDisplay*,wd,visserv_->getObject(displayid_))
    const MultiID& wellid = wd->getMultiID();
    if ( mnuid == attrmnuitem_.id )
    {
	menu->setIsHandled( true );
	//TODO false set to make it compile: change!
	applMgr()->wellAttribServer()->setAttribSet( 
				*applMgr()->attrServer()->curDescSet(false) );
	applMgr()->wellAttribServer()->createAttribLog( wellid );
    }
    else if ( mnuid==sellogmnuitem_.id )
    {
	menu->setIsHandled( true );
	Well::LogDisplayParSet* logparset = wd->getLogParSet();
	if( applMgr()->wellServer()->selectLogs( wellid, logparset ) )
	{
	    wd->displayRightLog();
	    wd->displayLeftLog();
	}
    }
    else if ( mnuid == propertiesmnuitem_.id )
    {
	menu->setIsHandled( true );
	uiWellPropDlg dlg( getUiParent(), wd );
	dlg.go();
	updateColumnText( uiODSceneMgr::cColorColumn() );
    }
    else if ( mnuid == nametopmnuitem_.id )
    {
	menu->setIsHandled( true );
	wd->showWellTopName( !wd->wellTopNameShown() );
    }
    else if ( mnuid == namebotmnuitem_.id )
    {
	menu->setIsHandled( true );
	wd->showWellBotName( !wd->wellBotNameShown() );
    }
    else if ( mnuid == markermnuitem_.id )
    {
	menu->setIsHandled( true );
	wd->showMarkers( !wd->markersShown() );

    }
    else if ( mnuid == markernamemnuitem_.id )
    {
	menu->setIsHandled( true );
	wd->showMarkerName( !wd->markerNameShown() );
    }
    else if ( mnuid == showlogmnuitem_.id )
    {
       	menu->setIsHandled( true );
	if( wd->getLogParSet()->getLeft()->getLogNm() == "None"
	    && wd->getLogParSet()->getRight()->getLogNm() == "None" )
	{
	    Well::LogDisplayParSet* logparset = wd->getLogParSet();
	    if( applMgr()->wellServer()->selectLogs( wellid, logparset ) )
	    {
	        wd->displayRightLog();
	        wd->displayLeftLog();
	    }
	}
	else
	    wd->showLogs( !wd->logsShown() );
    }
    else if ( mnuid == storemnuitem_.id )
    {
	menu->setIsHandled( true );
	const bool res = applMgr()->wellServer()->storeWell(
					wd->getWellCoords(), wd->name(), mid );
	if ( res )
	{
	    wd->setChanged( false );
	    wd->setMultiID( mid );
	}
    }
    else if ( mnuid == editmnuitem_.id )
    {
	menu->setIsHandled( true );
	const bool yn = wd->isHomeMadeWell();
	wd->setupPicking( !yn );
	if ( !yn )
	{
	    MouseCursorChanger cursorchgr( MouseCursor::Wait );
	    wd->showKnownPositions();
	}
    }
}


bool uiODWellTreeItem::askContinueAndSaveIfNeeded()
{
    mDynamicCastGet(visSurvey::WellDisplay*,wd,visserv_->getObject(displayid_));
    if ( wd->hasChanged() )
    {
	BufferString warnstr = "This well has changed since the last save.\n";
	warnstr += "Do you want to save it?";
	int retval = uiMSG().notSaved( warnstr.buf() );
	if ( !retval ) return true;
	else if ( retval == -1 ) return false;
	else
	    applMgr()->wellServer()->storeWell( wd->getWellCoords(),
		                                wd->name(), mid );
    }
    return true;
}
