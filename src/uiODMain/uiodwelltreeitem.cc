/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		May 2006
___________________________________________________________________

-*/

#include "uiodwelltreeitem.h"

#include "uiattribpartserv.h"
#include "uicreateattriblogdlg.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uipixmap.h"
#include "uitreeview.h"
#include "uivispartserv.h"
#include "uiwellattribpartserv.h"
#include "uiwellpartserv.h"

#include "draw.h"
#include "ioobj.h"
#include "survinfo.h"
#include "welldata.h"

#include "viswelldisplay.h"


uiODWellParentTreeItem::uiODWellParentTreeItem()
    : uiODSceneTreeItem( uiStrings::sWell() )
    , constlogsize_(true)
{
}


const char* uiODWellParentTreeItem::iconName() const
{ return "tree-well"; }


static const int cAddIdx	= 0;
static const int cTieIdx	= 1;
static const int cNewWellIdx	= 2;
static const int cAttribIdx	= 3;
static const int cLogDispSize = 4;

bool uiODWellParentTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertAction( new uiAction(m3Dots(uiStrings::sAdd())), cAddIdx );
    if ( SI().zIsTime() )
    {
	mnu.insertAction(
	    new uiAction(m3Dots(tr("Tie Well to Seismic")),"well_tie"),cTieIdx);
    }
    mnu.insertAction( new uiAction(m3Dots(tr("Pick New Trajectory"))),
		      cNewWellIdx );
    if ( children_.size() > 1 )
	mnu.insertAction( new uiAction(m3Dots(tr("Create Attribute Log"))),
			cAttribIdx);

    if ( children_.size() )
    {
	mnu.insertSeparator();
	uiAction* szmenuitem = new uiAction(tr("Constant Log Size"));
	mnu.insertAction( szmenuitem, cLogDispSize );
	szmenuitem->setCheckable( true );
	szmenuitem->setChecked( constlogsize_ );
    }

    if ( children_.size() > 1 )
    {
	mnu.insertSeparator();
	uiMenu* showmnu = new uiMenu( getUiParent(), tr("Show All") );
	showmnu->insertAction( new uiAction(tr("Well Names (Top)")), 41 );
	showmnu->insertAction( new uiAction(tr("Well Names (Bottom)")), 42 );
	showmnu->insertAction( new uiAction(uiStrings::sMarker(mPlural)), 43 );
	showmnu->insertAction( new uiAction(tr("Marker Names")), 44 );
	showmnu->insertAction( new uiAction(uiStrings::sLogs()), 45 );
	mnu.addMenu( showmnu );

	uiMenu* hidemnu = new uiMenu( getUiParent(), tr("Hide All") );
	hidemnu->insertAction( new uiAction(tr("Well Names (Top)")), 51 );
	hidemnu->insertAction( new uiAction(tr("Well Names (Bottom)")), 52 );
	hidemnu->insertAction( new uiAction(uiStrings::sMarker(mPlural)), 53 );
	hidemnu->insertAction( new uiAction(tr("Marker Names")), 54 );
	hidemnu->insertAction( new uiAction(uiStrings::sLogs()), 55 );
	mnu.addMenu( hidemnu );
    }
    addStandardItems( mnu );

    const int mnuid = mnu.exec();
    return mnuid<0 ? false : handleSubMenu( mnuid );
}


#define mGetWellDisplayFromChild(childidx)\
    mDynamicCastGet(uiODWellTreeItem*,itm,children_[childidx]);\
    if ( !itm ) continue;\
    mDynamicCastGet(visSurvey::WellDisplay*,wd,\
		    visserv->getObject(itm->displayID()));\
    if ( !wd ) continue;
