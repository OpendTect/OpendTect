/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2004
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uimpeman.h"

#include "attribstorprovider.h"
#include "coltabsequence.h"
#include "emobject.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "emundo.h"
#include "executor.h"
#include "horizon2dseedpicker.h"
#include "horizon3dseedpicker.h"
#include "mpeengine.h"
#include "sectionadjuster.h"
#include "sectiontracker.h"
#include "selector.h"
#include "survinfo.h"

#include "uicolortable.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiselsurvranges.h"
#include "uislider.h"
#include "uispinbox.h"
#include "uitaskrunner.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"
#include "uiviscoltabed.h"
#include "uivispartserv.h"
#include "visemobjdisplay.h"
#include "visrandomtrackdisplay.h"
#include "vismpe.h"
#include "vismpeseedcatcher.h"
#include "visselman.h"
#include "vistransform.h"
#include "vistransmgr.h"

using namespace MPE;

#define mAddButton(pm,func,tip,toggle) \
    toolbar_->addButton( pm, tip, mCB(this,uiMPEMan,func), toggle )

#define mAddMnuItm(mnu,txt,fn,fnm,idx) {\
    uiAction* itm = new uiAction( txt, mCB(this,uiMPEMan,fn) ); \
    mnu->insertItem( itm, idx ); itm->setIcon( fnm ); }


#define mGetDisplays(create) \
    ObjectSet<visSurvey::MPEDisplay> displays; \
    TypeSet<int> scenes; \
    visserv_->getChildIds( -1, scenes ); \
    for ( int idx=0; idx<scenes.size(); idx++ ) \
	displays += getDisplay( scenes[idx], create );



