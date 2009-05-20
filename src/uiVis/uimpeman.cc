/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          March 2004
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uimpeman.cc,v 1.148 2009-05-20 08:07:52 cvsumesh Exp $";

#include "uimpeman.h"

#include "attribsel.h"
#include "emobject.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "emtracker.h"
#include "executor.h"
#include "faultseedpicker.h"
#include "horizon2dseedpicker.h"
#include "horizon3dseedpicker.h"
#include "ioman.h"
#include "keystrs.h"
#include "mousecursor.h"
#include "mpeengine.h"
#include "pixmap.h"
#include "sectiontracker.h"
#include "selector.h"
#include "survinfo.h"
#include "undo.h"

#include "uicombobox.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uislider.h"
#include "uispinbox.h"
#include "uitaskrunner.h"
#include "uitoolbar.h"
#include "uiviscoltabed.h"
#include "uivispartserv.h"
#include "viscolortab.h"
#include "visdataman.h"
#include "visemobjdisplay.h"
#include "visplanedatadisplay.h"
#include "visrandomtrackdisplay.h"
#include "vismpe.h"
#include "vismpeseedcatcher.h"
#include "visselman.h"
#include "vissurvscene.h"
#include "vistexture3.h"
#include "vistransform.h"
#include "vistransmgr.h"

using namespace MPE;

#define mAddButton(pm,func,tip,toggle) \
    toolbar->addButton( pm, mCB(this,uiMPEMan,func), tip, toggle )

#define mAddMnuItm(mnu,txt,fn,fnm,idx) {\
    uiMenuItem* itm = new uiMenuItem( txt, mCB(this,uiMPEMan,fn) ); \
    mnu->insertItem( itm, idx ); itm->setPixmap( ioPixmap(fnm) ); }


#define mGetDisplays(create) \
    ObjectSet<visSurvey::MPEDisplay> displays; \
    TypeSet<int> scenes; \
    visserv->getChildIds( -1, scenes ); \
    for ( int idx=0; idx<scenes.size(); idx++ ) \
	displays += getDisplay( scenes[idx], create );



uiMPEMan::uiMPEMan( uiParent* p, uiVisPartServer* ps )
    : clickcatcher(0)
    , clickablesceneid(-1)
    , visserv(ps)
    , colbardlg(0)
    , seedpickwason(false)
    , oldactivevol(false)
    , mpeintropending(false)
{
    toolbar = new uiToolBar( p, "Tracking controls", uiToolBar::Bottom );
    addButtons();
    updateAttribNames();

    EM::EMM().undo().changenotifier.notify(
	    		mCB(this,uiMPEMan,updateButtonSensitivity) );
    engine().trackplanechange.notify(
	    		mCB(this,uiMPEMan,updateButtonSensitivity) );
    engine().trackplanetrack.notify(
	    		mCB(this,uiMPEMan,trackPlaneTrackCB) );
    engine().trackeraddremove.notify(
	    		mCB(this,uiMPEMan,trackerAddedRemovedCB) );
    MPE::engine().activevolumechange.notify(
			mCB(this,uiMPEMan,finishMPEDispIntro) );
    SurveyInfo& si = const_cast<SurveyInfo&>( SI() );
    si.workRangeChg.notify( mCB(this,uiMPEMan,workAreaChgCB) );
    visBase::DM().selMan().selnotifier.notify(
	    mCB(this,uiMPEMan,treeItemSelCB) );
    visBase::DM().selMan().deselnotifier.notify(
	    mCB(this,uiMPEMan,updateButtonSensitivity) );

    updateButtonSensitivity();
}