bool uiODWellParentTreeItem::handleSubMenu( int mnuid )
{
    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    if ( mnuid == cAddIdx )
    {
	DBKeySet wellids;
	applMgr()->selectWells( wellids );
	if ( wellids.isEmpty() )
	    return false;

	for ( int idx=0; idx<wellids.size(); idx++ )
	{
	    setMoreObjectsToDoHint( idx<wellids.size()-1 );
	    addChild(new uiODWellTreeItem(wellids[idx]), false );
	}
    }

    else if ( mnuid == cTieIdx )
    {
	 DBKey wid;
	 ODMainWin()->applMgr().wellAttribServer()->createD2TModel( wid );
    }

    else if ( mnuid == cNewWellIdx )
    {
	visSurvey::WellDisplay* wd = new visSurvey::WellDisplay;
	wd->setupPicking(true);
	BufferString wellname;
	Color color;
	if ( !applMgr()->wellServer()->setupNewWell(wellname,color) )
	    return false;

	wd->setLineStyle( OD::LineStyle(OD::LineStyle::Solid,1,color) );
	wd->setName( wellname );
	visserv->addObject( wd, sceneID(), true );
	addChild( new uiODWellTreeItem(wd->id()), false );
    }

    else if ( mnuid == cAttribIdx )
    {
	BufferStringSet wellnms;
	for ( int idx = 0; idx<children_.size(); idx++ )
	    wellnms.addIfNew( toString(children_[idx]->name()) );

	uiWellAttribPartServer* srv = ODMainWin()->applMgr().wellAttribServer();
	if ( srv->createAttribLog(wellnms) )
	    return false;
    }
    else if ( ( mnuid>40 && mnuid<46 ) || ( mnuid>50 && mnuid<56 ) )
    {
	for ( int idx=0; idx<children_.size(); idx++ )
	{
	    mGetWellDisplayFromChild( idx );
	    switch ( mnuid )
	    {
		case 41: wd->showWellTopName( true ); break;
		case 42: wd->showWellBotName( true ); break;
		case 43: wd->showMarkers( true ); break;
		case 44: wd->showMarkerName( true ); break;
		case 45: wd->showLogs( true ); break;
		case 51: wd->showWellTopName( false ); break;
		case 52: wd->showWellBotName( false ); break;
		case 53: wd->showMarkers( false ); break;
		case 54: wd->showMarkerName( false ); break;
		case 55: wd->showLogs( false ); break;
	    }
	}
    }
    else
	handleStandardItems( mnuid );

    return true;
}


uiTreeItem*
    uiODWellTreeItemFactory::createForVis( int visid, uiTreeItem* ) const
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


uiODWellTreeItem::uiODWellTreeItem( const DBKey& mid )
{
    mid_ = mid;
    initMenuItems();
}


uiODWellTreeItem::~uiODWellTreeItem()
{
    applMgr()->wellServer()->closePropDlg( mid_ );
    deepErase( logmnuitems_ );
}