uiMPEMan::uiMPEMan( uiParent* p, uiVisPartServer* ps )
    : clickcatcher_(0)
    , clickablesceneid_(-1)
    , visserv_(ps)
    , propdlg_(0)
    , seedpickwason_(false)
    , oldactivevol_(false)
    , mpeintropending_(false)
    , showtexture_(true)
    , cureventnr_(mUdf(int))
    , polyselstoppedseedpick_(false)
{
    toolbar_ = new uiToolBar( p, "Tracking controls", uiToolBar::Bottom );
    addButtons();

    EM::EMM().undo().undoredochange.notify(
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
    visserv_->selectionmodechange.notify( mCB(this,uiMPEMan,selectionMode) );

    updateButtonSensitivity();
}


void uiMPEMan::addButtons()
{
    mAddButton( "tools", showSettingsCB, tr("Settings"), false );

    seedconmodefld_ = new uiComboBox( toolbar_, "Seed connect mode" );
    seedconmodefld_->setToolTip( tr("Seed connect mode") );
    seedconmodefld_->selectionChanged.notify(
				mCB(this,uiMPEMan,seedConnectModeSel) );
    toolbar_->addObject( seedconmodefld_ );
    toolbar_->addSeparator();

    seedidx_ = mAddButton( "seedpickmode", addSeedCB,
			  tr("Create seed ( key: 'Tab' )"), true );
    toolbar_->setShortcut( seedidx_, "Tab" );

    trackinvolidx_ = mAddButton( "autotrack", trackFromSeedsAndEdges,
				tr("Auto-track"), false );

    trackwithseedonlyidx_ = mAddButton( "trackfromseeds", trackFromSeedsOnly,
				       tr("Track From Seeds Only"), false );

    retrackallidx_ = mAddButton( "retrackhorizon", retrackAllCB,
				tr("Retrack All"), false );
    toolbar_->addSeparator();

    showcubeidx_ = mAddButton( "trackcube", showCubeCB,
			      tr("Show track area"), true );
    uiMenu* cubemnu = new uiMenu( toolbar_, "CubeMenu" );
    mAddMnuItm( cubemnu, tr("Position ..."), setCubePosCB, "orientation64", 0 );
    toolbar_->setButtonMenu( showcubeidx_, cubemnu );

    moveplaneidx_ = mAddButton( "QCplane-inline", movePlaneCB,
			       tr("Display QC plane"), true );
    uiMenu* mnu = new uiMenu( toolbar_, tr("Menu") );
    mAddMnuItm( mnu, uiStrings::sInline(), handleOrientationClick,
		"QCplane-inline", 0 );
    mAddMnuItm( mnu, uiStrings::sCrossline(), handleOrientationClick,
		"QCplane-crossline", 1 );
    mAddMnuItm( mnu, tr("Z"), handleOrientationClick, "QCplane-z", 2 );
    toolbar_->setButtonMenu( moveplaneidx_, mnu );

    displayatsectionidx_ = mAddButton( "sectiononly", displayAtSectionCB,
				      tr("Display at section only"), true );

    nrstepsbox_ = new uiSpinBox( toolbar_, 0, "QC plane step" );
    nrstepsbox_->setToolTip( tr("QC plane step") );
    nrstepsbox_->setMinValue( 1 );
    toolbar_->addObject( nrstepsbox_ );
    trackforwardidx_ = mAddButton( "prevpos", moveBackward,
				  tr("Move QC plane backward (key: '[')"),
                                  false );
    toolbar_->setShortcut(trackforwardidx_,"[");
    trackbackwardidx_ = mAddButton( "nextpos", moveForward,
				   tr("Move QC plane forward (key: ']')"),
                                   false );
    toolbar_->setShortcut(trackbackwardidx_,"]");
    clrtabidx_ = mAddButton( "colorbar", setColorbarCB,
			    tr("Set QC plane colortable"), false );
    toolbar_->addSeparator();

    polyselectidx_ =  mAddButton( "polygonselect", selectionMode,
				 tr("Polygon Selection mode"), true );
    uiMenu* polymnu = new uiMenu( toolbar_, "PolyMenu" );
    mAddMnuItm( polymnu,uiStrings::sPolygon(), handleToolClick, "polygonselect",
                0 );
    mAddMnuItm( polymnu,uiStrings::sRectangle(),handleToolClick,
                "rectangleselect", 1 );
    toolbar_->setButtonMenu( polyselectidx_, polymnu );

    removeinpolygonidx_ = mAddButton( "trashcan", removeInPolygon,
				  tr(" Remove PolySelection"), false );
    toolbar_->addSeparator();

    undoidx_ = mAddButton( "undo", undoPush, uiStrings::sUndo(), false );
    redoidx_ = mAddButton( "redo", redoPush, uiStrings::sRedo(), false );

    toolbar_->addSeparator();
    saveidx_ = mAddButton( "save", savePush, uiStrings::sSave(true), false );
}


uiMPEMan::~uiMPEMan()
{
    EM::EMM().undo().undoredochange.remove(
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
    visserv_->selectionmodechange.remove( mCB(this,uiMPEMan,selectionMode) );
}


void uiMPEMan::deleteVisObjects()
{
    TypeSet<int> scenes;
    visserv_->getChildIds( -1, scenes );
    for ( int idx=0; idx<scenes.size(); idx++ )
    {
	visSurvey::MPEDisplay* mped = getDisplay(scenes[idx]);
	if ( mped )
	{
	    mped->boxDraggerStatusChange.remove(
		mCB(this,uiMPEMan,boxDraggerStatusChangeCB) );
	    mped->planeOrientationChange.remove(
		mCB(this,uiMPEMan,planeOrientationChangedCB) );
	    visserv_->removeObject(mped->id(),scenes[idx]);
	}
    }

    if ( clickcatcher_ )
    {
	if ( clickablesceneid_>=0 )
	    visserv_->removeObject( clickcatcher_->id(), clickablesceneid_ );

	clickcatcher_->click.remove( mCB(this,uiMPEMan,seedClick) );
	clickcatcher_->setEditor( 0 );
	clickcatcher_->unRef();
	clickcatcher_ = 0;
	clickablesceneid_ = -1;
    }
}


#define mSeedClickReturn() \
{ endSeedClickEvent(emobj);  return; }

void uiMPEMan::seedClick( CallBacker* )
{
    EM::EMObject* emobj = 0;
    MPE::Engine& engine = MPE::engine();
    MPE::EMTracker* tracker = getSelectedTracker();
    if ( !tracker )
	mSeedClickReturn();

    emobj = EM::EMM().getObject( tracker->objectID() );
    if ( !emobj )
	mSeedClickReturn();

    const int trackerid =
		MPE::engine().getTrackerByObject( tracker->objectID() );

    const int clickedobject = clickcatcher_->info().getObjID();
    if ( clickedobject == -1 )
	mSeedClickReturn();

    if ( !clickcatcher_->info().isLegalClick() )
    {
	visBase::DataObject* dataobj = visserv_->getObject( clickedobject );
	mDynamicCastGet( visSurvey::RandomTrackDisplay*, randomdisp, dataobj );

	if ( tracker->is2D() && !clickcatcher_->info().getObjLineName() )
	    uiMSG().error( tr("2D tracking cannot handle picks on 3D lines.") );
	else if ( !tracker->is2D() && clickcatcher_->info().getObjLineName() )
	    uiMSG().error( tr("3D tracking cannot handle picks on 2D lines.") );
	else if ( randomdisp )
	    uiMSG().error( emobj->getTypeStr(),
			   tr("Tracking cannot handle picks on random lines."));
	else if ( clickcatcher_->info().getObjCS().nrZ()==1 &&
		  !clickcatcher_->info().getObjCS().isEmpty() )
	    uiMSG().error( emobj->getTypeStr(),
			   tr("Tracking cannot handle picks on time slices.") );
	mSeedClickReturn();
    }

    const EM::PosID pid = clickcatcher_->info().getNode();
    TrcKeyZSampling newvolume;
    if ( pid.objectID()!=emobj->id() && pid.objectID()!=-1 )
	mSeedClickReturn();

    const Attrib::SelSpec* clickedas =
	clickcatcher_->info().getObjDataSelSpec();
    if ( !clickedas )
	mSeedClickReturn();

    MPE::EMSeedPicker* seedpicker = tracker->getSeedPicker(true);
    if ( !seedpicker || !seedpicker->canSetSectionID() ||
	 !seedpicker->setSectionID(emobj->sectionID(0)) )
    {
	mSeedClickReturn();
    }

    const MPE::SectionTracker* sectiontracker =
	tracker->getSectionTracker(emobj->sectionID(0), true);
    const Attrib::SelSpec* trackedatsel = sectiontracker
	? sectiontracker->adjuster()->getAttributeSel(0)
	: 0;

    if ( !visserv_->isTrackingSetupActive() && (seedpicker->nrSeeds()==0) )
    {
	if ( trackedatsel &&
	     (seedpicker->getSeedConnectMode()!=seedpicker->DrawBetweenSeeds) )
	{
	    bool chanceoferror = false;
	    if ( !trackedatsel->is2D() || trackedatsel->isStored() )
		chanceoferror = !engine.isSelSpecSame(*trackedatsel,*clickedas);
	    else
		chanceoferror = !FixedString(clickedas->defString())
				.startsWith( trackedatsel->defString() );

	    if ( chanceoferror )
	    {
		uiMSG().error(tr("Saved setup has different attribute. \n"
			         "Either change setup attribute or change\n"
			         "display attribute you want to track on"));
		mSeedClickReturn();
	    }
	}
    }

    if ( seedpicker->nrSeeds() > 0 )
    {
	if ( trackedatsel &&
	     (seedpicker->getSeedConnectMode()!=seedpicker->DrawBetweenSeeds) )
	{
	    bool chanceoferror = false;
	    if ( !trackedatsel->is2D() || trackedatsel->isStored() )
		chanceoferror = !engine.isSelSpecSame(*trackedatsel,*clickedas);
	    else
		chanceoferror = !FixedString(clickedas->defString())
				.startsWith( trackedatsel->defString() );

	    if ( chanceoferror )
	    {
		uiString warnmsg = tr("Setup suggests tracking is done on: "
				      "'%1'\nbut what you see is: '%2'.\n"
				      "To continue seed picking either "
				      "change displayed attribute or\n"
				      "change input data in Tracking Setup.")
				 .arg(trackedatsel->userRef())
				 .arg(clickedas->userRef());
		uiMSG().error( warnmsg );
		mSeedClickReturn();
	    }
	}
    }

    Coord3 seedpos;
    if ( pid.objectID() == -1 )
    {
	visSurvey::Scene* scene = visSurvey::STM().currentScene();
	seedpos = clickcatcher_->info().getPos();
	scene->getTempZStretchTransform()->transformBack( seedpos );
	scene->getUTM2DisplayTransform()->transformBack( seedpos );
    }
    else
    {
	seedpos = emobj->getPos(pid);
    }

    mGetDisplays(false);
    const bool trackerisshown = displays.size() &&
			        displays[0]->isDraggerShown();

    bool ctrlshiftclicked = clickcatcher_->info().isCtrlClicked() &&
			    clickcatcher_->info().isShiftClicked();

    if ( pid.objectID()==-1 && !ctrlshiftclicked &&
	 clickcatcher_->activateSower(emobj->preferredColor(),
				     seedpicker->getSeedPickArea()) )
    {
	 mSeedClickReturn();
    }

    if ( tracker->is2D() )
    {
	Pos::GeomID geomid = clickcatcher_->info().getGeomID();
	engine.setActive2DLine( geomid );

	mDynamicCastGet( MPE::Horizon2DSeedPicker*, h2dsp, seedpicker );
	DataPack::ID datapackid = clickcatcher_->info().getObjDataPackID();

	if ( clickedas && h2dsp )
	    h2dsp->setSelSpec( clickedas );

	if ( !clickedas || !h2dsp || !h2dsp->canAddSeed(*clickedas) )
	{
	    uiMSG().error(tr("2D tracking requires attribute from setup "
			     "to be displayed"));
	    mSeedClickReturn();
	}
	if ( datapackid > DataPack::cNoID() )
	    engine.setAttribData( *clickedas, datapackid );

	h2dsp->setLine( geomid );
	if ( !h2dsp->startSeedPick() )
	    mSeedClickReturn();
    }
    else
    {
	if ( !seedpicker->startSeedPick() )
	    mSeedClickReturn();

	newvolume = clickcatcher_->info().getObjCS();
	const TrcKeyZSampling trkplanecs = engine.trackPlane().boundingBox();

	if ( trackerisshown && trkplanecs.zsamp_.includes(seedpos.z,true) &&
	     trkplanecs.hrg.includes( SI().transform(seedpos) ) &&
	     trkplanecs.defaultDir()==newvolume.defaultDir() )
	{
	    newvolume = trkplanecs;
	}

	if ( newvolume.isEmpty() || !newvolume.isDefined() )
	    mSeedClickReturn();

	if ( newvolume != engine.activeVolume() )
	{
	    if ( oldactivevol_.isEmpty() )
	    {
		if ( newvolume == trkplanecs )
		    loadPostponedData();
		else
		    engine.swapCacheAndItsBackup();

		oldactivevol_ = engine.activeVolume();
		oldtrackplane_ = engine.trackPlane();
	    }

	    NotifyStopper notifystopper( engine.activevolumechange );
	    engine.setActiveVolume( newvolume );
	    notifystopper.restore();

	    if ( clickedas && !engine.cacheIncludes(*clickedas,newvolume) )
	    {
		DataPack::ID datapackid =
				clickcatcher_->info().getObjDataPackID();
		if ( datapackid > DataPack::cNoID() )
		    engine.setAttribData( *clickedas, datapackid );
	    }

	    seedpicker->setSelSpec( clickedas );

	    for ( int idx=0; idx<displays.size(); idx++ )
		displays[idx]->freezeBoxPosition( true );  // to do:

	    if ( !seedpicker->doesModeUseSetup() )
		visserv_->toggleBlockDataLoad();

	    engine.setOneActiveTracker( tracker );
	    engine.activevolumechange.trigger();

	    if ( !seedpicker->doesModeUseSetup() )
		visserv_->toggleBlockDataLoad();
	}
    }

    seedpicker->setSowerMode( clickcatcher_->sequentSowing() );
    if ( mIsUdf(cureventnr_) && clickcatcher_->moreToSow() )
	ctrlshiftclicked = true;  // 1st seed sown is "tracking buffer" only

    beginSeedClickEvent( emobj );

    if ( pid.objectID()!=-1 )
    {
	if ( ctrlshiftclicked )
	{
	    if ( seedpicker->removeSeed( pid, false, false ) )
		engine.updateFlatCubesContainer( newvolume, trackerid, false );
	}
	else if ( clickcatcher_->info().isCtrlClicked() )
	{
	    if ( seedpicker->removeSeed( pid, true, true ) )
		engine.updateFlatCubesContainer( newvolume, trackerid, false );
	}
	else if ( clickcatcher_->info().isShiftClicked() )
	{
	    if ( seedpicker->removeSeed( pid, true, false ) )
		engine.updateFlatCubesContainer( newvolume, trackerid, false );
	}
	else
	{
	    if ( seedpicker->addSeed( seedpos, false ) )
		engine.updateFlatCubesContainer( newvolume, trackerid, true );
	}
    }
    else if ( seedpicker->addSeed(seedpos, ctrlshiftclicked) )
	    engine.updateFlatCubesContainer( newvolume, trackerid, true );

    if ( !clickcatcher_->moreToSow() )
	endSeedClickEvent( emobj );
}


void uiMPEMan::beginSeedClickEvent( EM::EMObject* emobj )
{
    if ( mIsUdf(cureventnr_) )
    {
	cureventnr_ = EM::EMM().undo().currentEventID();
	MouseCursorManager::setOverride( MouseCursor::Wait );
	if ( emobj )
	    emobj->setBurstAlert( true );
    }
}


void uiMPEMan::endSeedClickEvent( EM::EMObject* emobj )
{
    clickcatcher_->stopSowing();

    if ( !mIsUdf(cureventnr_) )
    {
	if ( emobj )
	    emobj->setBurstAlert( false );

	MouseCursorManager::restoreOverride();
	setUndoLevel( cureventnr_ );
	cureventnr_ = mUdf(int);
    }

    restoreActiveVol();
}


void uiMPEMan::restoreActiveVolume()
{
    restoreActiveVol();
}


void uiMPEMan::restoreActiveVol()
{
    MPE::Engine& engine = MPE::engine();
    if ( !oldactivevol_.isEmpty() )
    {
	if ( engine.activeVolume() != oldtrackplane_.boundingBox() )
	    engine.swapCacheAndItsBackup();
	NotifyStopper notifystopper( engine.activevolumechange );
	engine.setActiveVolume( oldactivevol_ );
	notifystopper.restore();
	engine.setTrackPlane( oldtrackplane_, false );
	engine.unsetOneActiveTracker();
	engine.activevolumechange.trigger();

	mGetDisplays(false);
	for ( int idx=0; idx<displays.size(); idx++ )
	    displays[idx]->freezeBoxPosition( false );

	oldactivevol_.setEmpty();
    }
}


uiToolBar* uiMPEMan::getToolBar() const
{
    return toolbar_;
}


visSurvey::MPEDisplay* uiMPEMan::getDisplay( int sceneid, bool create )
{
    mDynamicCastGet(const visSurvey::Scene*,scene,visserv_->getObject(sceneid));
    if ( !scene ) return 0;

    TypeSet<int> displayids;
    visserv_->findObject( typeid(visSurvey::MPEDisplay), displayids );

    for ( int idx=0; idx<displayids.size(); idx++ )
    {
	if ( scene->getFirstIdx(displayids[idx]) == -1 )
	    continue;

	visBase::DataObject* dobj = visserv_->getObject( displayids[idx] );
	return reinterpret_cast<visSurvey::MPEDisplay*>( dobj );
    }

    if ( !create ) return 0;

    visSurvey::MPEDisplay* mpedisplay = new visSurvey::MPEDisplay;

    visserv_->addObject( mpedisplay, scene->id(), false ); // false or true?
    mpedisplay->setDraggerTransparency( 0 ); // to do: check 0
    mpedisplay->showDragger( toolbar_->isOn(moveplaneidx_) );
    mpedisplay->setColTabSequence( 0, ColTab::Sequence(
			    ColTab::defSeqName() ), 0 );

    mpedisplay->boxDraggerStatusChange.notify(
	    mCB(this,uiMPEMan,boxDraggerStatusChangeCB) );

    mpedisplay->planeOrientationChange.notify(
	    mCB(this,uiMPEMan,planeOrientationChangedCB) );

    return mpedisplay;
}


bool uiMPEMan::isSeedPickingOn() const
{
    return clickcatcher_ && clickcatcher_->isOn();
}


bool uiMPEMan::isPickingWhileSetupUp() const
{
    return isSeedPickingOn() &&
	visserv_->isTrackingSetupActive();
}


void uiMPEMan::updateOldActiveVol()
{
    if ( oldactivevol_.isEmpty() )
    {
	MPE::engine().swapCacheAndItsBackup();
	oldactivevol_ = MPE::engine().activeVolume();
	oldtrackplane_ = MPE::engine().trackPlane();
    }
}


void uiMPEMan::turnSeedPickingOn( bool yn )
{
    polyselstoppedseedpick_ = false;

    if ( !yn && clickcatcher_ )
	clickcatcher_->setEditor( 0 );

    if ( isSeedPickingOn() == yn )
	return;

    toolbar_->turnOn( seedidx_, yn );
    MPE::EMTracker* tracker = getSelectedTracker();

    if ( yn )
    {
	toolbar_->turnOn( polyselectidx_, false );
	selectionMode(0);

	visserv_->setViewMode(false);
	toolbar_->turnOn( showcubeidx_, false );
	showCubeCB(0);

	updateClickCatcher();
	clickcatcher_->turnOn( true );

	const EM::EMObject* emobj =
			tracker ? EM::EMM().getObject(tracker->objectID()) : 0;

	if ( emobj )
	    clickcatcher_->setTrackerType( emobj->getTypeStr() );
    }
    else
    {
	MPE::EMSeedPicker* seedpicker = tracker ?
				        tracker->getSeedPicker(true) : 0;
	if ( seedpicker )
	    seedpicker->stopSeedPick();

	if ( clickcatcher_ )
	    clickcatcher_->turnOn( false );

	restoreActiveVol();
    }

    mGetDisplays( false );
    for ( int idx=0; idx<displays.size(); idx++ )
	displays[idx]->enablePicking( yn );

    visserv_->sendPickingStatusChangeEvent();
}


void uiMPEMan::updateClickCatcher()
{
    if ( !clickcatcher_ )
    {
	TypeSet<int> catcherids;
	visserv_->findObject( typeid(visSurvey::MPEClickCatcher),
			     catcherids );
	if ( catcherids.size() )
	{
	    visBase::DataObject* dobj = visserv_->getObject( catcherids[0] );
	    clickcatcher_ = reinterpret_cast<visSurvey::MPEClickCatcher*>(dobj);
	}
	else
	{
	    clickcatcher_ = visSurvey::MPEClickCatcher::create();
	}
	clickcatcher_->ref();
	clickcatcher_->click.notify(mCB(this,uiMPEMan,seedClick));
	clickcatcher_->turnOn( false );
    }

    const TypeSet<int>& selectedids = visBase::DM().selMan().selected();
    if ( selectedids.size() != 1 )
	return;

    mDynamicCastGet( visSurvey::EMObjectDisplay*,
		     surface, visserv_->getObject(selectedids[0]) );
    clickcatcher_->setEditor( surface ? surface->getEditor() : 0 );

    const int newsceneid = visserv_->getSceneID( selectedids[0] );
    if ( newsceneid<0 || newsceneid == clickablesceneid_ )
	return;

    visserv_->removeObject( clickcatcher_->id(), clickablesceneid_ );
    visserv_->addObject( clickcatcher_, newsceneid, false );
    clickablesceneid_ = newsceneid;
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
	    displays[idx]->updateMPEActiveVolume();  // to do
    }

    toolbar_->turnOn( showcubeidx_, ison );
}


void uiMPEMan::showCubeCB( CallBacker* )
{
    const bool isshown = toolbar_->isOn( showcubeidx_ );
    if ( isshown)
	turnSeedPickingOn( false );

    if ( isshown )
    {
	bool isvolflat = false;
	TrcKeyZSampling cube = MPE::engine().activeVolume();
	if ( cube.isFlat() )
	{
	    cube = MPE::engine().getDefaultActiveVolume();
	    isvolflat = true;
	}

	if ( cube == MPE::engine().getDefaultActiveVolume() )
	{
	    bool seedincluded = false;
	    MPE::EMTracker* tracker = getSelectedTracker();
	    if ( tracker )
	    {
		EM::EMObject* emobj = EM::EMM().getObject( tracker->objectID());
		if ( emobj )
		{
		    const TypeSet<EM::PosID>* seeds =
			emobj->getPosAttribList( EM::EMObject::sSeedNode() );
		    if ( seeds->size() > 0 )
			seedincluded = true;
		    for ( int idx=0; idx<seeds->size(); idx++ )
		    {
			const Coord3 pos = emobj->getPos( (*seeds)[idx] );
			const BinID bid = SI().transform(pos);
			cube.hrg.include(bid);
			cube.zsamp_.include((float) pos.z);
		    }
		}
	    }

	    if ( isvolflat || seedincluded )
	    {
		NotifyStopper notifystopper( MPE::engine().activevolumechange );
		MPE::engine().setActiveVolume( cube );
		notifystopper.restore();
		visserv_->postponedLoadingData();
		MPE::engine().activevolumechange.trigger();
	    }
	}
    }

    mGetDisplays(isshown)
    for ( int idx=0; idx<displays.size(); idx++ )
	displays[idx]->showBoxDragger( isshown );

    toolbar_->setToolTip( showcubeidx_, isshown ? tr("Hide track area")
					      : tr("Show track area") );
    MPE::engine().setActiveVolShown( isshown );
}



class uiSetCubePosDlg : public uiDialog
{ mODTextTranslationClass(uiSetCubePosDlg);
public:
uiSetCubePosDlg( uiParent* p, const TrcKeyZSampling& cs )
    : uiDialog(p,uiDialog::Setup(tr("Set Cube Position"),mNoDlgTitle,
                                    mTODOHelpKey))
{
    selfld_ = new uiSelSubvol( this, false );
    selfld_->setSampling( cs );
    fullbut_ = new uiToolButton( this, "exttofullsurv",
				tr("Set ranges to full survey"),
				 mCB(this,uiSetCubePosDlg,fullPush) );
    fullbut_->attach( rightOf, selfld_ );
}

void fullPush( CallBacker* )
{
    selfld_->setSampling( SI().sampling(false) );
}

TrcKeyZSampling getSampling() const
{ return selfld_->getSampling(); }

    uiSelSubvol*	selfld_;
    uiToolButton*	fullbut_;
};


void uiMPEMan::setCubePosCB( CallBacker* )
{
    const TrcKeyZSampling& cs = MPE::engine().activeVolume();
    uiSetCubePosDlg dlg( toolbar_, cs );
    if ( !dlg.go() ) return;

    NotifyStopper notifystopper( MPE::engine().activevolumechange );
    MPE::engine().setActiveVolume( dlg.getSampling() );
    notifystopper.restore();
    visserv_->postponedLoadingData();
    MPE::engine().activevolumechange.trigger();
}


void uiMPEMan::addSeedCB( CallBacker* )
{
    turnSeedPickingOn( toolbar_->isOn(seedidx_) );
    updateButtonSensitivity(0);
}


void uiMPEMan::updateSeedModeSel()
{
    MPE::EMTracker* tracker = getSelectedTracker();
    MPE::EMSeedPicker* seedpicker = tracker ? tracker->getSeedPicker(true) : 0;
    if ( seedpicker )
	seedconmodefld_->setCurrentItem( seedpicker->getSeedConnectMode() );
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

    const int oldseedconmodefld_ = seedpicker->getSeedConnectMode();
    seedpicker->setSeedConnectMode( seedconmodefld_->currentItem() );

    if ( seedpicker->doesModeUseSetup() )
    {
	const SectionTracker* sectiontracker =
	      tracker->getSectionTracker( emobj->sectionID(0), true );
	if ( sectiontracker && !sectiontracker->hasInitializedSetup() )
	    visserv_->sendShowSetupDlgEvent();
	if ( !sectiontracker || !sectiontracker->hasInitializedSetup() )
	    seedpicker->setSeedConnectMode( oldseedconmodefld_ );
    }

    turnSeedPickingOn( true );
    visserv_->setViewMode(false);
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
    if ( visserv_->isTrackingSetupActive() )
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

    toolbar_->turnOn( showcubeidx_, true );
    showCubeCB(0);

    attribSel(0);

    mpeintropending_ = true;
}


void uiMPEMan::finishMPEDispIntro( CallBacker* )
{
    if ( !mpeintropending_ || !oldactivevol_.isEmpty() )
	return;

    mpeintropending_ = false;

    mGetDisplays(false);
    if ( !displays.size() || !displays[0]->isDraggerShown() )
	return;

    EMTracker* tracker = getSelectedTracker();
    if ( !tracker)
	tracker = engine().getTracker( engine().highestTrackerID() );

    if ( !tracker )
	return;

    ObjectSet<const Attrib::SelSpec> attribspecs;
    tracker->getNeededAttribs(attribspecs);
    if ( attribspecs.isEmpty() )
	return;

    attribSel(0);
}


void uiMPEMan::loadPostponedData()
{
    visserv_->loadPostponedData();
    finishMPEDispIntro( 0 );
}


void uiMPEMan::undoPush( CallBacker* )
{
    MouseCursorChanger mcc( MouseCursor::Wait );

    mDynamicCastGet( EM::EMUndo*, emundo, &EM::EMM().undo() );
    if ( emundo )
    {
	EM::ObjectID curid = emundo->getCurrentEMObjectID( false );
	EM::EMObject* emobj = EM::EMM().getObject( curid );
	if ( emobj )
        {
            emobj->ref();
	    emobj->setBurstAlert( true );
        }

        if ( !emundo->unDo(1,true) )
	    uiMSG().error( tr("Could not undo everything.") );

	if ( emobj )
        {
	    emobj->setBurstAlert( false );
            emobj->unRef();
        }
    }

    updateButtonSensitivity(0);
}


void uiMPEMan::redoPush( CallBacker* )
{
    MouseCursorChanger mcc( MouseCursor::Wait );

    mDynamicCastGet( EM::EMUndo*, emundo, &EM::EMM().undo() );
    if ( emundo )
    {
	EM::ObjectID curid = emundo->getCurrentEMObjectID( true );
	EM::EMObject* emobj = EM::EMM().getObject( curid );
        if ( emobj )
        {
            emobj->ref();
	    emobj->setBurstAlert( true );
        }

	if ( !emundo->reDo(1,true) )
	    uiMSG().error( tr("Could not redo everything.") );

	if ( emobj )
        {
	    emobj->setBurstAlert( false );
            emobj->unRef();
        }
    }

    updateButtonSensitivity(0);
}


void uiMPEMan::savePush( CallBacker* )
{
    MPE::EMTracker* tracker = getSelectedTracker();
    if ( !tracker )
	return;

    visserv_->fireFromMPEManStoreEMObject();
    toolbar_->setSensitive( saveidx_, false );
}


MPE::EMTracker* uiMPEMan::getSelectedTracker()
{
    const TypeSet<int>& selectedids = visBase::DM().selMan().selected();
    if ( selectedids.size()!=1 || visserv_->isLocked(selectedids[0]) )
	return 0;

    mDynamicCastGet( visSurvey::EMObjectDisplay*,
				surface, visserv_->getObject(selectedids[0]) );
    if ( !surface ) return 0;
    const EM::ObjectID oid = surface->getObjectID();
    const int trackerid = MPE::engine().getTrackerByObject( oid );
    MPE::EMTracker* tracker = MPE::engine().getTracker( trackerid );
    if ( tracker && tracker->isEnabled() )
	return tracker;

    return 0;
}


#define mAddSeedConModeItems( seedconmodefld_, typ ) \
    if ( emobj && EM##typ##TranslatorGroup::keyword() == emobj->getTypeStr() ) \
    { \
	seedconmodefld_->setEmpty(); \
	for ( int idx=0; idx<typ##SeedPicker::nrSeedConnectModes(); idx++ ) \
	{ \
	    seedconmodefld_-> \
		    addItem( typ##SeedPicker::seedConModeText(idx,true) ); \
	} \
	if ( typ##SeedPicker::nrSeedConnectModes()<=0 ) \
	    seedconmodefld_->addItem("No seed mode"); \
    }


void uiMPEMan::updateSeedPickState()
{
    MPE::EMTracker* tracker = getSelectedTracker();
    MPE::EMSeedPicker* seedpicker = tracker ? tracker->getSeedPicker(true) : 0;

    toolbar_->setSensitive( seedidx_, seedpicker );
    seedconmodefld_->setSensitive( seedpicker );
    seedconmodefld_->setEmpty();

    if ( !seedpicker )
    {
	seedconmodefld_->addItem(tr("No seed mode"));
	if ( isSeedPickingOn() )
	{
	    turnSeedPickingOn( false );
	    seedpickwason_ = true;
	}
	return;
    }

    if ( seedpickwason_ )
    {
	seedpickwason_ = false;
	turnSeedPickingOn( true );
    }

    const EM::EMObject* emobj = EM::EMM().getObject( tracker->objectID() );
    mAddSeedConModeItems( seedconmodefld_, Horizon3D );
    mAddSeedConModeItems( seedconmodefld_, Horizon2D );

    seedconmodefld_->setCurrentItem( seedpicker->getSeedConnectMode() );
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
	seedpickwason_ = false;
	toolbar_->turnOn( showcubeidx_, false );
	showCubeCB(0);
	showTracker(false);
	engine().setActiveVolume( engine().getDefaultActiveVolume() );
    }
}


void uiMPEMan::visObjectLockedCB( CallBacker* )
{
    updateButtonSensitivity();
}


void uiMPEMan::moveForward( CallBacker* )
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );
    const int nrsteps = nrstepsbox_->getValue();
    mGetDisplays(false)
    const int currentevent = EM::EMM().undo().currentEventID();
    for ( int idx=0; idx<displays.size(); idx++ )
	displays[idx]->moveMPEPlane( nrsteps );
    setUndoLevel(currentevent);
}


void uiMPEMan::moveBackward( CallBacker* )
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );
    const int nrsteps = nrstepsbox_->getValue();
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
    Executor* exec = engine().trackInVolume();
    if ( exec )
    {
	const int currentevent = EM::EMM().undo().currentEventID();
	uiTaskRunner uitr( toolbar_ );
	if ( !TaskRunner::execute( &uitr, *exec ) )
	{
	    if ( engine().errMsg() )
		uiMSG().error( engine().errMsg() );
	}
	delete exec;	// AutoTracker destructor adds the undo event!
	setUndoLevel(currentevent);
    }

    MouseCursorManager::restoreOverride();
    engine().setTrackMode(tm);
    updateButtonSensitivity();
}