void uiMPEMan::addButtons()
{
    seedconmodefld = new uiComboBox( toolbar, "Seed connect mode" );
    seedconmodefld->setToolTip( "Seed connect mode" );
    seedconmodefld->selectionChanged.notify(
				mCB(this,uiMPEMan,seedConnectModeSel) );
    toolbar->addObject( seedconmodefld );
    toolbar->addSeparator();

    seedidx = mAddButton( "seedpickmode.png", addSeedCB, 
	    		  "Create seed ( key: 'Tab' )", true );
    toolbar->setShortcut(seedidx,"Tab");
    toolbar->addSeparator();

    trackinvolidx = mAddButton( "autotrack.png", trackFromSeedsAndEdges,
    				"Auto-track", false );
    toolbar->addSeparator();

    trackwithseedonlyidx = mAddButton( "trackfromseeds.png", trackFromSeedsOnly,
	    			       "Track From Seeds Only", false );
    toolbar->addSeparator();

    polyselectidx =  mAddButton( "polygonselect.png", selectionMode,
	    			 "Polygon Selection mode", true );
    uiPopupMenu* polymnu = new uiPopupMenu( toolbar, "PolyMenu" );
    mAddMnuItm( polymnu, "Polygon", handleToolClick, "polygonselect.png", 0 );
    mAddMnuItm( polymnu, "Rectangle", handleToolClick, "rectangleselect.png",1);
    toolbar->setButtonMenu( polyselectidx, *polymnu );

    toolbar->addSeparator();

    removeinpolygon = mAddButton( "trashcan.png", removeInPolygon,
	    			  " Remove PolySelection", false );
    toolbar->addSeparator();

    showcubeidx = mAddButton( "trackcube.png", showCubeCB,
	    		      "Show track area", true );
    toolbar->addSeparator();

    moveplaneidx = mAddButton( "QCplane-inline.png", movePlaneCB,
			       "Move track plane", true );
    uiPopupMenu* mnu = new uiPopupMenu( toolbar, "Menu" );
    mAddMnuItm( mnu, "Inline", handleOrientationClick, 
	    	"QCplane-inline.png", 0 );
    mAddMnuItm( mnu, "CrossLine", handleOrientationClick, 
	    	"QCplane-crossline.png", 1 );
    mAddMnuItm( mnu, "Z", handleOrientationClick, "QCplane-z.png", 2 );
    toolbar->setButtonMenu( moveplaneidx, *mnu );

    toolbar->addSeparator();
    
    attribfld = new uiComboBox( toolbar, "QC Attribute" );
    attribfld->setToolTip( "QC Attribute" );
    attribfld->selectionChanged.notify( mCB(this,uiMPEMan,attribSel) );
    toolbar->addObject( attribfld );

    clrtabidx = mAddButton( "colorbar.png", setColorbarCB,
			    "Set track plane colorbar", true );

    transfld = new uiSlider( toolbar, "Transparency", 2 );
    transfld->setOrientation( uiSlider::Horizontal );
    transfld->setMaxValue( 1 );
    transfld->setToolTip( "Transparency" );
    transfld->setStretch( 0, 0 );
    transfld->valueChanged.notify( mCB(this,uiMPEMan,transpChg) );
    toolbar->addObject( transfld );
    toolbar->addSeparator();

    trackforwardidx = mAddButton( "leftarrow.png", moveBackward,
	    			  "Move plane backward ( key: '[' )", false );
    toolbar->setShortcut(trackforwardidx,"[");
    trackbackwardidx = mAddButton( "rightarrow.png", moveForward,
	    			   "Move plane forward ( key: ']' )", false );
    toolbar->setShortcut(trackbackwardidx,"]");

    nrstepsbox = new uiSpinBox( toolbar, 0, "Nr of track steps" );
    nrstepsbox->setToolTip( "Nr of track steps" );
    nrstepsbox->setMinValue( 1 );
    toolbar->addObject( nrstepsbox );
    toolbar->addSeparator();

    mAddButton( "tracker-settings.png", showSettingsCB, "Settings", false );

    undoidx = mAddButton( "undo.png", undoPush, "Undo", false );
    redoidx = mAddButton( "redo.png", redoPush, "Redo", false );
}


uiMPEMan::~uiMPEMan()
{
    EM::EMM().undo().changenotifier.remove(
	    		mCB(this,uiMPEMan,updateButtonSensitivity) );
    deleteVisObjects();
    engine().trackplanechange.remove(
	    		mCB(this,uiMPEMan,updateButtonSensitivity) );
    engine().trackplanetrack.remove(
	    		mCB(this,uiMPEMan,trackPlaneTrackCB) );
    engine().trackeraddremove.remove(
	    		mCB(this,uiMPEMan,trackerAddedRemovedCB) );
    MPE::engine().activevolumechange.remove(
			mCB(this,uiMPEMan,finishMPEDispIntro) );
    SurveyInfo& si = const_cast<SurveyInfo&>( SI() );
    si.workRangeChg.remove( mCB(this,uiMPEMan,workAreaChgCB) );
    visBase::DM().selMan().selnotifier.remove(
	    mCB(this,uiMPEMan,treeItemSelCB) );
    visBase::DM().selMan().deselnotifier.remove(
	    mCB(this,uiMPEMan,updateButtonSensitivity) );
}


void uiMPEMan::deleteVisObjects()
{
    TypeSet<int> scenes;
    visserv->getChildIds( -1, scenes );
    for ( int idx=0; idx<scenes.size(); idx++ )
    {
	visSurvey::MPEDisplay* mped = getDisplay(scenes[idx]);
	if ( mped )
	{
	    mped->boxDraggerStatusChange.remove(
		mCB(this,uiMPEMan,boxDraggerStatusChangeCB) );
	    visserv->removeObject(mped->id(),scenes[idx]);
	}
    }

    if ( clickcatcher )
    {
	if ( clickablesceneid>=0 )
	    visserv->removeObject( clickcatcher->id(), clickablesceneid );

	clickcatcher->click.remove( mCB(this,uiMPEMan,seedClick) );
	clickcatcher->unRef();
	clickcatcher = 0;
    }
}


