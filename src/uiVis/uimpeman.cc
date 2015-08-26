/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2004
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uimpeman.h"

#include "attribdescset.h"
#include "attribdescsetsholder.h"
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
#include "seisdatapack.h"
#include "seispreload.h"
#include "selector.h"
#include "survinfo.h"

#include "uicombobox.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uistrings.h"
#include "uitaskrunner.h"
#include "uitoolbar.h"
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


uiMPEMan::uiMPEMan( uiParent* p, uiVisPartServer* ps )
    : clickcatcher_(0)
    , clickablesceneid_(-1)
    , visserv_(ps)
    , seedpickwason_(false)
    , oldactivevol_(false)
    , mpeintropending_(false)
    , cureventnr_(mUdf(int))
    , polyselstoppedseedpick_(false)
{
    toolbar_ = new uiToolBar( p, "Tracking controls", uiToolBar::Bottom );
    addButtons();

    EM::EMM().undo().undoredochange.notify(
			mCB(this,uiMPEMan,updateButtonSensitivity) );
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
    visserv_->selectionmodeChange.notify( mCB(this,uiMPEMan,selectionMode) );

    updateButtonSensitivity();
}


void uiMPEMan::addButtons()
{
    mAddButton( "tools", showSettingsCB, uiStrings::sSettings(), false );

    seedconmodefld_ = new uiComboBox( toolbar_, "Seed connect mode" );
    seedconmodefld_->setToolTip( tr("Seed connect mode") );
    seedconmodefld_->selectionChanged.notify(
				mCB(this,uiMPEMan,seedConnectModeSel) );
    toolbar_->addObject( seedconmodefld_ );
    toolbar_->addSeparator();

    seedidx_ = mAddButton( "seedpickmode", addSeedCB,
			  tr("Create seed ( key: 'Tab' )"), true );
    toolbar_->setShortcut( seedidx_, "Tab" );

//    trackinvolidx_ = mAddButton( "autotrack", trackFromSeedsAndEdges,
//				tr("Auto-track"), false );

//    trackwithseedonlyidx_ = mAddButton( "trackfromseeds", trackFromSeedsOnly,
//				       tr("Track From Seeds Only"), false );

//    retrackallidx_ = mAddButton( "retrackhorizon", retrackAllCB,
//				tr("Retrack All"), false );
    toolbar_->addSeparator();

//    displayatsectionidx_ = mAddButton( "sectiononly", displayAtSectionCB,
//				      tr("Display at section only"), true );

//    toolbar_->addSeparator();
/*
    polyselectidx_ =  mAddButton( "polygonselect", selectionMode,
				 tr("Polygon Selection mode"), true );
    uiMenu* polymnu = new uiMenu( toolbar_, "PolyMenu" );
    mAddMnuItm( polymnu,uiStrings::sPolygon(), handleToolClick, "polygonselect",
                0 );
    mAddMnuItm( polymnu,uiStrings::sRectangle(),handleToolClick,
                "rectangleselect", 1 );
    toolbar_->setButtonMenu( polyselectidx_, polymnu );

    removeinpolygonidx_ = mAddButton( "trashcan", removeInPolygon,
				  tr("Remove PolySelection"), false );
    toolbar_->addSeparator();
*/

    undoidx_ = mAddButton( "undo", undoPush, tr("Undo (Ctrl+Z)"), false );
    redoidx_ = mAddButton( "redo", redoPush, tr("Redo (Ctrl+Y)"), false );
    toolbar_->setShortcut( undoidx_, "Ctrl+Z" );
    toolbar_->setShortcut( redoidx_, "Ctrl+Y" );

//    toolbar_->addSeparator();
//    saveidx_ = mAddButton( "save", savePush, tr("Save (Ctrl+S"), false );
//    toolbar_->setShortcut( saveidx_, "Ctrl+S" );
}


uiMPEMan::~uiMPEMan()
{
    EM::EMM().undo().undoredochange.remove(
			mCB(this,uiMPEMan,updateButtonSensitivity) );
    deleteVisObjects();
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
    visserv_->selectionmodeChange.remove( mCB(this,uiMPEMan,selectionMode) );
}