static bool sIsPolySelect = true;

void uiMPEMan::selectionMode( CallBacker* cb )
{
    if ( cb == visserv_ )
    {
	toolbar_->turnOn( polyselectidx_, visserv_->isSelectionModeOn() );
	sIsPolySelect = visserv_->getSelectionMode()==uiVisPartServer::Polygon;
    }
    else
    {
	uiVisPartServer::SelectionMode mode = sIsPolySelect ?
			 uiVisPartServer::Polygon : uiVisPartServer::Rectangle;
	visserv_->turnSelectionModeOn( toolbar_->isOn(polyselectidx_) );
	visserv_->setSelectionMode( mode );
    }

    toolbar_->setIcon( polyselectidx_, sIsPolySelect ?
			"polygonselect" : "rectangleselect" );
    toolbar_->setToolTip( polyselectidx_,
                         sIsPolySelect ? tr("Polygon Selection mode")
                                       : tr("Rectangle Selection mode") );

    if ( toolbar_->isOn(polyselectidx_) )
    {
	if ( toolbar_->isOn(seedidx_) )
	{
	    visserv_->turnSeedPickingOn( false );
	    polyselstoppedseedpick_ = true;
	}
    }
    else if ( polyselstoppedseedpick_ )
	visserv_->turnSeedPickingOn( true );

    updateButtonSensitivity(0);
}