void uiODWellTreeItem::initMenuItems()
{
    propertiesmnuitem_.text = m3Dots(uiStrings::sProperties());
    propertiesmnuitem_.iconfnm = "disppars";
    logviewermnuitem_.text = m3Dots(tr("2D Log Viewer"));
    gend2tmmnuitem_.text = m3Dots(tr("Tie Well to Seismic"));
    gend2tmmnuitem_.iconfnm = "well_tie";
    nametopmnuitem_.text = tr("Well Name (Top)");
    namebotmnuitem_.text = tr("Well Name (Bottom)");
    markermnuitem_.text = uiStrings::sMarker(mPlural);
    markernamemnuitem_.text = tr("Marker Names");
    showlogmnuitem_.text = uiStrings::sLogs() ;
    attrmnuitem_.text = m3Dots(tr("Create Attribute Log"));
    logcubemnuitem_.text = m3Dots(tr("Create Log Cube"));
    showmnuitem_.text = uiStrings::sShow() ;
    editmnuitem_.text = tr("Edit Welltrack" );
    storemnuitem_.text = uiStrings::sSave();
    storemnuitem_.iconfnm = "save" ;
    amplspectrummnuitem_.text = tr("Show Amplitude Spectrum");

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
	visSurvey::WellDisplay* wd = new visSurvey::WellDisplay;
	displayid_ = wd->id();
	visserv_->addObject( wd, sceneID(), true );
	uiRetVal uirv = wd->setDBKey( mid_ );
	if ( uirv.isError() )
	{
	    visserv_->removeObject( wd, sceneID() );
	    mTIUiMsg().error( uirv );
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


void uiODWellTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODDisplayTreeItem::createMenu( menu, istb );
    if ( !menu || menu->menuID()!=displayID() )
	return;

    if ( istb )
    {
	mAddMenuItem( menu, &propertiesmnuitem_, true, false );
	return;
    }

    mDynamicCastGet(visSurvey::WellDisplay*,wd,visserv_->getObject(displayid_));
    if ( !wd )
	return;

    const bool islocked = visserv_->isLocked( displayid_ );
    mAddMenuItem( menu, &displaymnuitem_, true, false );
    mAddMenuItem( &displaymnuitem_, &propertiesmnuitem_, true, false );
    mAddMenuItem( &displaymnuitem_, &logviewermnuitem_, true, false );
    mAddMenuItem( &displaymnuitem_, &showmnuitem_, true, false );
    mAddMenuItem( &showmnuitem_, &nametopmnuitem_, true,
						wd->wellTopNameShown() );
    mAddMenuItem( &showmnuitem_, &namebotmnuitem_, true,
						wd->wellBotNameShown() );
    mAddMenuItem( &showmnuitem_, &markermnuitem_, wd->canShowMarkers(),
		 wd->markersShown() );
    mAddMenuItem( &showmnuitem_, &markernamemnuitem_, wd->canShowMarkers(),
		  wd->canShowMarkers() && wd->markerNameShown() );
    mAddMenuItem( &showmnuitem_, &showlogmnuitem_,
		  applMgr()->wellServer()->hasLogs(wd->getDBKey()),
		  wd->logsShown() );

    deepErase( logmnuitems_ );
    mAddMenuItem( &displaymnuitem_, &amplspectrummnuitem_, true, false );
    BufferStringSet lognms;
    applMgr()->wellServer()->getLogNames( wd->getDBKey(), lognms );
    for ( int logidx=0; logidx<lognms.size(); logidx++ )
    {
    logmnuitems_ += new MenuItem( toUiString(lognms.get( logidx ) ));
	mAddMenuItem(&amplspectrummnuitem_,logmnuitems_[logidx],true,false);
    }

    if ( SI().zIsTime() )
	mAddMenuItem( menu, &gend2tmmnuitem_, true, false );

    mAddMenuItem( menu, &attrmnuitem_, true, false );
    mAddMenuItem( menu, &logcubemnuitem_, true, false );
    mAddMenuItem( menu, &editmnuitem_, !islocked, wd->isHomeMadeWell() );
    mAddMenuItem( menu, &storemnuitem_, wd->hasChanged(), false );
}


void uiODWellTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( menu->isHandled() || menu->menuID()!=displayID() || mnuid==-1 )
	return;

    mDynamicCastGet(visSurvey::WellDisplay*,wd,visserv_->getObject(displayid_))
    const DBKey& wellid = wd->getDBKey();
    if ( mnuid == attrmnuitem_.id )
    {
	menu->setIsHandled( true );
	applMgr()->wellAttribServer()->createAttribLog( wellid );
    }
    if ( mnuid == logcubemnuitem_.id )
    {
	menu->setIsHandled( true );
	applMgr()->wellAttribServer()->createLogCube( wellid );
    }
    else if ( mnuid == propertiesmnuitem_.id )
    {
	menu->setIsHandled( true );
	wd->restoreDispProp();
	ODMainWin()->applMgr().wellServer()->editDisplayProperties( wellid );
	updateColumnText( uiODSceneMgr::cColorColumn() );
    }
    else if ( amplspectrummnuitem_.findItem(mnuid) )
    {
	menu->setIsHandled( true );
	ODMainWin()->applMgr().wellAttribServer()->showAmplSpectrum( wellid,
		toString(amplspectrummnuitem_.findItem(mnuid)->text) );
	updateColumnText( uiODSceneMgr::cColorColumn() );
    }
    else if ( mnuid == logviewermnuitem_.id )
    {
	menu->setIsHandled( true );
	ODMainWin()->applMgr().wellServer()->displayIn2DViewer( wellid );
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
	wd->showLogs( !wd->logsShown() );
    }
    else if ( mnuid == storemnuitem_.id )
    {
	menu->setIsHandled( true );
	const bool res = applMgr()->wellServer()->storeWell(
			    wd->getWellCoords(), wd->name(), mid_ );
	if ( res )
	{
	    wd->setChanged( false );
	    wd->setDBKey( mid_ );
	}
    }
    else if ( mnuid == editmnuitem_.id )
    {
	menu->setIsHandled( true );
	const bool yn = wd->isHomeMadeWell();
	wd->setupPicking( !yn );
	if ( !yn )
	{
	    uiUserShowWait usw( getUiParent(), uiStrings::sUpdatingDisplay() );
	    wd->showKnownPositions();
	}
    }
    else if ( mnuid == gend2tmmnuitem_.id )
    {
	menu->setIsHandled( true );
	ODMainWin()->applMgr().wellAttribServer()->createD2TModel( wellid );
    }
}


bool uiODWellTreeItem::askContinueAndSaveIfNeeded( bool withcancel )
{
    mDynamicCastGet(visSurvey::WellDisplay*,wd,visserv_->getObject(displayid_));
    if ( wd && wd->hasChanged() )
    {
	uiString warnstr = tr("This well has changed since the last save.\n\n"
			      "Do you want to save it?");
	int retval = mTIUiMsg().askSave( warnstr, withcancel );
	if ( !retval ) return true;
	else if ( retval == -1 ) return false;
	else
	    applMgr()->wellServer()->storeWell( wd->getWellCoords(),
						wd->name(), mid_ );
    }

    return true;
}