void uiMPEMan::deleteVisObjects()
{
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
    if ( engine.trackingInProgress() )
	mSeedClickReturn();

    MPE::EMTracker* tracker = getSelectedTracker();
    if ( !tracker )
	mSeedClickReturn();

    emobj = EM::EMM().getObject( tracker->objectID() );
    if ( !emobj )
	mSeedClickReturn();

    while ( emobj->hasBurstAlert() )
	emobj->setBurstAlert( false );

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

    bool shiftclicked = clickcatcher_->info().isShiftClicked();

    if ( pid.objectID()==-1 && !shiftclicked &&
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
	if ( newvolume.isEmpty() || !newvolume.isDefined() )
	    mSeedClickReturn();

	if ( newvolume != engine.activeVolume() )
	{
	    if ( oldactivevol_.isEmpty() )
	    {
		engine.swapCacheAndItsBackup();
		oldactivevol_ = engine.activeVolume();
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

	    engine.setOneActiveTracker( tracker );
	    engine.activevolumechange.trigger();
	}
    }

    seedpicker->setSowerMode( clickcatcher_->sequentSowing() );
    if ( mIsUdf(cureventnr_) && clickcatcher_->moreToSow() )
	shiftclicked = true;  // 1st seed sown is "tracking buffer" only

    beginSeedClickEvent( emobj );

    if ( pid.objectID()!=-1 || !clickcatcher_->info().getPickedNode().isUdf() )
    {
	const bool ctrlclicked = clickcatcher_->info().isCtrlClicked();
	if ( !clickcatcher_->info().getPickedNode().isUdf() )
	{
	    const EM::PosID nextpid =
		seedpicker->replaceSeed( clickcatcher_->info().getPickedNode(),
					 seedpos );
	    clickcatcher_->info().setPickedNode( nextpid );
	}
	if ( !shiftclicked && !ctrlclicked &&
	     seedpicker->getSeedConnectMode()==EMSeedPicker::DrawBetweenSeeds )
	{
	    if ( clickcatcher_->info().getPickedNode().isUdf() )
		clickcatcher_->info().setPickedNode( pid );
	}
	else if ( shiftclicked && ctrlclicked )
	{
	    if ( seedpicker->removeSeed( pid, true, false ) )
		engine.updateFlatCubesContainer( newvolume, trackerid, false );
	}
	else if ( shiftclicked || ctrlclicked )
	{
	    if ( seedpicker->removeSeed( pid, true, true ) )
		engine.updateFlatCubesContainer( newvolume, trackerid, false );
	}
	else
	{
	    if ( seedpicker->addSeed( seedpos, false ) )
		engine.updateFlatCubesContainer( newvolume, trackerid, true );
	}
    }
    else if ( seedpicker->addSeed(seedpos,shiftclicked) )
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
}


uiToolBar* uiMPEMan::getToolBar() const
{
    return toolbar_;
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
//	toolbar_->turnOn( polyselectidx_, false );
	selectionMode(0);

	visserv_->setViewMode(false);

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
    }

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

    mpeintropending_ = true;
}