void uiMPEMan::handleToolClick( CallBacker* cb )
{
    mDynamicCastGet(uiAction*,itm,cb)
    if ( !itm ) return;

    sIsPolySelect = itm->getID()==0;
    selectionMode( cb );
}


void uiMPEMan::removeInPolygon( CallBacker* cb )
{
    const Selector<Coord3>* sel =
	visserv_->getCoordSelector( clickablesceneid_ );

    if ( sel && sel->isOK() )
    {
	const int currentevent = EM::EMM().undo().currentEventID();
	mGetDisplays(true);
	uiTaskRunner taskrunner( toolbar_ );
	for ( int idx=0; idx<displays.size(); idx++ )
	    displays[idx]->removeSelectionInPolygon( *sel, &taskrunner );

	toolbar_->turnOn( polyselectidx_, false );
	selectionMode( cb );

	toolbar_->turnOn( moveplaneidx_, false );
	movePlaneCB( cb );
	setUndoLevel( currentevent );
    }
}


void uiMPEMan::showTracker( bool yn )
{
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
	toolbar_->turnOn( showcubeidx_, false );
	showCubeCB(0);
	showTracker( false );
	engine().setActiveVolume( engine().getDefaultActiveVolume() );
    }
}


class uiPropertiesDialog : public uiDialog
{ mODTextTranslationClass(uiPropertiesDialog);
public:
				uiPropertiesDialog(uiMPEMan*);

