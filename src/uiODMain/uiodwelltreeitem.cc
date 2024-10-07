/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodwelltreeitem.h"

#include "ui3dviewer.h"
#include "uiattribpartserv.h"
#include "uicreateattriblogdlg.h"
#include "uimain.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uinotsaveddlg.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uipixmap.h"
#include "uitaskrunner.h"
#include "uitreeview.h"
#include "uivispartserv.h"
#include "uiwellattribpartserv.h"
#include "uiwellpartserv.h"

#include "draw.h"
#include "ioobj.h"
#include "ioman.h"
#include "mousecursor.h"
#include "survinfo.h"
#include "threadwork.h"
#include "welldata.h"
#include "welllog.h"
#include "wellman.h"
#include "wellreader.h"
#include "welltrack.h"


CNotifier<uiODWellParentTreeItem,uiMenu*>&
	uiODWellParentTreeItem::showMenuNotifier()
{
    static CNotifier<uiODWellParentTreeItem,uiMenu*> notif( nullptr );
    return notif;
}


uiODWellParentTreeItem::uiODWellParentTreeItem()
    : uiODParentTreeItem( uiStrings::sWell() )
    , constlogsize_(true)
{
}


uiODWellParentTreeItem::~uiODWellParentTreeItem()
{}


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
	uiAction* szmenuitem = new uiAction(tr("Constant Log Size"));
	mnu.insertAction( szmenuitem, cLogDispSize );
	szmenuitem->setCheckable( true );
	szmenuitem->setChecked( constlogsize_ );
    }

    showMenuNotifier().trigger( &mnu, this );

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
    mDynamicCastGet(visSurvey::WellDisplay*,welldisplay,\
		    visserv->getObject(itm->displayID()));\
    if ( !welldisplay ) continue;
bool uiODWellParentTreeItem::handleSubMenu( int mnuid )
{
    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    if ( mnuid == cAddIdx )
    {
	TypeSet<MultiID> emwellids;
	applMgr()->selectWells( emwellids );
	if ( emwellids.isEmpty() )
	    return false;

	const bool zistime = SI().zIsTime();
	Well::LoadReqs lreqs( Well::Inf, Well::Trck, Well::DispProps3D );
	lreqs.add( Well::LogInfos );
	if ( zistime )
	    lreqs.add( Well::D2T );

	uiTaskRunner uitr( uiMain::instance().topLevel() );
	RefObjectSet<Well::Data> wds;
	MultiWellReader mwr( emwellids, wds, lreqs );
	if ( !uitr.execute(mwr) )
	{
	    uiMSG().error( tr("Could not load wells.") );
	    return false;
	}

	uiString msg;
	if ( !mwr.allWellsRead() )
	    msg = mwr.errMsg();

	MouseCursorChanger mcc( MouseCursor::Wait );
	TypeSet<MultiID> remids;
	for ( const auto* wd : wds )
	{
	    const Well::D2TModel* d2t = wd->d2TModel();
	    const bool trackabovesrd = wd->track().zRange().stop_ <
					    -1.f * SI().seismicReferenceDatum();
	    if ( zistime && !d2t && !trackabovesrd )
	    {
		msg.append( wd->name() )
		   .append( tr(" : No depth to time model defined") )
		   .addNewLine();
		remids.addIfNew( wd->multiID() );
	    }
	}

	for ( int idx=emwellids.size()-1; idx>=0; idx-- )
	{
	    if ( remids.isPresent(emwellids[idx]) )
		emwellids.removeSingle( idx );
	}

	for ( int idx=0; idx<emwellids.size(); idx++ )
	{
	    setMoreObjectsToDoHint( idx<emwellids.size()-1 );
	    addChild( new uiODWellTreeItem(emwellids[idx]),false );
	}

	mcc.restore();
	if ( !msg.isEmpty() )
	    uiMSG().errorWithDetails( msg, tr("Could not load some wells") );
    }
    else if ( mnuid == cTieIdx )
    {
	 MultiID wid;
	 ODMainWin()->applMgr().wellAttribServer()->createD2TModel( wid );
    }
    else if ( mnuid == cNewWellIdx )
    {
	RefMan<visSurvey::WellDisplay> welldisplay = new visSurvey::WellDisplay;
	welldisplay->setupPicking(true);
	BufferString wellname;
	OD::Color color;
	if ( !applMgr()->wellServer()->setupNewWell(wellname,color) )
	    return false;

	welldisplay->setLineStyle(OD::LineStyle(OD::LineStyle::Solid,3,color));
	welldisplay->setName( wellname );
	visserv->addObject( welldisplay, sceneID(), true );
	addChild( new uiODWellTreeItem(welldisplay->id()), false );
    }
    else if ( mnuid == cAttribIdx )
    {
	BufferStringSet wellnms;
	for ( int idx = 0; idx<children_.size(); idx++ )
	    wellnms.addIfNew( children_[idx]->name().getFullString() );

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
		case 41: welldisplay->showWellTopName( true ); break;
		case 42: welldisplay->showWellBotName( true ); break;
		case 43: welldisplay->showMarkers( true ); break;
		case 44: welldisplay->showMarkerName( true ); break;
		case 45: welldisplay->showLogs( true ); break;
		case 51: welldisplay->showWellTopName( false ); break;
		case 52: welldisplay->showWellBotName( false ); break;
		case 53: welldisplay->showMarkers( false ); break;
		case 54: welldisplay->showMarkerName( false ); break;
		case 55: welldisplay->showLogs( false ); break;
	    }
	}
    }
    else
	handleStandardItems( mnuid );

    return true;
}


