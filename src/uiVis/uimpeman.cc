/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          March 2004
 RCS:           $Id: uimpeman.cc,v 1.99 2006-07-28 13:56:09 cvsjaap Exp $
________________________________________________________________________

-*/

#include "uimpeman.h"

#include "attribsel.h"
#include "emhistory.h"
#include "emobject.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "emtracker.h"
#include "faultseedpicker.h"
#include "horizonseedpicker.h"
#include "horizon2dseedpicker.h"
#include "ioman.h"
#include "keystrs.h"
#include "mpeengine.h"
#include "sectiontracker.h"
#include "survinfo.h"

#include "uicombobox.h"
#include "uicursor.h"
#include "uiexecutor.h"
#include "uimsg.h"
#include "uislider.h"
#include "uispinbox.h"
#include "uitoolbar.h"
#include "uiviscoltabed.h"
#include "uivispartserv.h"
#include "viscolortab.h"
#include "visdataman.h"
#include "visdataman.h"
#include "visplanedatadisplay.h"
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

#define mGetDisplays(create) \
    ObjectSet<visSurvey::MPEDisplay> displays; \
    TypeSet<int> scenes; \
    visserv->getChildIds( -1, scenes ); \
    for ( int idx=0; idx<scenes.size(); idx++ ) \
	displays += getDisplay( scenes[idx], create );



uiMPEMan::uiMPEMan( uiParent* p, uiVisPartServer* ps )
    : clickcatcher(0)
    , visserv(ps)
    , init(false)
    , colbardlg(0)
    , blockattribsel(false)
    , seedpickwason(false)
    , oldactivevol(false)
    , trackerwasshown(false)
{
    toolbar = new uiToolBar(p,"Tracking controls");

    addButtons();

    toolbar->setCloseMode( 2 );
    toolbar->setResizeEnabled();
    toolbar->setVerticallyStretchable(false);
    updateAttribNames();

    EM::EMM().history().changenotifier.notify(
	    		mCB(this,uiMPEMan,updateButtonSensitivity) );
    engine().trackplanechange.notify(
	    		mCB(this,uiMPEMan,updateButtonSensitivity) );
    engine().trackeraddremove.notify(
	    		mCB(this,uiMPEMan,trackerAddedRemovedCB) );
    visBase::DM().selMan().selnotifier.notify(
	    mCB(this,uiMPEMan,updateButtonSensitivity) );
    visBase::DM().selMan().deselnotifier.notify(
	    mCB(this,uiMPEMan,updateButtonSensitivity) );

    updateButtonSensitivity();
}


void uiMPEMan::addButtons()
{
    seedconmodefld = new uiComboBox( toolbar, "Seed connect mode" );
    seedconmodefld->setToolTip( "Select seed connect mode" );
    seedconmodefld->selectionChanged.notify(
				mCB(this,uiMPEMan,seedConnectModeSel) );
    toolbar->addSeparator();

    seedidx = mAddButton( "seedpickmode.png", addSeedCB, "Create seed", true );
    toolbar->addSeparator();

    trackinvolidx = mAddButton( "track_seed.png", trackInVolume,
    				"Auto-track", false );
    toolbar->addSeparator();

    showcubeidx = mAddButton( "trackcube.png", showCubeCB,
	    		      "Show track area", true );
    toolbar->addSeparator();

    moveplaneidx = mAddButton( "moveplane.png", movePlaneCB,
			       "Move track plane", true );
    extendidx = mAddButton( "trackplane.png", extendModeCB,
	    		    "Extend mode", true );
    retrackidx = mAddButton( "retrackplane.png", retrackModeCB, 
	    		     "ReTrack mode", true );
    eraseidx = mAddButton( "erasingplane.png", eraseModeCB, 
	    		   "Erase mode", true );
    toolbar->addSeparator();
/*
    mouseeraseridx = mAddButton( "eraser.png", mouseEraseModeCB, 
	    			 "Erase with mouse", true );
    toolbar->addSeparator();
*/

    attribfld = new uiComboBox( toolbar, "Attribute" );
    attribfld->setToolTip( "QC Attribute" );
    attribfld->selectionChanged.notify( mCB(this,uiMPEMan,attribSel) );
    toolbar->setStretchableWidget( attribfld );

    clrtabidx = mAddButton( "colorbar.png", setColorbarCB,
			    "Set track plane colorbar", true );

    transfld = new uiSlider( toolbar, "Slider", 2 );
    transfld->setOrientation( uiSlider::Horizontal );
    transfld->setMaxValue( 1 );
    transfld->setToolTip( "Transparency" );
    transfld->valueChanged.notify( mCB(this,uiMPEMan,transpChg) );
    toolbar->addSeparator();

    trackforwardidx = mAddButton( "leftarrow.png", trackBackward,
	    			  "Track backward", false );
    trackbackwardidx = mAddButton( "rightarrow.png", trackForward,
	    			   "Track forward", false );

    nrstepsbox = new uiSpinBox( toolbar );
    nrstepsbox->setToolTip( "Nr of track steps" );
    nrstepsbox->setMinValue( 1 );
    toolbar->addSeparator();

    undoidx = mAddButton( "undo.png", undoPush, "Undo", false );
    redoidx = mAddButton( "redo.png", redoPush, "Redo", false );
}