    uiVisColTabEd&              editor()        { return *coltbl_; }
    int				selAttrib() const
				{ return attribfld_->currentItem(); }
    void			updateAttribNames();

protected:
    void			transpChg(CallBacker*);
    void			colSeqChange(CallBacker*);
    void			colMapperChange(CallBacker*);
    void			updateSelectedAttrib();
    void			updateDisplayList();

    uiVisColTabEd*		coltbl_;
    uiComboBox*			attribfld_;
    uiSlider*			transfld_;
    uiMPEMan*			mpeman_;

    ObjectSet<visSurvey::MPEDisplay> displays_;
};


uiPropertiesDialog::uiPropertiesDialog( uiMPEMan* mpeman )
    : uiDialog( mpeman->getToolBar(),
	    uiDialog::Setup(tr("QC display properties"),
			    uiStrings::sEmptyString(),
                            mNoHelpKey).modal(false))
    , mpeman_(mpeman)
{
    updateDisplayList();
    setCtrlStyle( CloseOnly );

    ColTab::Sequence ctseq( "" );
    uiColorTableGroup* coltabgrp =
	new uiColorTableGroup( this, ctseq, false, false );

    coltbl_ = new uiVisColTabEd( *coltabgrp );
    coltbl_->seqChange().notify( mCB(this,uiPropertiesDialog,colSeqChange) );
    coltbl_->mapperChange().notify( mCB(this,uiPropertiesDialog,
					colMapperChange) );

    uiLabeledComboBox* lcb = new uiLabeledComboBox( this, tr("QC Attribute"),
						    "QC Attribute" );
    attribfld_ = lcb->box();
    attribfld_->setToolTip( tr("QC Attribute") );
    attribfld_->selectionChanged.notify( mCB(mpeman_,uiMPEMan,attribSel) );
    lcb->attach( leftAlignedBelow, coltabgrp );

    transfld_ = new uiSlider( this, uiSlider::Setup(uiStrings::sTransparency())
				    .nrdec(2), "Slider" );
    transfld_->setOrientation( OD::Horizontal );
    transfld_->setMaxValue( 1 );
    transfld_->setToolTip( uiStrings::sTransparency() );
    transfld_->setStretch( 0, 0 );
    transfld_->setValue( displays_[0]->getDraggerTransparency() );
    transfld_->valueChanged.notify( mCB(this,uiPropertiesDialog,transpChg) );
    transfld_->attach( alignedBelow, lcb );

    updateAttribNames();
}