uiTreeItem* uiODWellTreeItemFactory::createForVis( const VisID& visid,
						   uiTreeItem* ) const
{
    mDynamicCastGet(visSurvey::WellDisplay*,welldisplay,
		    ODMainWin()->applMgr().visServer()->getObject(visid));
    return welldisplay ? new uiODWellTreeItem(visid) : nullptr;
}


uiODWellTreeItem::uiODWellTreeItem( const VisID& did )
{
    displayid_ = did;
    initMenuItems();

    mAttachCB( NotSavedPrompter::NSP().promptSaving,
	       uiODWellTreeItem::askSaveCB );
}


uiODWellTreeItem::uiODWellTreeItem( const MultiID& mid )
{
    mid_ = mid;
    initMenuItems();

    mAttachCB( NotSavedPrompter::NSP().promptSaving,
	       uiODWellTreeItem::askSaveCB );
}


uiODWellTreeItem::~uiODWellTreeItem()
{
    detachAllNotifiers();
    if ( applMgr()->wellServer() )
	applMgr()->wellServer()->closePropDlg( mid_ );

    deepErase( logmnuitems_ );
    visserv_->removeObject( displayid_, sceneID() );
}


void uiODWellTreeItem::initMenuItems()
{
    propertiesmnuitem_.text = m3Dots(uiStrings::sProperties());
    propertiesmnuitem_.iconfnm = "disppars";
    propertiesmnuitem_.placement = 1000;
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
    if ( !displayid_.isValid() )
    {
	RefMan<visSurvey::WellDisplay> vwd = new visSurvey::WellDisplay;
	displayid_ = vwd->id();
	{ //TODO: Should not be needed yet
	    RefMan<visSurvey::Scene> scene = visserv_->getScene( sceneID() );
	    vwd->setScene( scene );
	    if ( !vwd->setMultiID(mid_) )
	    {
		PtrMan<IOObj> ioobj = IOM().get( mid_ );
		BufferString objnm;
		if ( ioobj )
		    objnm = ioobj->name();

		uiMSG().error(tr("Could not load well %1").arg( objnm ) );
		vwd->setScene( nullptr );
		return false;
	    }
	}

	visserv_->addObject( vwd, sceneID(), true );
    }

    mDynamicCastGet(visSurvey::WellDisplay*,vwd,
		    visserv_->getObject(displayid_));
    if ( !vwd )
	return false;

    ConstRefMan<visSurvey::Scene> scene = visserv_->getScene( sceneID() );
    if ( !mid_.isUdf() && mid_ != vwd->getMultiID() )
    {
	if ( !vwd->setMultiID(mid_) )
	{
	    PtrMan<IOObj> ioobj = IOM().get( mid_ );
	    BufferString objnm;
	    if ( ioobj )
		objnm = ioobj->name();

	    uiMSG().error(tr("Could not load well %1").arg( objnm ) );
	    return false;
	}
    }

    if ( scene )
	vwd->setDisplayTransformation( scene->getUTM2DisplayTransform() );

    welldisplay_ = vwd;
    return uiODDisplayTreeItem::init();
}