void uiMPEMan::seedClick( CallBacker* )
{
    MPE::Engine& engine = MPE::engine();
    MPE::EMTracker* tracker = getSelectedTracker();
    if ( !tracker ) 
	return;

    EM::EMObject* emobj = EM::EMM().getObject( tracker->objectID() );
    if ( !emobj ) 
	return;

    const int clickedobject = clickcatcher->info().getObjID();
    if ( clickedobject == -1 )
	return;

    if ( !clickcatcher->info().isLegalClick() )
    {
	visBase::DataObject* dataobj = visserv->getObject( clickedobject );
	mDynamicCastGet( visSurvey::RandomTrackDisplay*, randomdisp, dataobj );

	if ( tracker->is2D() && !clickcatcher->info().getObjLineName() )
	    uiMSG().error( "2D tracking cannot handle picks on 3D lines.");
	else if ( !tracker->is2D() && clickcatcher->info().getObjLineName() )
	    uiMSG().error( "3D tracking cannot handle picks on 2D lines.");
	else if ( randomdisp )
	    uiMSG().error( emobj->getTypeStr(),
			   " tracking cannot handle picks on random lines.");
	else if ( clickcatcher->info().getObjCS().nrZ()==1 &&
		  !clickcatcher->info().getObjCS().isEmpty() )
	    uiMSG().error( emobj->getTypeStr(), 
			   " tracking cannot handle picks on time slices." );
	return;
    }
	
    const EM::PosID pid = clickcatcher->info().getNode();
    if ( pid.objectID()!=emobj->id() && pid.objectID()!=-1 )
	return;

    MPE::EMSeedPicker* seedpicker = tracker->getSeedPicker(true);
    if ( !seedpicker || !seedpicker->canSetSectionID() ||
	 !seedpicker->setSectionID(emobj->sectionID(0)) ) 
    {
	return;
    }

    Coord3 seedpos;
    if ( pid.objectID() == -1 )
    {
	visSurvey::Scene* scene = visSurvey::STM().currentScene();
	const Coord3 pos0 = clickcatcher->info().getPos();
	const Coord3 pos1 = scene->getZScaleTransform()->transformBack( pos0 );
	seedpos = scene->getUTM2DisplayTransform()->transformBack( pos1 );
    }
    else
    {
	seedpos = emobj->getPos(pid);
    }

    mGetDisplays(false);
    const bool trackerisshown = displays.size() && 
			        displays[0]->isDraggerShown();

    if ( tracker->is2D() )
    {
	const MultiID& lset = clickcatcher->info().getObjLineSet();
	const BufferString& lname = clickcatcher->info().getObjLineName();

	const bool lineswitch = lset!=engine.active2DLineSetID() ||
			        lname!=engine.active2DLineName(); 

	engine.setActive2DLine( lset, lname );

	mDynamicCastGet( MPE::Horizon2DSeedPicker*, h2dsp, seedpicker );
	DataPack::ID datapackid = clickcatcher->info().getObjDataPackID();
	const Attrib::SelSpec* as = clickcatcher->info().getObjDataSelSpec();

	if ( !as || !h2dsp || !h2dsp->canAddSeed(*as) )
	{
	    uiMSG().error( "2D tracking requires attribute from setup "
			   "to be displayed" );
	    return;
	}
	if ( datapackid > DataPack::cNoID() )
	    engine.setAttribData( *as, datapackid );

	h2dsp->setLine( lset, lname );
	if ( !h2dsp->startSeedPick() )
	    return;
    }
    else
    {
	if ( !seedpicker->startSeedPick() )
	    return;
	
	CubeSampling newvolume = clickcatcher->info().getObjCS();
	const CubeSampling trkplanecs = engine.trackPlane().boundingBox();

	if ( trackerisshown && trkplanecs.zrg.includes(seedpos.z) && 
	     trkplanecs.hrg.includes( SI().transform(seedpos) ) &&
	     trkplanecs.defaultDir()==newvolume.defaultDir() )
	{
	    newvolume = trkplanecs;
	}
	
	if ( newvolume.isEmpty() )
	    return;
	
	if ( newvolume != engine.activeVolume() )
	{
	    if ( oldactivevol.isEmpty() )
	    {
		if ( newvolume == trkplanecs )
		    loadPostponedData();
		else
		    engine.swapCacheAndItsBackup();

		oldactivevol = engine.activeVolume();
		oldtrackplane = engine.trackPlane();
	    }

	    NotifyStopper notifystopper( engine.activevolumechange );
	    engine.setActiveVolume( newvolume );
	    notifystopper.restore();

	    const Attrib::SelSpec* clickedas = 
				   clickcatcher->info().getObjDataSelSpec();
	    if ( clickedas && !engine.cacheIncludes(*clickedas,newvolume) )
	    {
		DataPack::ID datapackid =
				clickcatcher->info().getObjDataPackID();
		if ( datapackid > DataPack::cNoID() )
		    engine.setAttribData( *clickedas, datapackid );
	    }

	    for ( int idx=0; idx<displays.size(); idx++ )
		displays[idx]->freezeBoxPosition( true );

	    if ( !seedpicker->doesModeUseSetup() )
		visserv->toggleBlockDataLoad();

	    engine.setOneActiveTracker( tracker );
	    engine.activevolumechange.trigger();

	    if ( !seedpicker->doesModeUseSetup() )
		visserv->toggleBlockDataLoad();
	}
    }

    const int currentevent = EM::EMM().undo().currentEventID();
    MouseCursorManager::setOverride( MouseCursor::Wait );
    emobj->setBurstAlert( true );
    
    const bool ctrlshiftclicked = clickcatcher->info().isCtrlClicked() &&
				  clickcatcher->info().isShiftClicked();
    if ( pid.objectID()!=-1 )
    {
	if ( ctrlshiftclicked )
	    seedpicker->removeSeed( pid, false, false );
	else if ( clickcatcher->info().isCtrlClicked() )
	    seedpicker->removeSeed( pid, true, true );
	else if ( clickcatcher->info().isShiftClicked() )
	    seedpicker->removeSeed( pid, true, false );
	else
	    seedpicker->addSeed( seedpos, false );
    }
    else
	seedpicker->addSeed( seedpos, ctrlshiftclicked );

    emobj->setBurstAlert( false );
    MouseCursorManager::restoreOverride();
    setUndoLevel(currentevent);
    
    if ( !isPickingWhileSetupUp() )
	restoreActiveVol();
}