void uiPropertiesDialog::updateDisplayList()
{
    displays_.erase();
    TypeSet<int> scenes;
    mpeman_->visserv_->getChildIds( -1, scenes );
    for ( int idx=0; idx<scenes.size(); idx++ )
	displays_ += mpeman_->getDisplay( scenes[idx], false );
}


void uiPropertiesDialog::transpChg( CallBacker* )
{
    updateDisplayList();
    if ( displays_.size() < 1 )
	return;

    for ( int idx=0; idx<displays_.size(); idx++ )
	displays_[idx]->setDraggerTransparency( transfld_->getValue() );
}

void uiPropertiesDialog::colSeqChange( CallBacker* )
{
    updateDisplayList();
    if ( displays_.size()<1 )
	return;

    displays_[0]->setColTabSequence( 0, coltbl_->getColTabSequence(), 0 );
}


void uiPropertiesDialog::colMapperChange( CallBacker* )
{
    updateDisplayList();
    if ( displays_.size()<1 )
	return;

    displays_[0]->setColTabMapperSetup( 0, coltbl_->getColTabMapperSetup(), 0 );
}


void uiPropertiesDialog::updateAttribNames()
{
    BufferString oldsel = attribfld_->text();
    attribfld_->setEmpty();
    attribfld_->addItem( mpeman_->sKeyNoAttrib() );

    ObjectSet<const Attrib::SelSpec> attribspecs;
    engine().getNeededAttribs( attribspecs );
    for ( int idx=0; idx<attribspecs.size(); idx++ )
    {
	const Attrib::SelSpec* spec = attribspecs[idx];
	attribfld_->addItem( spec->userRef() );
    }
    attribfld_->setCurrentItem( oldsel );

    updateSelectedAttrib();
    mpeman_->attribSel(0);
}