ConstRefMan<visSurvey::WellDisplay> uiODWellTreeItem::getDisplay() const
{
    return welldisplay_.get();
}


RefMan<visSurvey::WellDisplay> uiODWellTreeItem::getDisplay()
{
    return welldisplay_.get();
}


bool uiODWellTreeItem::doubleClick( uiTreeViewItem* item )
{
    if ( item != uitreeviewitem_ )
	return uiTreeItem::doubleClick( item );

    RefMan<visSurvey::WellDisplay> welldisplay = getDisplay();
    if ( !welldisplay )
	return false;

    welldisplay->restoreDispProp();
    applMgr()->wellServer()->editDisplayProperties( welldisplay->getMultiID(),
					    welldisplay->getBackgroundColor() );
    return true;
}


void uiODWellTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODDisplayTreeItem::createMenu( menu, istb );
    if ( !menu || !isDisplayID(menu->menuID()) )
	return;

    if ( istb )
    {
	mAddMenuItem( menu, &propertiesmnuitem_, true, false );
	return;
    }

    ConstRefMan<visSurvey::WellDisplay> welldisplay = getDisplay();
    if ( !welldisplay )
	return;

    const bool islocked = visserv_->isLocked( displayid_ );
    mAddMenuItem( menu, &displaymnuitem_, true, false );
    mAddMenuItem( &displaymnuitem_, &propertiesmnuitem_, true, false );
    mAddMenuItem( &displaymnuitem_, &logviewermnuitem_, true, false );
    mAddMenuItem( &displaymnuitem_, &showmnuitem_, true, false );
    mAddMenuItem( &showmnuitem_, &nametopmnuitem_, true,
		  welldisplay->wellTopNameShown() );
    mAddMenuItem( &showmnuitem_, &namebotmnuitem_, true,
		  welldisplay->wellBotNameShown() );
    mAddMenuItem( &showmnuitem_, &markermnuitem_, welldisplay->canShowMarkers(),
		  welldisplay->markersShown() );
    mAddMenuItem( &showmnuitem_, &markernamemnuitem_,
		  welldisplay->canShowMarkers(),
		  welldisplay->canShowMarkers() &&
		  welldisplay->markerNameShown() );
    mAddMenuItem( &showmnuitem_, &showlogmnuitem_,
		  applMgr()->wellServer()->hasLogs(welldisplay->getMultiID()),
		  welldisplay->logsShown() );

    deepErase( logmnuitems_ );
    mAddMenuItem( &displaymnuitem_, &amplspectrummnuitem_, true, false );
    BufferStringSet lognms;
    Well::MGR().getLogNamesByID( welldisplay->getMultiID(), lognms );
    for ( int logidx=0; logidx<lognms.size(); logidx++ )
    {
	logmnuitems_ += new MenuItem( toUiString(lognms.get(logidx) ));
	mAddMenuItem(&amplspectrummnuitem_,logmnuitems_[logidx],true,false);
    }

    if ( SI().zIsTime() )
	mAddMenuItem( menu, &gend2tmmnuitem_, true, false );

    mAddMenuItem( menu, &attrmnuitem_, true, false );
    mAddMenuItem( menu, &logcubemnuitem_, true, false );
    mAddMenuItem( menu, &editmnuitem_, !islocked,welldisplay->isHomeMadeWell());
    mAddMenuItem( menu, &storemnuitem_, welldisplay->hasChanged(), false );
}


void uiODWellTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( menu->isHandled() || !isDisplayID(menu->menuID()) || mnuid==-1 )
	return;

    RefMan<visSurvey::WellDisplay> welldisplay = getDisplay();
    if ( !welldisplay )
	return;

    const MultiID& wellid = welldisplay->getMultiID();
    if ( mnuid == attrmnuitem_.id )
    {
	menu->setIsHandled( true );
	//TODO false set to make it compile: change!
	applMgr()->wellAttribServer()->setAttribSet(
				*applMgr()->attrServer()->curDescSet(false) );
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
	welldisplay->restoreDispProp();
	ODMainWin()->applMgr().wellServer()->editDisplayProperties( wellid,
					welldisplay->getBackgroundColor() );
	updateColumnText( uiODSceneMgr::cColorColumn() );
    }
    else if ( amplspectrummnuitem_.findItem(mnuid) )
    {
	menu->setIsHandled( true );
	ODMainWin()->applMgr().wellAttribServer()->showAmplSpectrum( wellid,
		amplspectrummnuitem_.findItem(mnuid)->text.getFullString() );
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
	welldisplay->showWellTopName( !welldisplay->wellTopNameShown() );
    }
    else if ( mnuid == namebotmnuitem_.id )
    {
	menu->setIsHandled( true );
	welldisplay->showWellBotName( !welldisplay->wellBotNameShown() );
    }
    else if ( mnuid == markermnuitem_.id )
    {
	menu->setIsHandled( true );
	welldisplay->showMarkers( !welldisplay->markersShown() );
    }
    else if ( mnuid == markernamemnuitem_.id )
    {
	menu->setIsHandled( true );
	welldisplay->showMarkerName( !welldisplay->markerNameShown() );
    }
    else if ( mnuid == showlogmnuitem_.id )
    {
	menu->setIsHandled( true );
	welldisplay->showLogs( !welldisplay->logsShown() );
    }
    else if ( mnuid == storemnuitem_.id )
    {
	menu->setIsHandled( true );
	saveCB( nullptr );
    }
    else if ( mnuid == editmnuitem_.id )
    {
	menu->setIsHandled( true );
	const bool yn = welldisplay->isHomeMadeWell();
	welldisplay->setupPicking( !yn );
	if ( !yn )
	{
	    MouseCursorChanger cursorchgr( MouseCursor::Wait );
	    welldisplay->showKnownPositions();
	}
    }
    else if ( mnuid == gend2tmmnuitem_.id )
    {
	menu->setIsHandled( true );
	ODMainWin()->applMgr().wellAttribServer()->createD2TModel( wellid );
    }
}


void uiODWellTreeItem::askSaveCB( CallBacker* )
{
    ConstRefMan<visSurvey::WellDisplay> welldisplay = getDisplay();
    if ( !welldisplay || !welldisplay->hasChanged() )
	return;

    const uiString obj = toUiString("Well \"%2\"").arg( welldisplay->name() );
    NotSavedPrompter::NSP().addObject( obj, mCB(this,uiODWellTreeItem,saveCB),
				       true, 0 );

    Threads::WorkManager::twm().addWork(
	    Threads::Work( *new uiTreeItemRemover(parent_, this), true ), 0,
	    NotSavedPrompter::NSP().queueID(), false );
}


void uiODWellTreeItem::saveCB( CallBacker* )
{
    if ( !applMgr() || !applMgr()->wellServer() )
	return;

    RefMan<visSurvey::WellDisplay> welldisplay = getDisplay();
    const bool res = applMgr()->wellServer()->storeWell(
		welldisplay->getWellCoords(), welldisplay->name(), mid_ );
    if ( !res )
	return;

    welldisplay->setChanged( false );
    welldisplay->setMultiID( mid_ );
}


bool uiODWellTreeItem::askContinueAndSaveIfNeeded( bool withcancel )
{
    ConstRefMan<visSurvey::WellDisplay> welldisplay = getDisplay();
    if ( welldisplay && welldisplay->hasChanged() )
    {
	uiString warnstr = tr("This well has changed since the last save.\n\n"
			      "Do you want to save it?");
	int retval = uiMSG().askSave( warnstr, withcancel );
	if ( !retval )
	    return true;
	else if ( retval == -1 )
	    return false;
	else
	    saveCB( nullptr );
    }

    return true;
}