void uiMPEMan::restoreActiveVol()
{
    MPE::Engine& engine = MPE::engine();
    if ( !oldactivevol.isEmpty() ) 
    {
	if ( engine.activeVolume() != oldtrackplane.boundingBox() )
	    engine.swapCacheAndItsBackup();
	NotifyStopper notifystopper( engine.activevolumechange );
	engine.setActiveVolume( oldactivevol );
	notifystopper.restore();
	engine.setTrackPlane( oldtrackplane, false );
	engine.unsetOneActiveTracker();
	engine.activevolumechange.trigger();

	mGetDisplays(false);
	for ( int idx=0; idx<displays.size(); idx++ )
	    displays[idx]->freezeBoxPosition( false );

	oldactivevol.setEmpty();
    }
}


uiToolBar* uiMPEMan::getToolBar() const 
{ 
    return toolbar; 
}


visSurvey::MPEDisplay* uiMPEMan::getDisplay( int sceneid, bool create )
{
    mDynamicCastGet(const visSurvey::Scene*,scene,visserv->getObject(sceneid));
    if ( !scene ) return 0;

    TypeSet<int> displayids;
    visserv->findObject( typeid(visSurvey::MPEDisplay), displayids );

    for ( int idx=0; idx<displayids.size(); idx++ )
    {
	if ( scene->getFirstIdx(displayids[idx]) == -1 )
	    continue;

	visBase::DataObject* dobj = visserv->getObject( displayids[idx] );
	return reinterpret_cast<visSurvey::MPEDisplay*>( dobj );
    }

    if ( !create ) return 0;

    visSurvey::MPEDisplay* mpedisplay = visSurvey::MPEDisplay::create();

    visserv->addObject( mpedisplay, scene->id(), false );
    mpedisplay->setDraggerTransparency( transfld->getValue() );
    mpedisplay->showDragger( toolbar->isOn(moveplaneidx) );

    mpedisplay->boxDraggerStatusChange.notify(
	    mCB(this,uiMPEMan,boxDraggerStatusChangeCB) );

    return mpedisplay;
}


void uiMPEMan::updateAttribNames()
{
    BufferString oldsel = attribfld->text();
    attribfld->empty();
    attribfld->addItem( sKeyNoAttrib() );

    ObjectSet<const Attrib::SelSpec> attribspecs;
    engine().getNeededAttribs( attribspecs );
    for ( int idx=0; idx<attribspecs.size(); idx++ )
    {
	const Attrib::SelSpec* spec = attribspecs[idx];
	attribfld->addItem( spec->userRef() );
    }
    attribfld->setCurrentItem( oldsel );

    updateSelectedAttrib();
    attribSel(0);
    updateButtonSensitivity(0);
}


void uiMPEMan::updateSelectedAttrib()
{
    mGetDisplays(false);

    if ( displays.isEmpty() )
	return;

    ObjectSet<const Attrib::SelSpec> attribspecs;
    engine().getNeededAttribs( attribspecs );

    const char* userref = displays[0]->getSelSpecUserRef();
    if ( !userref && !attribspecs.isEmpty() )
    {
	for ( int idx=0; idx<displays.size(); idx++ )
	    displays[idx]->setSelSpec( 0, *attribspecs[0] );

	userref = displays[0]->getSelSpecUserRef();
    }
    else if ( userref==sKey::None )
	userref = sKeyNoAttrib();
    
    if ( userref )  	
	attribfld->setCurrentItem( userref );
}


bool uiMPEMan::isSeedPickingOn() const
{
    return clickcatcher && clickcatcher->isOn();
}


bool uiMPEMan::isPickingWhileSetupUp() const
{
    return isSeedPickingOn() && 
	( visserv->isTrackingSetupActive() );
}


void uiMPEMan::turnSeedPickingOn( bool yn )
{
    if ( isSeedPickingOn() == yn )
	return;
    toolbar->turnOn( seedidx, yn );
    MPE::EMTracker* tracker = getSelectedTracker();

    if ( yn )
    {
	visserv->setViewMode(false);
	toolbar->turnOn( showcubeidx, false );
	showCubeCB(0);

	if ( !clickcatcher )
	{
	    TypeSet<int> catcherids;
	    visserv->findObject( typeid(visSurvey::MPEClickCatcher), 
		    		 catcherids );
	    if ( catcherids.size() )
	    {
		visBase::DataObject* dobj = visserv->getObject( catcherids[0] );
	        clickcatcher = 
		    reinterpret_cast<visSurvey::MPEClickCatcher*>( dobj );
	    }
	    else
	    {
		clickcatcher = visSurvey::MPEClickCatcher::create();
	    }
	    clickcatcher->ref();
	    clickcatcher->click.notify(mCB(this,uiMPEMan,seedClick));
	}

	clickcatcher->turnOn( true );
	updateClickCatcher();
	
	const EM::EMObject* emobj = 
	    		tracker ? EM::EMM().getObject(tracker->objectID()) : 0;

    	if ( emobj ) 
	    clickcatcher->setTrackerType( emobj->getTypeStr() );
    }
    else
    {
	MPE::EMSeedPicker* seedpicker = tracker ? 
				        tracker->getSeedPicker(true) : 0;
	if ( seedpicker )
	    seedpicker->stopSeedPick();

	if ( clickcatcher ) clickcatcher->turnOn( false );

	restoreActiveVol();
    }
    visserv->sendPickingStatusChangeEvent();
}


void uiMPEMan::updateClickCatcher()
{
    const TypeSet<int>& selectedids = visBase::DM().selMan().selected();
    if ( !clickcatcher || selectedids.size()!=1 ) 
	return;

    const int newsceneid = visserv->getSceneID( selectedids[0] );
    if ( newsceneid<0 || newsceneid == clickablesceneid )
	return;
    
    visserv->removeObject( clickcatcher->id(), clickablesceneid );
    visserv->addObject( clickcatcher, newsceneid, false );
    clickablesceneid = newsceneid;
}