void uiMPEMan::finishMPEDispIntro( CallBacker* )
{
    if ( !mpeintropending_ || !oldactivevol_.isEmpty() )
	return;

    mpeintropending_ = false;

    EMTracker* tracker = getSelectedTracker();
    if ( !tracker)
	tracker = engine().getTracker( engine().highestTrackerID() );

    if ( !tracker )
	return;

    TypeSet<Attrib::SelSpec> attribspecs;
    tracker->getNeededAttribs( attribspecs );
    if ( attribspecs.isEmpty() )
	return;
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
//    toolbar_->setSensitive( saveidx_, false );
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


void uiMPEMan::trackerAddedRemovedCB( CallBacker* )
{
    if ( !engine().nrTrackersAlive() )
    {
	seedpickwason_ = false;
	engine().setActiveVolume( TrcKeyZSampling() );
    }
}


void uiMPEMan::visObjectLockedCB( CallBacker* )
{
    updateButtonSensitivity();
}


void uiMPEMan::trackFromSeedsOnly( CallBacker* cb )
{
    trackInVolume( cb );
}


void uiMPEMan::trackFromSeedsAndEdges( CallBacker* cb )
{
    trackInVolume( cb );
}


void uiMPEMan::trackInVolume( CallBacker* )
{
    updateButtonSensitivity();

    MPE::EMTracker* tracker = getSelectedTracker();
    MPE::EMSeedPicker* seedpicker = tracker ? tracker->getSeedPicker(true) : 0;
    const Attrib::SelSpec* as = seedpicker ? seedpicker->getSelSpec() : 0;
    if ( !as ) return;

    if ( !as->isStored() )
    {
	uiMSG().error( "Volume tracking can only be done on stored volumes.");
	return;
    }

    const Attrib::DescSet* ads = Attrib::DSHolder().getDescSet( false, true );
    const MultiID mid = ads ? ads->getStoredKey(as->id()) : MultiID::udf();
    if ( mid.isUdf() )
    {
	uiMSG().error( "Cannot find picked data in database" );
	return;
    }

    mDynamicCastGet(RegularSeisDataPack*,sdp,Seis::PLDM().get(mid));
    if ( !sdp )
    {
	uiMSG().error( "Seismic data is not preloaded yet" );
	return;
    }

    engine().setAttribData( *as, sdp->id() );
    engine().setActiveVolume( sdp->sampling() );

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
    updateButtonSensitivity();
}


static bool sIsPolySelect = true;

void uiMPEMan::selectionMode( CallBacker* cb )
{
    /*
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
    */
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
/*
    const Selector<Coord3>* sel =
	visserv_->getCoordSelector( clickablesceneid_ );
    if ( !sel || !sel->isOK() )
	return;

    const int currentevent = EM::EMM().undo().currentEventID();
    const TypeSet<int>& selectedids = visBase::DM().selMan().selected();
    if ( selectedids.size()!=1 || visserv_->isLocked(selectedids[0]) )
	return;
    mDynamicCastGet(visSurvey::EMObjectDisplay*,
		    emod,visserv_->getObject(selectedids[0]) );

    uiTaskRunner taskrunner( toolbar_ );
    emod->removeSelection( *sel, &taskrunner );
    toolbar_->turnOn( polyselectidx_, false );
    selectionMode( cb );

    setUndoLevel( currentevent );
*/
}


void uiMPEMan::workAreaChgCB( CallBacker* )
{
    if ( !SI().sampling(true).includes( engine().activeVolume() ) )
    {
	engine().setActiveVolume( SI().sampling(true) );
    }
}


void uiMPEMan::showSettingsCB( CallBacker* )
{
    visserv_->sendShowSetupDlgEvent();
}


void uiMPEMan::displayAtSectionCB( CallBacker* )
{
/*
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
	ison ? tr("Display full") : tr("Display at section only") );
*/
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
    toolbar_->setSensitive( undoidx_, EM::EMM().undo().canUnDo() );
    toolbar_->setSensitive( redoidx_, EM::EMM().undo().canReDo() );

    //Seed button
    updateSeedPickState();

    MPE::EMTracker* tracker = getSelectedTracker();
    MPE::EMSeedPicker* seedpicker = tracker ? tracker->getSeedPicker(true) : 0;
//    mDynamicCastGet(MPE::Horizon2DSeedPicker*,sp2d,seedpicker)
//    const bool is2d = sp2d;

//    const bool isinvolumemode = seedpicker && seedpicker->doesModeUseVolume();
//    toolbar_->setSensitive( trackinvolidx_,
//	    !is2d && isinvolumemode && seedpicker );

//    toolbar_->setSensitive( removeinpolygonidx_,
//			    toolbar_->isOn(polyselectidx_) );

    toolbar_->setSensitive( tracker );
    if ( seedpicker &&
	    !(visserv_->isTrackingSetupActive() && (seedpicker->nrSeeds()<1)) )
	toolbar_->setSensitive( true );

    //Save button
//    const EM::EMObject* emobj =
//	tracker ? EM::EMM().getObject( tracker->objectID() ) : 0;
//    toolbar_->setSensitive( saveidx_, emobj && emobj->isChanged() );
}