uiMPEMan::~uiMPEMan()
{
    EM::EMM().history().changenotifier.remove(
	    		mCB(this,uiMPEMan,updateButtonSensitivity) );
    deleteVisObjects();
    engine().trackplanechange.remove(
	    		mCB(this,uiMPEMan,updateButtonSensitivity) );
    engine().trackeraddremove.remove(
	    		mCB(this,uiMPEMan,trackerAddedRemovedCB) );
    visBase::DM().selMan().selnotifier.remove(
	    mCB(this,uiMPEMan,updateButtonSensitivity) );
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
	if ( scenes.size() )
	    visserv->removeObject( clickcatcher->id(), scenes[0] );

	clickcatcher->click.remove( mCB(this,uiMPEMan,seedClick) );
	clickcatcher->unRef();
	clickcatcher = 0;
    }

    init = false;
}


void uiMPEMan::seedClick( CallBacker* )
{
    MPE::Engine& engine = MPE::engine();
    MPE::EMTracker* tracker = getSelectedTracker();
    if ( !tracker ) 
	return;

    const EM::EMObject* emobj = EM::EMM().getObject( tracker->objectID() );
    if ( !emobj ) 
	return;

    const EM::PosID pid = clickcatcher->clickedNode();
    if ( pid.objectID()!=emobj->id() && pid.objectID()!=-1 )
	return;

    const int clickedobject = clickcatcher->clickedObjectID();
    if ( clickedobject == -1 )
	return;

    MPE::EMSeedPicker* seedpicker = tracker->getSeedPicker(true);
    if ( !seedpicker || !seedpicker->canSetSectionID() ||
	 !seedpicker->setSectionID(emobj->sectionID(0)) ||
	 !seedpicker->startSeedPick() )
    {
	return;
    }

    Coord3 seedpos;
    if ( pid.objectID() == -1 )
    {
	visSurvey::Scene* scene = visSurvey::STM().currentScene();
	const Coord3 disppos = scene->getZScaleTransform()->
				   transformBack( clickcatcher->clickedPos() );
	seedpos = scene->getUTM2DisplayTransform()->transformBack(disppos);
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
	RefMan<const Attrib::DataCubes> cached =
	    clickcatcher->clickedObjectData();

	if ( cached )
	{
	    engine.setAttribData( *clickcatcher->clickedObjectDataSelSpec(),
				  cached );
	    mDynamicCastGet( MPE::Horizon2DSeedPicker*, h2dsp, seedpicker );
	    h2dsp->setLine( clickcatcher->clickedObjectLineSet(),
		    	    clickcatcher->clickedObjectLineName(),
		    	    clickcatcher->clickedObjectLineData() );
	}
    }
    else
    {
	CubeSampling newvolume = engine.trackPlane().boundingBox();

	BinID seedbid = SI().transform( seedpos );
	if ( !trackerisshown || !newvolume.zrg.includes(seedpos.z) ||
	     !newvolume.hrg.includes(seedbid) )
	{
	    newvolume = clickcatcher->clickedObjectCS();
	}
	
	if ( newvolume.isEmpty() )
	    return;
	
	if ( !engine.activeVolume().includes(newvolume) )
	{
	    // Not allowed to pick on more than one plane in wizard stage
	    if ( isPickingInWizard() && seedpicker->nrSeeds() )
		return;

	    if ( oldactivevol.isEmpty() )
	    {
		oldactivevol = engine.activeVolume();
		trackerwasshown = trackerisshown;
		oldtrackplane = engine.trackPlane();
		showTracker( false );
		engine.swapCacheAndItsBackup();
	    }

	    NotifyStopper notifystopper( engine.activevolumechange );
	    engine.setActiveVolume( newvolume );
	    notifystopper.restore();

	    RefMan<const Attrib::DataCubes> cached = 
					    clickcatcher->clickedObjectData();
	    if ( cached )
		engine.setAttribData( *clickcatcher->clickedObjectDataSelSpec(),
				      cached );

	    for ( int idx=0; idx<displays.size(); idx++ )
		displays[idx]->turnOn( false );

	    engine.activevolumechange.trigger();
	}
    }

    const int currentevent = EM::EMM().history().currentEventNr();
    uiCursor::setOverride( uiCursor::Wait );
    
    if ( pid.objectID()!=-1 )
    {
	if ( clickcatcher->ctrlClicked() )
	    seedpicker->removeSeed( pid, true );
	else if ( clickcatcher->shiftClicked() )
	    seedpicker->removeSeed( pid, false );
	else
	    seedpicker->addSeed( seedpos );
    }
    else
	seedpicker->addSeed( seedpos );
    
    uiCursor::restoreOverride();
    setHistoryLevel(currentevent);
    
    if ( !isPickingInWizard() )
	restoreActiveVol();
    visserv->makeSectionDisplayRefresh();
}