void uiMPEMan::boxDraggerStatusChangeCB( CallBacker* cb )
{
    mGetDisplays(false);
    bool ison = false;
    for ( int idx=0; idx<displays.size(); idx++ )
    {
	if ( cb!=displays[idx] )
	    continue;

	ison = displays[idx]->isBoxDraggerShown();
	if ( !ison )
	    displays[idx]->updateMPEActiveVolume();
    }

    toolbar->turnOn( showcubeidx, ison );
}


void uiMPEMan::showCubeCB( CallBacker* )
{
    const bool isshown = toolbar->isOn( showcubeidx );
    if ( isshown)
	turnSeedPickingOn( false );
    mGetDisplays(isshown)
    for ( int idx=0; idx<displays.size(); idx++ )
	displays[idx]->showBoxDragger( isshown );

    toolbar->setToolTip( showcubeidx, isshown ? "Hide track area"
					      : "Show track area" );
}


void uiMPEMan::attribSel( CallBacker* )
{
    mGetDisplays(false);
    const bool trackerisshown = displays.size() &&
			        displays[0]->isDraggerShown();
    if ( trackerisshown && attribfld->currentItem() )
	loadPostponedData();

    MouseCursorChanger cursorchanger( MouseCursor::Wait );

    if ( !attribfld->currentItem() )
    {
	for ( int idx=0; idx<displays.size(); idx++ )
	{
	    Attrib::SelSpec spec( 0, Attrib::SelSpec::cNoAttrib() );
	    displays[idx]->setSelSpec( 0, spec );
	    if ( trackerisshown )
		displays[idx]->updateTexture();
	}
    }
    else
    {
	ObjectSet<const Attrib::SelSpec> attribspecs;
	engine().getNeededAttribs( attribspecs );
	for ( int idx=0; idx<attribspecs.size(); idx++ )
	{
	    const Attrib::SelSpec* spec = attribspecs[idx];
	    if ( strcmp(spec->userRef(),attribfld->text()) )
		continue;

	    for ( int idy=0; idy<displays.size(); idy++ )
	    {
		displays[idy]->setSelSpec( 0, *spec );
		if ( trackerisshown )
		    displays[idy]->updateTexture();
	    }
	    break;
	}	
    }

    if ( colbardlg && displays.size() )
    {
	const int coltabid =
	    displays[0]->getTexture() && displays[0]->getTexture()->isOn()
		?  displays[0]->getTexture()->getColorTab().id()
		: -1;

	colbardlg->setColTab( coltabid );
    }

    updateButtonSensitivity();
}


void uiMPEMan::transpChg( CallBacker* )
{
    mGetDisplays(false)
    for ( int idx=0; idx<displays.size(); idx++ )
	displays[idx]->setDraggerTransparency( transfld->getValue() );
}


void uiMPEMan::addSeedCB( CallBacker* )
{
    turnSeedPickingOn( toolbar->isOn(seedidx) );
    updateButtonSensitivity(0);
}


void uiMPEMan::updateSeedModeSel()
{ 
    MPE::EMTracker* tracker = getSelectedTracker();
    MPE::EMSeedPicker* seedpicker = tracker ? tracker->getSeedPicker(true) : 0;
    if ( seedpicker )
	seedconmodefld->setCurrentItem( seedpicker->getSeedConnectMode() );
}


void uiMPEMan::seedConnectModeSel( CallBacker* )
{
    MPE::EMTracker* tracker = getSelectedTracker();
    if ( !tracker )
	return;

    EM::EMObject* emobj = EM::EMM().getObject( tracker->objectID() );
    MPE::EMSeedPicker* seedpicker = tracker->getSeedPicker( true );
    if ( !emobj || !seedpicker )
	return;

    const int oldseedconmode = seedpicker->getSeedConnectMode();
    seedpicker->setSeedConnectMode( seedconmodefld->currentItem() ); 

    if ( seedpicker->doesModeUseSetup() )
    {
	const SectionTracker* sectiontracker = 
	      tracker->getSectionTracker( emobj->sectionID(0), true );
	if ( sectiontracker && !sectiontracker->hasInitializedSetup() )
	    visserv->sendShowSetupDlgEvent();
	if ( !sectiontracker || !sectiontracker->hasInitializedSetup() )
	    seedpicker->setSeedConnectMode( oldseedconmode ); 
    }
    
    turnSeedPickingOn( true );
    visserv->setViewMode(false);
    updateButtonSensitivity(0);
}


void uiMPEMan::treeItemSelCB( CallBacker* )
{
    validateSeedConMode();
    updateClickCatcher();
    updateButtonSensitivity(0);
}


void uiMPEMan::validateSeedConMode()
{
    if ( visserv->isTrackingSetupActive() )
	return;
    MPE::EMTracker* tracker = getSelectedTracker();
    if ( !tracker ) 
	return;
    const EM::EMObject* emobj = EM::EMM().getObject( tracker->objectID() );
    if ( !emobj ) 
	return;
    MPE::EMSeedPicker* seedpicker = tracker->getSeedPicker(true);
    if ( !seedpicker ) return;

    const SectionTracker* sectiontracker = 
			tracker->getSectionTracker( emobj->sectionID(0), true );
    const bool setupavailable = sectiontracker && 
				sectiontracker->hasInitializedSetup();
    if ( setupavailable || !seedpicker->doesModeUseSetup() ) 
	return;

    const int defaultmode = seedpicker->defaultSeedConMode( false ); 
    seedpicker->setSeedConnectMode( defaultmode );
    
    updateButtonSensitivity(0);
}