void uiPropertiesDialog::updateSelectedAttrib()
{
    updateDisplayList();
    if ( displays_.isEmpty() )
	return;

    ObjectSet<const Attrib::SelSpec> attribspecs;
    engine().getNeededAttribs( attribspecs );

    const char* userref = displays_[0]->getSelSpecUserRef();
    if ( !userref && !attribspecs.isEmpty() )
    {
	for ( int idx=0; idx<displays_.size(); idx++ )
	    displays_[idx]->setSelSpec( 0, *attribspecs[0] );

	userref = displays_[0]->getSelSpecUserRef();
    }
    else if ( FixedString(userref) == sKey::None() )
	userref = mpeman_->sKeyNoAttrib();

    if ( userref )
	attribfld_->setCurrentItem( userref );
}


void uiMPEMan::attribSel( CallBacker* )
{
    mGetDisplays(false);
    const bool trackerisshown = displays.size() &&
			        displays[0]->isDraggerShown();
    if ( trackerisshown && showtexture_ )
	loadPostponedData();

    if ( propdlg_ )
	showtexture_ = propdlg_->selAttrib() > 0;

    MouseCursorChanger cursorchanger( MouseCursor::Wait );

    if ( !showtexture_ )
    {
	for ( int idy=0; idy<displays.size(); idy++ )
	{
	    Attrib::SelSpec spec( 0, Attrib::SelSpec::cNoAttrib() );
	    displays[idy]->setSelSpec( 0, spec );
	    if ( trackerisshown )
		displays[idy]->updateSlice();
	}
    }
    else
    {
	ObjectSet<const Attrib::SelSpec> attribspecs;
	engine().getNeededAttribs( attribspecs );
	for ( int idx=0; idx<attribspecs.size(); idx++ )
	{
	    const Attrib::SelSpec* spec = attribspecs[idx];
	    if ( !spec )
		continue;

	    for ( int idy=0; idy<displays.size(); idy++ )
	    {
		displays[idy]->setSelSpec( 0, *spec );
		if ( trackerisshown )
		    displays[idy]->updateSlice();
	    }
	    break;
	}
    }

    if ( displays.size() && propdlg_ )
    {
	propdlg_->editor().setColTab( displays[0], 0, mUdf(int) );
    }
}


void uiMPEMan::setColorbarCB( CallBacker* )
{
    mGetDisplays(false);

    if ( displays.size()<1 )
	return;

    if ( !propdlg_ )
    {
	propdlg_ = new uiPropertiesDialog( this );
	propdlg_->windowClosed.notify( mCB(this,uiMPEMan,onColTabClosing) );
    }

    propdlg_->editor().setColTab( displays[0], 0, mUdf(int) );
    propdlg_->updateAttribNames();
    propdlg_->show();
    toolbar_->setSensitive( clrtabidx_, false );
}


void uiMPEMan::onColTabClosing( CallBacker* )
{
    toolbar_->setSensitive( clrtabidx_, true );
}


void uiMPEMan::movePlaneCB( CallBacker* )
{
    const bool ison = toolbar_->isOn( moveplaneidx_ );
    showTracker( ison );
    engine().setTrackMode( ison ? TrackPlane::Move : TrackPlane::None );

    if ( ison )
    {
	toolbar_->setToolTip( moveplaneidx_, tr("Hide QC plane") );
	attribSel(0);
    }
}


void uiMPEMan::turnQCPlaneOff()
{
    const bool ison = toolbar_->isOn( moveplaneidx_ );

    if ( ison )
    {
	toolbar_->turnOn( moveplaneidx_, false );
	movePlaneCB(0);
    }
}


static void updateQCButton( uiToolBar* tb, int butidx, int dim )
{
    BufferString pm, tooltip;
    if ( dim == 0 )
	{ pm = "QCplane-inline"; tooltip = "Display QC plane Inline"; }
    else if ( dim == 1 )
	{ pm = "QCplane-crossline"; tooltip = "Display QC plane Crossline"; }
    else
	{ pm = "QCplane-z"; tooltip = "Display QC plane Z-dir"; }

    tb->setIcon( butidx, pm );
    tb->setToolTip( butidx, tooltip );
}


void uiMPEMan::handleOrientationClick( CallBacker* cb )
{
    mDynamicCastGet(uiAction*,itm,cb)
    if ( !itm ) return;
    const int dim = itm->getID();
    updateQCButton( toolbar_, moveplaneidx_, dim );
    changeTrackerOrientation( dim );
    movePlaneCB( cb );
}


void uiMPEMan::planeOrientationChangedCB( CallBacker* cb )
{
    mDynamicCastGet(visSurvey::MPEDisplay*,disp,cb)
    if ( !disp ) return;

    const int dim = disp->getPlaneOrientation();
    updateQCButton( toolbar_, moveplaneidx_, dim );
}