void uiMPEMan::restoreActiveVol()
{
    MPE::Engine& engine = MPE::engine();
    if ( !oldactivevol.isEmpty() ) 
    {
	engine.swapCacheAndItsBackup();
	NotifyStopper notifystopper( engine.activevolumechange );
	engine.setActiveVolume( oldactivevol );
	notifystopper.restore();

	mGetDisplays(false);
	for ( int idx=0; idx<displays.size(); idx++ )
	    displays[idx]->turnOn( true );

	engine.setTrackPlane( oldtrackplane, false );
	engine.activevolumechange.trigger();
	showTracker( trackerwasshown );
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
    mpedisplay->showDragger( toolbar->isOn(extendidx) );

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

/*    if ( !init && attribfld->size()>1 && attribspecs.size() &&
	 engine().activeVolume() != engine().getDefaultActiveVolume() )
    {
	MPE::EMTracker* tracker = getSelectedTracker();
	MPE::EMSeedPicker* seedpicker = tracker ? 
					tracker->getSeedPicker(true) : 0;
	if ( !seedpicker || seedpicker->doesModeUseVolume() )
	{
	    init = true;
	    engine().setTrackMode( TrackPlane::Extend );
	    showTracker( true );
	    attribfld->setCurrentItem( (int)1 );
	}
    }
*/
    attribSel(0);
    updateButtonSensitivity(0);
}


void uiMPEMan::updateSelectedAttrib()
{
    mGetDisplays(false);

    if ( !displays.size() )
	return;

    ObjectSet<const Attrib::SelSpec> attribspecs;
    engine().getNeededAttribs( attribspecs );

    const char* userref = displays[0]->getSelSpecUserRef();
    if ( !userref && attribspecs.size() )
    {
	for ( int idx=0; idx<displays.size(); idx++ )
	    displays[idx]->setSelSpec( 0, *attribspecs[0] );

	userref = displays[0]->getSelSpecUserRef();
    }
    else if ( userref==sKey::None )
	userref = sKeyNoAttrib();
    
    if ( userref && !blockattribsel ) 	
	attribfld->setCurrentItem( userref );
}


bool uiMPEMan::isSeedPickingOn() const
{
    return clickcatcher && clickcatcher->isOn();
}


bool uiMPEMan::isPickingInWizard() const
{
    return isSeedPickingOn() && !seedconmodefld->sensitive();
}


void uiMPEMan::turnSeedPickingOn( bool yn )
{
    if ( isSeedPickingOn() == yn )
	return;
    blockattribsel = yn;
    toolbar->turnOn( seedidx, yn );

    if ( yn )
    {
	toolbar->turnOn( showcubeidx, false );
	showCubeCB(0);

	if ( !clickcatcher )
	{
	    clickcatcher = visSurvey::MPEClickCatcher::create();
	    clickcatcher->ref();

	    TypeSet<int> scenes;
	    visserv->getChildIds( -1, scenes );
	    visserv->addObject( clickcatcher, scenes[0], false );
	    clickcatcher->click.notify(mCB(this,uiMPEMan,seedClick));
	}

	clickcatcher->turnOn( true );
    }
    else
    {
	MPE::EMTracker* tracker = getSelectedTracker();
	MPE::EMSeedPicker* seedpicker = tracker ? 
				        tracker->getSeedPicker(true) : 0;
	if ( seedpicker )
	    seedpicker->stopSeedPick();

	if ( clickcatcher ) clickcatcher->turnOn( false );

	restoreActiveVol();
    }
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
    mGetDisplays(isshown)
    for ( int idx=0; idx<displays.size(); idx++ )
	displays[idx]->showBoxDragger( isshown );

    toolbar->setToolTip( showcubeidx, isshown ? "Hide track area"
					      : "Show track area" );
}


void uiMPEMan::attribSel( CallBacker* )
{
    if ( blockattribsel )
	return;

    mGetDisplays(false);
    const bool trackerisshown = displays.size() &&
			        displays[0]->isDraggerShown();
    if ( trackerisshown && attribfld->currentItem() )
	visserv->loadPostponedData();

    uiCursorChanger cursorchanger( uiCursor::Wait );

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


void uiMPEMan::mouseEraseModeCB( CallBacker* )
{
    mGetDisplays(false)
}


void uiMPEMan::addSeedCB( CallBacker* )
{
    turnSeedPickingOn( toolbar->isOn(seedidx) );
    updateButtonSensitivity(0);
}


void uiMPEMan::seedConnectModeSel( CallBacker* )
{
    MPE::EMTracker* tracker = getSelectedTracker();
    MPE::EMSeedPicker* seedpicker = tracker ? tracker->getSeedPicker(true) : 0;
    if ( !seedpicker )
	return;

    const int oldseedconmode = seedpicker->getSeedConnectMode();
    seedpicker->setSeedConnectMode( seedconmodefld->currentItem() ); 
    turnSeedPickingOn( false );

    if ( seedpicker->doesModeUseSetup() )
    {
	const SectionTracker* sectiontracker = 
	      tracker->getSectionTracker( seedpicker->getSectionID(), true );
	if (  sectiontracker && !sectiontracker->hasInitializedSetup() )
	    visserv->sendShowSetupDlgEvent();
	if (  !sectiontracker || !sectiontracker->hasInitializedSetup() )
	    seedpicker->setSeedConnectMode( oldseedconmode ); 
    }
    
    turnSeedPickingOn( !seedpicker->doesModeUseVolume() );
    updateButtonSensitivity(0);
}


void uiMPEMan::validateSeedConMode()
{
    MPE::EMTracker* tracker = getSelectedTracker();
    MPE::EMSeedPicker* seedpicker = tracker ? tracker->getSeedPicker(true) : 0;
    if ( !seedpicker ) return;

    const SectionTracker* sectiontracker = 
		tracker->getSectionTracker( seedpicker->getSectionID(), true );
    const bool setupavailable = sectiontracker && 
				sectiontracker->hasInitializedSetup();
    if ( setupavailable || !seedpicker->doesModeUseSetup() ) 
	return;

    const int defaultmode = seedpicker->defaultSeedConMode( false ); 
    seedpicker->setSeedConnectMode( defaultmode );
    
    turnSeedPickingOn( !seedpicker->doesModeUseVolume() );
    updateButtonSensitivity(0);
}


void uiMPEMan::undoPush( CallBacker* )
{
    if ( !EM::EMM().history().unDo( 1, mEMHistoryUserInteractionLevel ) )
	uiMSG().error("Could not undo everything.");

    visserv->makeSectionDisplayRefresh();
    updateButtonSensitivity(0);
}


void uiMPEMan::redoPush( CallBacker* )
{
    if ( !EM::EMM().history().reDo( 1, mEMHistoryUserInteractionLevel) )
	uiMSG().error("Could not redo everything.");

    visserv->makeSectionDisplayRefresh();
    updateButtonSensitivity(0);
}


MPE::EMTracker* uiMPEMan::getSelectedTracker() 
{
    const TypeSet<int>& selectedids = visBase::DM().selMan().selected();
    if ( selectedids.size()!=1 || visserv->isLocked(selectedids[0]) )
	return 0;
    const MultiID mid = visserv->getMultiID( selectedids[0] );
    const EM::ObjectID oid = EM::EMM().getObjectID( mid );
    const int trackerid = MPE::engine().getTrackerByObject( oid );
    MPE::EMTracker* tracker = MPE::engine().getTracker( trackerid );

    return tracker && tracker->isEnabled() ? tracker : 0;
}


void uiMPEMan::updateButtonSensitivity( CallBacker* ) 
{
    //Undo/Redo
    toolbar->setSensitive( undoidx, EM::EMM().history().canUnDo() );
    toolbar->setSensitive( redoidx, EM::EMM().history().canReDo() );

    //Seed button
    updateSeedPickState();

//    MPE::EMTracker* tracker = getSelectedTracker();
//    MPE::EMSeedPicker* seedpicker = tracker ? tracker->getSeedPicker(true) : 0;
//    const bool isvoltrack = seedpicker ? seedpicker->doesModeUseVolume() : true;
    const bool isseedpicking = toolbar->isOn(seedidx);
    
    toolbar->setSensitive( extendidx, true );
    toolbar->setSensitive( retrackidx, true );
    toolbar->setSensitive( eraseidx, true );
    toolbar->setSensitive( moveplaneidx, true );
    toolbar->setSensitive( showcubeidx, !isseedpicking );
    toolbar->setSensitive( trackinvolidx, true );
    
    //Track forward, backward, attrib, trans, nrstep
    mGetDisplays(false);
    const bool trackerisshown = displays.size() &&
				displays[0]->isDraggerShown();

    toolbar->setSensitive( trackforwardidx, trackerisshown );
    toolbar->setSensitive( trackbackwardidx, trackerisshown );
    attribfld->setSensitive( trackerisshown );
    transfld->setSensitive( trackerisshown );
    nrstepsbox->setSensitive( trackerisshown );

    //coltab
    toolbar->setSensitive( clrtabidx, trackerisshown && !colbardlg &&
			   attribfld->currentItem()>0 );


    //trackmode buttons
    const TrackPlane::TrackMode tm = engine().trackPlane().getTrackMode();
    toolbar->turnOn( extendidx, trackerisshown && tm==TrackPlane::Extend );
    toolbar->turnOn( retrackidx, trackerisshown && tm==TrackPlane::ReTrack );
    toolbar->turnOn( eraseidx, trackerisshown && tm==TrackPlane::Erase );
    toolbar->turnOn( moveplaneidx, trackerisshown && tm==TrackPlane::Move );
}


#define mAddSeedConModeItems( seedconmodefld, typ ) \
    if ( emobj && emobj->getTypeStr() == EM##typ##TranslatorGroup::keyword ) \
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
	turnSeedPickingOn( true );
	seedpickwason = false;
    }

    const EM::EMObject* emobj = EM::EMM().getObject( tracker->objectID() );
    mAddSeedConModeItems( seedconmodefld, Horizon );
    mAddSeedConModeItems( seedconmodefld, Fault );

    seedconmodefld->setCurrentItem( seedpicker->getSeedConnectMode() );
}


void uiMPEMan::trackerAddedRemovedCB( CallBacker* )
{
    seedpickwason = false;
    if ( !engine().nrTrackersAlive() )
    {
	toolbar->turnOn( showcubeidx, false );
	showCubeCB(0);
	showTracker(false);
    }

    updateAttribNames();
}


void uiMPEMan::visObjectLockedCB()
{
    updateButtonSensitivity();
}


void uiMPEMan::trackForward( CallBacker* )
{
    uiCursorChanger cursorlock( uiCursor::Wait );
    const int nrsteps = nrstepsbox->getValue();
    mGetDisplays(false)
    const int currentevent = EM::EMM().history().currentEventNr();
    for ( int idx=0; idx<displays.size(); idx++ )
	displays[idx]->moveMPEPlane( nrsteps );
    setHistoryLevel(currentevent);
}


void uiMPEMan::trackBackward( CallBacker* )
{
    uiCursorChanger cursorlock( uiCursor::Wait );
    const int nrsteps = nrstepsbox->getValue();
    mGetDisplays(false)
    const int currentevent = EM::EMM().history().currentEventNr();
    for ( int idx=0; idx<displays.size(); idx++ )
	displays[idx]->moveMPEPlane( -nrsteps );
    setHistoryLevel(currentevent);
}


void uiMPEMan::trackInVolume( CallBacker* )
{
    mGetDisplays(false);
    for ( int idx=0; idx<displays.size(); idx++ )
	displays[idx]->updateMPEActiveVolume();
    visserv->loadPostponedData();

    const TrackPlane::TrackMode tm = engine().trackPlane().getTrackMode();
    engine().setTrackMode(TrackPlane::Extend);
    updateButtonSensitivity();
   
    NotifyStopper selstopper( EM::EMM().history().changenotifier );
    uiCursor::setOverride( uiCursor::Wait );
    PtrMan<Executor> exec = engine().trackInVolume();
    if ( exec )
    {
	const int currentevent = EM::EMM().history().currentEventNr();
	uiExecutor uiexec( toolbar, *exec );
	if ( !uiexec.go() )
	{
	    if ( engine().errMsg() )
		uiMSG().error( engine().errMsg() );
	}
	setHistoryLevel(currentevent);
    }

    uiCursor::restoreOverride();
    engine().setTrackMode(tm);
    updateButtonSensitivity();
}


void uiMPEMan::showTracker( bool yn )
{
    if ( yn && attribfld->currentItem() ) 
	visserv->loadPostponedData();

    uiCursor::setOverride( uiCursor::Wait );
    mGetDisplays(true)
    for ( int idx=0; idx<displays.size(); idx++ )
	displays[idx]->showDragger(yn);
    uiCursor::restoreOverride();
    updateButtonSensitivity();
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


void uiMPEMan::extendModeCB( CallBacker* )
{
    const bool ison = toolbar->isOn( extendidx );
    if ( ison ) 
	visserv->loadPostponedData();
    showTracker( ison );
    engine().setTrackMode( ison ? TrackPlane::Extend : TrackPlane::None );
}


void uiMPEMan::retrackModeCB( CallBacker* )
{
    const bool ison = toolbar->isOn( retrackidx );
    if ( ison ) 
	visserv->loadPostponedData();
    showTracker( ison );
    engine().setTrackMode( ison ? TrackPlane::ReTrack : TrackPlane::None );
}


void uiMPEMan::eraseModeCB( CallBacker* )
{
    const bool ison = toolbar->isOn( eraseidx );
    showTracker( ison );
    engine().setTrackMode( ison ? TrackPlane::Erase : TrackPlane::None );
}


void uiMPEMan::initFromDisplay()
{
    mGetDisplays(false)
    for ( int idx=0; idx<displays.size(); idx++ )
    {
	displays[idx]->boxDraggerStatusChange.notify(
		mCB(this,uiMPEMan,boxDraggerStatusChangeCB) );

	if ( idx==0 )
	{
	    transfld->setValue( displays[idx]->getDraggerTransparency() );
	}
    }

    updateSelectedAttrib();
    updateButtonSensitivity(0);
}


void uiMPEMan::setHistoryLevel( int preveventnr )
{
    EM::History& history = EM::EMM().history();
    const int currentevent = history.currentEventNr();
    if ( currentevent != preveventnr )
	history.setLevel(currentevent,mEMHistoryUserInteractionLevel);
}