void uiMPEMan::introduceMPEDisplay()
{
    EMTracker* tracker = getSelectedTracker();
    const EMSeedPicker* seedpicker = tracker ? tracker->getSeedPicker(true) : 0;
    validateSeedConMode();

    if ( !SI().has3D() )
	return;
    if ( !seedpicker || !seedpicker->doesModeUseVolume() )
	return;

    toolbar->turnOn( showcubeidx, true );
    showCubeCB(0);
    
    attribfld->setCurrentItem( sKeyNoAttrib() );
    attribSel(0);

    mGetDisplays(false);
    if ( displays.size() && !displays[0]->isDraggerShown() )
    {
	toolbar->turnOn( moveplaneidx, true );
	movePlaneCB(0);
    }

    mpeintropending = true; 
}


void uiMPEMan::finishMPEDispIntro( CallBacker* )
{
    if ( !mpeintropending || !oldactivevol.isEmpty() )
	return;
    mpeintropending = false;
    
    mGetDisplays(false);
    if ( !displays.size() || !displays[0]->isDraggerShown() )
	return;
    
    EMTracker* tracker = getSelectedTracker();
    if ( !tracker)
	tracker = engine().getTracker( engine().highestTrackerID() );

    if ( !tracker || attribfld->currentItem() )
	return;

    ObjectSet<const Attrib::SelSpec> attribspecs;
    tracker->getNeededAttribs(attribspecs);
    if ( attribspecs.isEmpty() )
    	return;

    attribfld->setCurrentItem( attribspecs[0]->userRef() );
    attribSel(0);
}


void uiMPEMan::loadPostponedData()
{
    visserv->loadPostponedData();
    finishMPEDispIntro( 0 );
}


void uiMPEMan::undoPush( CallBacker* )
{
    MouseCursorChanger mcc( MouseCursor::Wait );

    EM::EMM().burstAlertToAll( true );
    if ( !EM::EMM().undo().unDo( 1, true  ) )
	uiMSG().error("Could not undo everything.");
    EM::EMM().burstAlertToAll( false );

    updateButtonSensitivity(0);
}


void uiMPEMan::redoPush( CallBacker* )
{
    MouseCursorChanger mcc( MouseCursor::Wait );

    EM::EMM().burstAlertToAll( true );
    if ( !EM::EMM().undo().reDo( 1, true ) )
	uiMSG().error("Could not redo everything.");
    EM::EMM().burstAlertToAll( false );

    updateButtonSensitivity(0);
}


MPE::EMTracker* uiMPEMan::getSelectedTracker() 
{
    const TypeSet<int>& selectedids = visBase::DM().selMan().selected();
    if ( selectedids.size()!=1 || visserv->isLocked(selectedids[0]) )
	return 0;

    mDynamicCastGet( visSurvey::EMObjectDisplay*, 
	    			surface, visserv->getObject(selectedids[0]) );
    if ( !surface ) return 0;
    const EM::ObjectID oid = surface->getObjectID();
    const int trackerid = MPE::engine().getTrackerByObject( oid );
    MPE::EMTracker* tracker = MPE::engine().getTracker( trackerid );
    if ( tracker  && tracker->isEnabled() )
	return tracker;

    return 0; 
}


void uiMPEMan::updateButtonSensitivity( CallBacker* ) 
{
    //Undo/Redo
    toolbar->setSensitive( undoidx, EM::EMM().undo().canUnDo() );
    BufferString tooltip("Undo ");
    if ( EM::EMM().undo().canUnDo() )
	tooltip += EM::EMM().undo().unDoDesc();
    toolbar->setToolTip( undoidx, tooltip.buf() );

    if ( EM::EMM().undo().canReDo() )
    {
	tooltip = "Redo ";
	tooltip += EM::EMM().undo().reDoDesc();
    }
    toolbar->setToolTip( redoidx, tooltip.buf() );

    toolbar->setSensitive( redoidx, EM::EMM().undo().canReDo() );

    //Seed button
    updateSeedPickState();

    const bool is2d = !SI().has3D();
    const bool isseedpicking = toolbar->isOn(seedidx);
    
    toolbar->setSensitive( moveplaneidx, !is2d );
    toolbar->setSensitive( showcubeidx, !is2d); 

    MPE::EMTracker* tracker = getSelectedTracker();
    MPE::EMSeedPicker* seedpicker = tracker ? tracker->getSeedPicker(true) : 0;
    const bool isinvolumemode = !seedpicker || seedpicker->doesModeUseVolume();
    toolbar->setSensitive( trackinvolidx, !is2d && isinvolumemode );
    toolbar->setSensitive( trackwithseedonlyidx, !is2d && isinvolumemode );
    toolbar->setSensitive ( removeinpolygon, !is2d && isinvolumemode );
    
    //Track forward, backward, attrib, trans, nrstep
    mGetDisplays(false);
    const bool trackerisshown = displays.size() &&
				displays[0]->isDraggerShown();

    toolbar->setSensitive( trackforwardidx, !is2d && trackerisshown );
    toolbar->setSensitive( trackbackwardidx, !is2d && trackerisshown );
    attribfld->setSensitive( !is2d && trackerisshown );
    transfld->setSensitive( !is2d && trackerisshown );
    nrstepsbox->setSensitive( !is2d && trackerisshown );

    //coltab
    toolbar->setSensitive( clrtabidx, !is2d && trackerisshown && !colbardlg &&
			   attribfld->currentItem()>0 );


    const TrackPlane::TrackMode tm = engine().trackPlane().getTrackMode();
    toolbar->turnOn( moveplaneidx, trackerisshown && tm==TrackPlane::Move );
}