void uiMPEMan::showSettingsCB( CallBacker* )
{
    visserv_->sendShowSetupDlgEvent();
}


void uiMPEMan::displayAtSectionCB( CallBacker* )
{
    MPE::EMTracker* tracker = getSelectedTracker();
    if ( !tracker ) return;

    const TypeSet<int>& selectedids = visBase::DM().selMan().selected();
    if ( selectedids.size()!=1 || visserv_->isLocked(selectedids[0]) )
	return;

    mDynamicCastGet( visSurvey::EMObjectDisplay*,
		     surface, visserv_->getObject(selectedids[0]) );

    bool ison = toolbar_->isOn(displayatsectionidx_);

    if ( surface && (surface->getObjectID()== tracker->objectID()) )
	surface->setOnlyAtSectionsDisplay( ison );

    toolbar_->setToolTip( displayatsectionidx_,
                         ison ? tr("Display full")
                              : tr("Display at section only") );
}


void uiMPEMan::retrackAllCB( CallBacker* )
{
    MPE::EMTracker* tracker = getSelectedTracker();
    if ( !tracker ) return;

    MPE::EMSeedPicker* seedpicker = tracker->getSeedPicker( true );
    if ( !seedpicker) return;

    Undo& undo = EM::EMM().undo();
    int cureventnr = undo.currentEventID();
    undo.setUserInteractionEnd( cureventnr, false );

    MouseCursorManager::setOverride( MouseCursor::Wait );
    EM::EMObject* emobj = EM::EMM().getObject( tracker->objectID() );
    emobj->setBurstAlert( true );
    emobj->removeAllUnSeedPos();

    TrcKeyZSampling realactivevol = MPE::engine().activeVolume();
    ObjectSet<TrcKeyZSampling>* trackedcubes =
	MPE::engine().getTrackedFlatCubes(
	    MPE::engine().getTrackerByObject(tracker->objectID()) );
    if ( trackedcubes )
    {
	for ( int idx=0; idx<trackedcubes->size(); idx++ )
	{
	    NotifyStopper notifystopper( MPE::engine().activevolumechange );
	    MPE::engine().setActiveVolume( *(*trackedcubes)[idx] );
	    notifystopper.restore();

	    const TrcKeyZSampling curvol =  MPE::engine().activeVolume();
	    if ( curvol.nrInl()==1 || curvol.nrCrl()==1 )
		visserv_->fireLoadAttribDataInMPEServ();

	    seedpicker->reTrack();
	}

	emobj->setBurstAlert( false );
	deepErase( *trackedcubes );

	MouseCursorManager::restoreOverride();
	undo.setUserInteractionEnd( undo.currentEventID() );

	MPE::engine().setActiveVolume( realactivevol );

	if ( !(MPE::engine().activeVolume().nrInl()==1) &&
	     !(MPE::engine().activeVolume().nrCrl()==1) )
	    trackInVolume(0);
    }
    else
	seedpicker->reTrack();
	emobj->setBurstAlert( false );
	MouseCursorManager::restoreOverride();
	undo.setUserInteractionEnd( undo.currentEventID() );
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

	displays[idx]->planeOrientationChange.notify(
		mCB(this,uiMPEMan,planeOrientationChangedCB) );

	if ( idx==0 )
	    toolbar_->turnOn( showcubeidx_, displays[idx]->isBoxDraggerShown());
    }

    bool showtracker = engine().trackPlane().getTrackMode()!=TrackPlane::None;
    if ( !engine().nrTrackersAlive() )
    {
	engine().setTrackMode( TrackPlane::None );
	showtracker = false;
    }

    showTracker( showtracker );
    updateButtonSensitivity(0);
}


void uiMPEMan::trackInVolume()
{ trackInVolume(0); }


void uiMPEMan::setUndoLevel( int preveventnr )
{
    Undo& undo = EM::EMM().undo();
    const int currentevent = undo.currentEventID();
    if ( currentevent != preveventnr )
	    undo.setUserInteractionEnd(currentevent);
}


void uiMPEMan::updateButtonSensitivity( CallBacker* )
{
    //Undo/Redo
    BufferString tooltip("Undo ");
    if ( EM::EMM().undo().canUnDo() )
	tooltip += EM::EMM().undo().unDoDesc();
    toolbar_->setToolTip( undoidx_, tooltip.buf() );
    toolbar_->setSensitive( undoidx_, EM::EMM().undo().canUnDo() );

    tooltip = "Redo ";
    if ( EM::EMM().undo().canReDo() )
	tooltip += EM::EMM().undo().reDoDesc();
    toolbar_->setToolTip( redoidx_, tooltip.buf() );
    toolbar_->setSensitive( redoidx_, EM::EMM().undo().canReDo() );

    //Seed button
    updateSeedPickState();

    MPE::EMTracker* tracker = getSelectedTracker();
    MPE::EMSeedPicker* seedpicker = tracker ? tracker->getSeedPicker(true) : 0;
    mDynamicCastGet(MPE::Horizon2DSeedPicker*,sp2d,seedpicker)
    const bool is2d = sp2d;
    toolbar_->setSensitive( moveplaneidx_, !is2d );
    toolbar_->setSensitive( showcubeidx_, !is2d );

    const bool isinvolumemode = seedpicker && seedpicker->doesModeUseVolume();
    toolbar_->setSensitive( trackinvolidx_,
	    !is2d && isinvolumemode && seedpicker );
    toolbar_->setSensitive( trackwithseedonlyidx_,
	    !is2d && isinvolumemode && seedpicker );
    toolbar_->setSensitive( displayatsectionidx_, seedpicker );

    toolbar_->setSensitive( removeinpolygonidx_,
			    toolbar_->isOn(polyselectidx_) );

    //Track forward, backward, attrib, trans, nrstep
    mGetDisplays(false);
    const bool trackerisshown = displays.size() &&
				displays[0]->isDraggerShown();

    toolbar_->setSensitive( trackforwardidx_, !is2d && trackerisshown );
    toolbar_->setSensitive( trackbackwardidx_, !is2d && trackerisshown );
    nrstepsbox_->setSensitive( !is2d && trackerisshown );

    //coltab
    const bool hasdlg = propdlg_ && !propdlg_->isHidden();
    toolbar_->setSensitive( clrtabidx_, !is2d && trackerisshown && !hasdlg );

    const TrackPlane::TrackMode tm = engine().trackPlane().getTrackMode();
    toolbar_->turnOn( moveplaneidx_, trackerisshown && tm==TrackPlane::Move );

    toolbar_->setSensitive( tracker );
    if ( seedpicker &&
	    !(visserv_->isTrackingSetupActive() && (seedpicker->nrSeeds()<1)) )
	toolbar_->setSensitive( true );
}