#define mAddSeedConModeItems( seedconmodefld, typ ) \
    if ( emobj && emobj->getTypeStr() == EM##typ##TranslatorGroup::keyword() ) \
    { \
	for ( int idx=0; idx<typ##SeedPicker::nrSeedConnectModes(); idx++ ) \
	{ \
	    seedconmodefld-> \
		    addItem( typ##SeedPicker::seedConModeText(idx,true) ); \
	} \
	if ( typ##SeedPicker::nrSeedConnectModes()<=0 ) \
	    seedconmodefld->addItem("No seed mode"); \
    }


void uiMPEMan::updateSeedPickState()
{
    MPE::EMTracker* tracker = getSelectedTracker();
    MPE::EMSeedPicker* seedpicker = tracker ? tracker->getSeedPicker(true) : 0;
    
    toolbar->setSensitive( seedidx, seedpicker );
    seedconmodefld->setSensitive( seedpicker );
    seedconmodefld->empty();

    if ( !seedpicker )
    {
	seedconmodefld->addItem("No seed mode");
	if ( isSeedPickingOn() )
	{
	    turnSeedPickingOn( false );
	    seedpickwason = true;
	}
	return;
    }

    if ( seedpickwason )
    {
	seedpickwason = false;
	turnSeedPickingOn( true );
	if ( isPickingWhileSetupUp() )
	    turnSeedPickingOn( false );
    }

    const EM::EMObject* emobj = EM::EMM().getObject( tracker->objectID() );
    mAddSeedConModeItems( seedconmodefld, Horizon3D );
    mAddSeedConModeItems( seedconmodefld, Horizon2D );

    seedconmodefld->setCurrentItem( seedpicker->getSeedConnectMode() );
}


void uiMPEMan::trackPlaneTrackCB( CallBacker* )
{
    if ( engine().trackPlane().getTrackMode() != TrackPlane::Erase )
	loadPostponedData();
}


void uiMPEMan::trackerAddedRemovedCB( CallBacker* )
{
    if ( !engine().nrTrackersAlive() )
    {
	seedpickwason = false;
	toolbar->turnOn( showcubeidx, false );
	showCubeCB(0);
	showTracker(false);
	engine().setActiveVolume( engine().getDefaultActiveVolume() );
    }

    updateAttribNames();
}


void uiMPEMan::visObjectLockedCB()
{
    updateButtonSensitivity();
}


void uiMPEMan::moveForward( CallBacker* )
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );
    const int nrsteps = nrstepsbox->getValue();
    mGetDisplays(false)
    const int currentevent = EM::EMM().undo().currentEventID();
    for ( int idx=0; idx<displays.size(); idx++ )
	displays[idx]->moveMPEPlane( nrsteps );
    setUndoLevel(currentevent);
}


void uiMPEMan::moveBackward( CallBacker* )
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );
    const int nrsteps = nrstepsbox->getValue();
    mGetDisplays(false)
    const int currentevent = EM::EMM().undo().currentEventID();
    for ( int idx=0; idx<displays.size(); idx++ )
	displays[idx]->moveMPEPlane( -nrsteps );
    setUndoLevel(currentevent);
}


void uiMPEMan::trackFromSeedsOnly( CallBacker* cb )
{
    mGetDisplays(false);
    for ( int idx=0; idx<displays.size(); idx++ )
	displays[idx]->updateSeedOnlyPropagation( true );

    trackInVolume( cb );
}


void uiMPEMan::trackFromSeedsAndEdges( CallBacker* cb )
{
    mGetDisplays(false);
    for ( int idx=0; idx<displays.size(); idx++ )
	displays[idx]->updateSeedOnlyPropagation( false );

    trackInVolume( cb );
}


void uiMPEMan::trackInVolume( CallBacker* )
{
    mGetDisplays(false);
    for ( int idx=0; idx<displays.size(); idx++ )
	displays[idx]->updateMPEActiveVolume();
    loadPostponedData();

    const TrackPlane::TrackMode tm = engine().trackPlane().getTrackMode();
    engine().setTrackMode(TrackPlane::Extend);
    updateButtonSensitivity();
   
    NotifyStopper selstopper( EM::EMM().undo().changenotifier );
    MouseCursorManager::setOverride( MouseCursor::Wait );
    PtrMan<Executor> exec = engine().trackInVolume();
    if ( exec )
    {
	const int currentevent = EM::EMM().undo().currentEventID();
	uiTaskRunner uitr( toolbar );
	if ( !uitr.execute(*exec) )
	{
	    if ( engine().errMsg() )
		uiMSG().error( engine().errMsg() );
	}
	setUndoLevel(currentevent);
    }

    MouseCursorManager::restoreOverride();
    engine().setTrackMode(tm);
    updateButtonSensitivity();
}


static bool sIsPolySelect = true;

void uiMPEMan::selectionMode( CallBacker* cb )
{
    const bool ison = toolbar->isOn( polyselectidx );
    if ( !ison )
	visserv->setSelectionMode( uiVisPartServer::Off );
    else
    {
	visserv->setSelectionMode( sIsPolySelect ? uiVisPartServer::Polygon
						 : uiVisPartServer::Rectangle );
	visserv->turnSeedPickingOn( false );
    }
}


void uiMPEMan::handleToolClick( CallBacker* cb )
{
    mDynamicCastGet(uiMenuItem*,itm,cb)
    if ( !itm ) return;

    const bool ispoly = itm->id() == 0;

    toolbar->setPixmap( polyselectidx, ispoly ? "polygonselect.png"
	    				      : "rectangleselect.png" );
    toolbar->setToolTip( polyselectidx, ispoly ? "Polygon Selection mode"
	    				       : "Rectangle Selection mode" );
    sIsPolySelect = ispoly;
    selectionMode( cb );
}


void uiMPEMan::removeInPolygon( CallBacker* cb )
{
    const Selector<Coord3>* sel = visserv->getCoordSelector( clickablesceneid);

    if ( sel && sel->isOK() ) 
    {
	mGetDisplays(false);
	for ( int idx=0; idx<displays.size(); idx++ )
	    displays[idx]->removeSelectionInPolygon( *sel );

	toolbar->turnOn( polyselectidx, false );
	selectionMode( cb );

	toolbar->turnOn( seedidx, true );
	addSeedCB( cb );
    }
}


void uiMPEMan::showTracker( bool yn )
{
    if ( yn && attribfld->currentItem() ) 
	loadPostponedData();

    MouseCursorManager::setOverride( MouseCursor::Wait );
    mGetDisplays(true)
    for ( int idx=0; idx<displays.size(); idx++ )
	displays[idx]->showDragger(yn);
    MouseCursorManager::restoreOverride();
    updateButtonSensitivity();
}


void uiMPEMan::changeTrackerOrientation( int orient )
{
    MouseCursorManager::setOverride( MouseCursor::Wait );
    mGetDisplays(true)
	for ( int idx=0; idx<displays.size(); idx++ )
	    displays[idx]->setPlaneOrientation( orient );
    MouseCursorManager::restoreOverride();
}


void uiMPEMan::workAreaChgCB( CallBacker* )
{
    mGetDisplays(true)
    for ( int idx=0; idx<displays.size(); idx++ )
	displays[idx]->updateBoxSpace();

    if ( !SI().sampling(true).includes( engine().activeVolume() ) )
    {
	toolbar->turnOn( showcubeidx, false );
	showCubeCB(0);
	showTracker( false );
	engine().setActiveVolume( engine().getDefaultActiveVolume() );
    }
}


void uiMPEMan::setColorbarCB(CallBacker*)
{
    if ( colbardlg || !toolbar->isOn(clrtabidx) )
	return;

    mGetDisplays(false);

    if ( displays.size()<1 )
	return;

    const int coltabid = displays[0]->getTexture()
	?  displays[0]->getTexture()->getColorTab().id()
	: -1;

    colbardlg = new uiColorBarDialog( toolbar, coltabid,
				      "Track plane colorbar" );
    colbardlg->winClosing.notify( mCB(this,uiMPEMan,onColTabClosing) );
    colbardlg->go();

    updateButtonSensitivity();
}


void uiMPEMan::onColTabClosing( CallBacker* )
{
    toolbar->turnOn( clrtabidx, false );
    colbardlg = 0;

    updateButtonSensitivity();
}


void uiMPEMan::movePlaneCB( CallBacker* )
{
    const bool ison = toolbar->isOn( moveplaneidx );
    showTracker( ison );
    engine().setTrackMode( ison ? TrackPlane::Move : TrackPlane::None );
}


void uiMPEMan::handleOrientationClick( CallBacker* cb )
{
    mDynamicCastGet(uiMenuItem*,itm,cb)
    if ( !itm ) return;
    const int dim = itm->id();
    if ( !dim )
    {
	toolbar->setPixmap( moveplaneidx, "QCplane-inline.png" );
	toolbar->setToolTip( moveplaneidx, "Move track plane Inline" );
    }
    else if ( dim == 1 )
    {
	toolbar->setPixmap( moveplaneidx, "QCplane-crossline.png" );
	toolbar->setToolTip( moveplaneidx, "Move track plane Crossline" );
    }
    else
    {
	toolbar->setPixmap( moveplaneidx, "QCplane-z.png" );
	toolbar->setToolTip( moveplaneidx, "Move track plane Z-dir" );
    }

    changeTrackerOrientation( dim );
    movePlaneCB( cb );
}


void uiMPEMan::showSettingsCB( CallBacker* )
{
    visserv->sendShowSetupDlgEvent();
}


void uiMPEMan::initFromDisplay()
{
    // compatibility for session files where box outside workarea
    workAreaChgCB(0);

    mGetDisplays(false)
    for ( int idx=0; idx<displays.size(); idx++ )
    {
	displays[idx]->boxDraggerStatusChange.notify(
		mCB(this,uiMPEMan,boxDraggerStatusChangeCB) );

	if ( idx==0 )
	{
	    transfld->setValue( displays[idx]->getDraggerTransparency() );
	    toolbar->turnOn( showcubeidx, displays[idx]->isBoxDraggerShown() );
	}
    }
    
    bool showtracker = engine().trackPlane().getTrackMode()!=TrackPlane::None;
    if ( !engine().nrTrackersAlive() )
    {
	engine().setTrackMode( TrackPlane::None );
	showtracker = false;
    }

    showTracker( showtracker );
    updateSelectedAttrib();
    updateButtonSensitivity(0);
}


void uiMPEMan::setUndoLevel( int preveventnr )
{
    Undo& undo = EM::EMM().undo();
    const int currentevent = undo.currentEventID();
    if ( currentevent != preveventnr )
	    undo.setUserInteractionEnd(currentevent);
}
