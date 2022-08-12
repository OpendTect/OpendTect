/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:    Nanne Hemstra
 Date:	    March 2004
________________________________________________________________________

-*/

#include "uimpeman.h"

#include "attribdescset.h"
#include "attribdescsetsholder.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "emtracker.h"
#include "emundo.h"
#include "executor.h"
#include "horizon2dseedpicker.h"
#include "horizon3dseedpicker.h"
#include "keyboardevent.h"
#include "mpeengine.h"
#include "sectionadjuster.h"
#include "sectiontracker.h"
#include "seisdatapack.h"
#include "seispreload.h"
#include "selector.h"
#include "survinfo.h"
#include "timer.h"

#include "uicombobox.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uistrings.h"
#include "uitaskrunner.h"
#include "uitoolbar.h"
#include "uivispartserv.h"
#include "vishorizon2ddisplay.h"
#include "vishorizondisplay.h"
#include "visrandomtrackdisplay.h"
#include "vismpe.h"
#include "vismpeseedcatcher.h"
#include "visselman.h"
#include "vistransform.h"
#include "vistransmgr.h"
#include "vismpeeditor.h"
#include "visevent.h"

using namespace MPE;


class LockedDisplayTimer : public CallBacker
{
public:
LockedDisplayTimer()
    : hd_(0)
    , timer_(new Timer("Display Locked Nodes"))
{
    timer_->tick.notify( mCB(this,LockedDisplayTimer,hideCB) );
}


~LockedDisplayTimer()
{
    if ( hd_ )
	hd_->unRef();

    delete timer_;
}


#define cLockWaitTime 2000
void start( visSurvey::HorizonDisplay* hd )
{
    if ( hd_ )
	hd_->unRef();

    hd_ = hd;
    if ( hd_ )
	hd_->ref();

    timer_->start( cLockWaitTime, true );
}


protected:
void hideCB( CallBacker* )
{
    if ( !hd_ ) return;

    if ( hd_->lockedShown() )
	hd_->showLocked( false );
    hd_->unRef();
    hd_ = 0;
}

    Timer*			timer_;
    visSurvey::HorizonDisplay*	hd_;

};



uiMPEMan::uiMPEMan( uiParent* p, uiVisPartServer* ps )
    : parent_(p)
    , clickcatcher_(0)
    , visserv_(ps)
    , seedpickwason_(false)
    , oldactivevol_(false)
    , cureventnr_(mUdf(int))
    , lockeddisplaytimer_(new LockedDisplayTimer)
    , sowingmode_(false)
{
    mAttachCB( engine().trackeraddremove, uiMPEMan::trackerAddedRemovedCB );
    mAttachCB( engine().actionCalled, uiMPEMan::mpeActionCalledCB );
    mAttachCB( engine().actionFinished, uiMPEMan::mpeActionFinishedCB );

    mAttachCB( visBase::DM().selMan().selnotifier, uiMPEMan::treeItemSelCB );
    SurveyInfo& si = const_cast<SurveyInfo&>( SI() );
    mAttachCB( si.workRangeChg, uiMPEMan::workAreaChgCB );
    mAttachCB( visserv_->mouseEvent, uiMPEMan::mouseEventCB );
    mAttachCB( visserv_->keyEvent, uiMPEMan::keyEventCB );
    mAttachCB( visSurvey::STM().mouseCursorCall, uiMPEMan::mouseCursorCallCB );
    mAttachCB( visserv_->planeMovedEvent, uiMPEMan::planeChangedCB );
}


uiMPEMan::~uiMPEMan()
{
    detachAllNotifiers();
    deleteVisObjects();
    delete lockeddisplaytimer_;
}


void uiMPEMan::mpeActionCalledCB( CallBacker* )
{
    mEnsureExecutedInMainThread( uiMPEMan::mpeActionCalledCB );
    visSurvey::HorizonDisplay* hd = getSelectedDisplay();
    if ( !hd ) return;

    if ( engine().getState() == Engine::Started )
    {
	if ( hd->displayedOnlyAtSections() )
	    hd->setOnlyAtSectionsDisplay( false );
    }
}


void uiMPEMan::mpeActionFinishedCB( CallBacker* )
{
    mEnsureExecutedInMainThread( uiMPEMan::mpeActionFinishedCB );
    visSurvey::HorizonDisplay* hd = getSelectedDisplay();
    if ( !hd ) return;

    hd->updateAuxData();
}


static const int sStart = 0;
static const int sRetrack = 1;
static const int sStop = 2;
static const int sPoly = 3;
static const int sChild = 4;
static const int sParent = 5;
static const int sParPath = 6;
static const int sClear = 7;
static const int sDelete = 8;
static const int sUndo = 9;
static const int sRedo = 10;
static const int sLock = 11;
static const int sUnlock = 12;
static const int sShowLocked = 13;
static const int sHideLocked = 14;
static const int sSave = 15;
static const int sSaveAs = 16;
static const int sAtSect = 17;
static const int sFull = 18;
static const int sSett = 19;
static const int sAutoMode = 20;
static const int sManMode = 21;
static const int sSnapMode = 22;


void uiMPEMan::keyEventCB( CallBacker* )
{
    int action = -1;
    const KeyboardEvent& kev = visserv_->getKeyboardEvent();

    const bool undoonloadedhor =
	KeyboardEvent::isUnDo(kev) || KeyboardEvent::isReDo(kev);

    if ( KeyboardEvent::isUnDo(kev) )
	undo();
    else if ( KeyboardEvent::isReDo(kev) )
	redo();

    if ( undoonloadedhor || MPE::engine().nrTrackersAlive() == 0 ) return;

    if ( kev.key_ == OD::KB_K )
    {
	if ( MPE::engine().trackingInProgress() )
	    action = sStop;
	else
	{
	    if ( OD::ctrlKeyboardButton(kev.modifier_) )
		action = sRetrack;
	    else if ( kev.modifier_==OD::NoButton )
		action = sStart;
	}
    }
    else if ( kev.key_==OD::KB_R && kev.modifier_==OD::NoButton )
	restrictCurrentHorizon();
    else if ( kev.key_ == OD::KB_Y )
	action = sPoly;
    else if ( kev.key_ == OD::KB_A )
	action = sClear;
    else if ( kev.key_==OD::KB_D || kev.key_==OD::KB_Delete )
	action = sDelete;
    else if ( kev.key_ == OD::KB_L )
	action = sLock;
    else if ( kev.key_ == OD::KB_U )
	action = sUnlock;
    else if ( kev.key_ == OD::KB_S && OD::ctrlKeyboardButton(kev.modifier_)
				&& OD::shiftKeyboardButton(kev.modifier_) )
	action = sSaveAs;
    else if ( kev.key_ == OD::KB_S && OD::ctrlKeyboardButton(kev.modifier_) )
	action = sSave;
    else if ( kev.key_ == OD::KB_T )
	action = sAutoMode;
    else if ( kev.key_ == OD::KB_M )
	action = sManMode;
    else if ( kev.key_ == OD::KB_S )
	action = sSnapMode;
    else if ( kev.key_ == OD::KB_H )
	action = sSett;

    if ( action != -1 )
	handleAction( action );
}


void uiMPEMan::mouseEventCB( CallBacker* )
{
    if ( MPE::engine().nrTrackersAlive() == 0 ) return;

    const MouseEvent& mev = visserv_->getMouseEvent();
    if ( mev.ctrlStatus() && mev.rightButton() && !mev.isPressed() )
    {
	const int res = popupMenu();
	handleAction( res );
	visserv_->setSceneEventHandled();
    }
}


static void addAction( uiMenu& mnu, uiString txt, const char* sc, int id,
			const char* icon, bool enab, bool doadd )
{
    if ( !doadd ) return;

    uiAction* action = new uiAction( txt );
    mnu.insertAction( action, id );
    action->setEnabled( enab );
    action->setIcon( icon );
}


int uiMPEMan::popupMenu()
{
    EM::Horizon* hor = getSelectedHorizon();
    EM::Horizon2D* hor2d = getSelectedHorizon2D();
    EM::Horizon3D* hor3d = getSelectedHorizon3D();
    if ( !hor2d && !hor3d ) return -1;

    visSurvey::EMObjectDisplay* emod = getSelectedEMDisplay();
    visSurvey::Scene* scene = emod ? emod->getScene() : 0;
    if ( !scene ) return -1;

    uiMenu mnu( tr("Tracking Menu") );
    const bool istracking = MPE::engine().trackingInProgress();
    if ( istracking )
    {
	addAction( mnu, tr("Stop Auto Tracking"), "k", sStop, "stop",
		   true, hor3d );
	return mnu.exec();
    }

    const Coord3& clickedpos = scene->getMousePos( true );
    const bool haspos = !clickedpos.isUdf();
    addAction( mnu, tr("Start Auto Tracking"), "k", sStart, "autotrack",
	       true, hor3d );
    addAction( mnu, tr("Retrack From Seeds"), "ctrl+k", sRetrack,
		"retrackhorizon", true, hor3d );
    addAction( mnu, tr("Select With Polygon"), "y", sPoly,
		"polygonselect", true, true );
    if ( haspos )
    {
	addAction( mnu, tr("Select Parents"), "", sParent, 0, true, hor3d );
	addAction( mnu, tr("Show Parents Path"), "", sParPath, 0, true, hor3d );
	addAction( mnu, tr("Select Children"), "", sChild, 0, true, hor3d );
    }

    addAction( mnu, tr("Clear Selection"), "a", sClear, "clear", true, hor3d );
    addAction( mnu, tr("Delete Selected"), "d", sDelete,
	       "clearselection", true, hor3d );
    addAction( mnu, tr("Undo"), "ctrl+z", sUndo, "undo",
		EM::EMM().undo(hor->id()).canUnDo(), true );
    addAction( mnu, tr("Redo"), "ctrl+y", sRedo, "redo",
		EM::EMM().undo(hor->id()).canReDo(), true );
    addAction( mnu, tr("Lock"), "l", sLock, "lock", true, hor3d );
    addAction( mnu, tr("Unlock"), "u", sUnlock, "unlock", true, hor3d );

    visSurvey::HorizonDisplay* hd3d = getSelectedDisplay();
    addAction( mnu, tr("Hide Locked"), "", sHideLocked, 0, true,
	       hd3d && hd3d->lockedShown() );
    addAction( mnu, tr("Show Locked"), "", sShowLocked, 0, true,
	       hd3d && !hd3d->lockedShown() );

    addAction( mnu, tr("Save"), "ctrl+s", sSave,
	       "save", hor->isChanged(), true );
    addAction( mnu, tr("Save As ..."), "ctrl+shift+s", sSaveAs,
	       "saveas", true, true );
    if ( !emod->displayedOnlyAtSections() )
	addAction( mnu, tr("Display Only at Sections"), "v", sAtSect,
		    "sectiononly", true, true );
    else
	addAction( mnu, tr("Display in Full"), "v", sFull,
		   "sectionoff", true, true );
    addAction( mnu, tr("Change Settings ..."), "", sSett,
	       "seedpicksettings", true, true );

    return mnu.exec();
}


void uiMPEMan::handleAction( int res )
{
    MPE::EMTracker* tracker = getSelectedTracker();
    EM::EMObject* emobj =
		tracker ? EM::EMM().getObject(tracker->objectID()) : 0;
    mDynamicCastGet(EM::Horizon3D*,hor3d,emobj)
    mDynamicCastGet(EM::Horizon2D*,hor2d,emobj)
    if ( !hor2d && !hor3d ) return;

    visSurvey::EMObjectDisplay* emod = getSelectedEMDisplay();
    visSurvey::Scene* scene = emod ? emod->getScene() : 0;
    if ( !scene ) return;

    const Coord3& clickedpos = scene->getMousePos( true );
    const TrcKey tk( SI().transform( clickedpos.coord() ) );

    visSurvey::HorizonDisplay* hd3d = getSelectedDisplay();
    switch ( res )
    {
    case sStart: startTracking(); break;
    case sRetrack: startRetrack(); break;
    case sStop: stopTracking(); break;
    case sPoly: changePolySelectionMode(); break;
    case sChild: if ( hor3d ) hor3d->selectChildren(tk); break;
    case sParent: if ( hd3d ) hd3d->selectParent(tk); break;
    case sParPath: showParentsPath(); break;
    case sClear: clearSelection(); break;
    case sDelete: deleteSelection(); break;
    case sUndo: undo(); break;
    case sRedo: redo(); break;
    case sLock: lockAll(); break;
    case sUnlock: if ( hor3d ) hor3d->unlockAll(); break;
    case sShowLocked: if ( hd3d ) hd3d->showLocked( true ); break;
    case sHideLocked: if ( hd3d ) hd3d->showLocked( false ); break;
    case sSave: visserv_->storeEMObject( false ); break;
    case sSaveAs: visserv_->storeEMObject( true ); break;
    case sAtSect: emod->setOnlyAtSectionsDisplay( true ); break;
    case sFull: emod->setOnlyAtSectionsDisplay( false ); break;
    case sSett: showSetupDlg(); break;
    case sAutoMode:
    case sManMode:
    case sSnapMode: changeMode(res);
    default:
	break;
    }
}


void uiMPEMan::startTracking()
{
    uiString errmsg;
    if ( !MPE::engine().startTracking(errmsg) && !errmsg.isEmpty() )
	uiMSG().error( errmsg );
}


void uiMPEMan::startRetrack()
{
    uiString errmsg;
    if ( !MPE::engine().startRetrack(errmsg) && !errmsg.isEmpty() )
	uiMSG().error( errmsg );
}


void uiMPEMan::stopTracking()
{
    MPE::engine().stopTracking();
}


void uiMPEMan::restrictCurrentHorizon()
{
    MPE::EMTracker* tracker = getSelectedTracker();
    if ( !tracker )
    {
	const int nrtrackers = MPE::engine().nrTrackersAlive();
	if ( nrtrackers > 0 )
	    tracker = MPE::engine().getTracker( 0 );
    }

    EM::EMObject* emobj =
		tracker ? EM::EMM().getObject(tracker->objectID()) : 0;
    mDynamicCastGet(EM::Horizon3D*,hor3d,emobj)
    if ( !hor3d ) return;

    TypeSet<VisID> visids;
    visserv_->findObject( typeid(visSurvey::HorizonDisplay), visids );
    for ( int idx=0; idx<visids.size(); idx++ )
    {
	mDynamicCastGet(visSurvey::HorizonDisplay*,hd,
			visserv_->getObject(visids[idx]))
	if ( hd && hd->getObjectID() == hor3d->id() )
	    hd->setOnlyAtSectionsDisplay( !hd->displayedOnlyAtSections() );
    }
}


void uiMPEMan::deleteVisObjects()
{
    if ( clickcatcher_ )
    {
	if ( clickablesceneid_.isValid() )
	    visserv_->removeObject( clickcatcher_->id(), clickablesceneid_ );

	clickcatcher_->click.remove( mCB(this,uiMPEMan,seedClick) );
	clickcatcher_->setEditor( 0 );
	clickcatcher_->unRef();
	clickcatcher_ = 0;
	clickablesceneid_.setUdf();
    }
}


void uiMPEMan::mouseCursorCallCB( CallBacker* )
{
    visSurvey::Scene* scene = visSurvey::STM().currentScene();

    if ( !scene || scene->id()!=clickablesceneid_ ||
	 !isSeedPickingOn() || !clickcatcher_ || !clickcatcher_->getEditor() ||
	 MPE::engine().trackingInProgress() )
	return;

    MPE::EMTracker* tracker = getSelectedTracker();
    MPE::EMSeedPicker* seedpicker = tracker ? tracker->getSeedPicker(true) : 0;
    if ( !seedpicker )
	return;

    mDefineStaticLocalObject( MouseCursor, pickcursor, = MouseCursor::Cross );
    scene->passMouseCursor( pickcursor );
}


#define mSeedClickReturn() \
{\
if ( seedpicker && clickcatcher_ && clickcatcher_->moreToSow() )\
{\
    if (seedpicker->getTrackMode()==seedpicker->DrawBetweenSeeds ||\
	seedpicker->getTrackMode()==seedpicker->DrawAndSnap )\
	return;\
}\
endSeedClickEvent(emobj);\
return; \
}\

void uiMPEMan::seedClick( CallBacker* )
{
    MPE::EMSeedPicker* seedpicker = 0;
    EM::EMObject* emobj = 0;
    MPE::Engine& engine = MPE::engine();
    if ( engine.trackingInProgress() )
	mSeedClickReturn();

    MPE::EMTracker* tracker = getSelectedTracker();
    if ( !tracker )
	mSeedClickReturn();

    emobj = EM::EMM().getObject( tracker->objectID() );
    mDynamicCastGet(EM::Horizon*,hor,emobj)
    if ( !hor )
	mSeedClickReturn();

    while ( emobj->hasBurstAlert() )
	emobj->setBurstAlert( false );

    if ( !clickcatcher_ )
	mSeedClickReturn();

    const int trackerid =
		MPE::engine().getTrackerByObject( tracker->objectID() );

    const VisID clickedobject = clickcatcher_->info().getObjID();
    if ( !clickedobject.isValid() )
	mSeedClickReturn();

    const EM::ObjectID emobjid = clickcatcher_->info().getEMObjID();
    mDynamicCastGet(EM::Horizon*,clickedhor,EM::EMM().getObject(emobjid))
    const bool clickedonhorizon = clickedhor;
    if ( clickedhor && clickedhor!=hor )
    {
	const VisID emvisid = clickcatcher_->info().getEMVisID();
	visBase::DM().selMan().select(
				emvisid.isValid() ? emvisid : clickedobject );
	mSeedClickReturn();
    }

    seedpicker = tracker->getSeedPicker(true);
    if ( !seedpicker )
	mSeedClickReturn();

    const bool shiftclicked = clickcatcher_->info().isShiftClicked();
    const bool ctrlclicked = clickcatcher_->info().isCtrlClicked();
    if ( clickedhor && clickedhor==hor &&
	!clickcatcher_->info().isDoubleClicked() && !ctrlclicked )
    {
	if ( seedpicker->getTrackMode()==seedpicker->DrawBetweenSeeds ||
	    seedpicker->getTrackMode()==seedpicker->DrawAndSnap )
	    mSeedClickReturn();
    }

    const bool dbclick = clickcatcher_->info().isDoubleClicked() &&
	(seedpicker->getTrackMode()==seedpicker->DrawBetweenSeeds ||
	 seedpicker->getTrackMode()==seedpicker->DrawAndSnap);

    if ( dbclick )
    {
	seedpicker->endPatch( false );
	cleanPatchDisplay();
	mSeedClickReturn();
    }

    if ( !clickcatcher_->info().isLegalClick() )
    {
	if ( tracker->is2D() && !clickcatcher_->info().getObjLineName() )
	    uiMSG().error( tr("2D tracking cannot handle picks on 3D lines.") );
	else if ( !tracker->is2D() && clickcatcher_->info().getObjLineName() )
	    uiMSG().error( tr("3D tracking cannot handle picks on 2D lines.") );
	else if ( clickcatcher_->info().getObjCS().nrZ()==1 &&
		  !clickcatcher_->info().getObjCS().isEmpty() )
	    uiMSG().error( emobj->getUserTypeStr(),
			   tr("Tracking cannot handle picks on time slices.") );
	mSeedClickReturn();
    }

    const Attrib::SelSpec* clickedas =
	clickcatcher_->info().getObjDataSelSpec();
    if ( !clickedas )
	mSeedClickReturn();

    const MPE::SectionTracker* sectiontracker =
			tracker->getSectionTracker( true );
    const Attrib::SelSpec* trackedatsel = sectiontracker
	? sectiontracker->adjuster()->getAttributeSel(0)
	: 0;

    if ( seedpicker->nrSeeds() > 0 )
    {
	if ( trackedatsel &&
	     (seedpicker->getTrackMode()!=seedpicker->DrawBetweenSeeds) &&
	      seedpicker->getTrackMode()!=seedpicker->DrawAndSnap )
	{
	    uiString msg;
	    const bool isdatasame = MPE::engine().pickingOnSameData(
			*trackedatsel, *clickedas, msg );
	    if ( !isdatasame )
	    {
		const bool res = uiMSG().askContinue( msg );
		if ( res )
		{
		    DataPackID datapackid =
				clickcatcher_->info().getObjDataPackID();
		    if ( datapackid.isValid() && datapackid!=DataPack::cNoID() )
			engine.setAttribData( *clickedas, datapackid );
		    seedpicker->setSelSpec( clickedas );
		}
		mSeedClickReturn();
	    }
	}
    }

    const TrcKey node = clickcatcher_->info().getNode();
    Coord3 seedcrd;
    if ( !clickedonhorizon )
    {
	visSurvey::Scene* scene = visSurvey::STM().currentScene();
	seedcrd = clickcatcher_->info().getPos();
	scene->getTempZStretchTransform()->transformBack( seedcrd );
	scene->getUTM2DisplayTransform()->transformBack( seedcrd );
    }
    else
    {
	seedcrd = hor->getCoord( node );
    }
    if ( seedcrd.isUdf() )
	mSeedClickReturn();

    const Pos::GeomID geomid = clickcatcher_->info().getGeomID();
    const bool undefgeomid = geomid == Survey::GM().cUndefGeomID();
    TrcKeyValue seedpos( undefgeomid ? TrcKey(SI().transform(seedcrd)) : node,
			 (float)seedcrd.z );

    OD::Color clr = OD::Color::Green();
    if ( Math::Abs(emobj->preferredColor().g()-OD::Color::Green().g())<30 )
	    clr = OD::Color::Red();

    const OD::Color sowclr=
	seedpicker->getTrackMode()==seedpicker->DrawBetweenSeeds ||
	seedpicker->getTrackMode()==seedpicker->DrawAndSnap ?
	clr : emobj->preferredColor();
    if ( !clickedonhorizon && !shiftclicked &&
	 clickcatcher_->activateSower( sowclr,
	 tracker->is2D() ? 0 : &seedpicker->getSeedPickArea()) )
    {
	 mSeedClickReturn();
    }

    TrcKeyZSampling newvolume;
    if ( tracker->is2D() )
    {
	engine.setActive2DLine( geomid );

	mDynamicCastGet( MPE::Horizon2DSeedPicker*, h2dsp, seedpicker );
	DataPackID datapackid = clickcatcher_->info().getObjDataPackID();

	if ( h2dsp )
	    h2dsp->setSelSpec( clickedas );

	if ( !h2dsp || !h2dsp->canAddSeed(*clickedas) )
	{
	    uiMSG().error( tr("Cannot add seeds") );
	    mSeedClickReturn();
	}

	if ( datapackid.isValid() && datapackid!=DataPack::cNoID() )
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
	    engine.setActivePath( clickcatcher_->info().getObjTKPath() );
	    engine.setActiveRandomLineID(
		    clickcatcher_->info().getObjRandomLineID() );
	    notifystopper.enableNotification();

	    if ( clickedas )
	    {
		DataPackID datapackid =
				clickcatcher_->info().getObjDataPackID();
		if ( datapackid.isValid() && datapackid!=DataPack::cNoID() )
		    engine.setAttribData( *clickedas, datapackid );
	    }

	    engine.setOneActiveTracker( tracker );
	    engine.activevolumechange.trigger();
	}
    }

    seedpicker->setSelSpec( clickedas );
    seedpicker->setSowerMode( clickcatcher_->sequentSowing() );
    beginSeedClickEvent( emobj );

    const visBase::EventInfo* eventinfo = clickcatcher_->visInfo();
    const bool ctrlbut = OD::ctrlKeyboardButton( eventinfo->buttonstate_ );
    const bool blockcallback =
	emobj->geometryElement()->blocksCallBacks();

    if ( clickedonhorizon || !clickcatcher_->info().getPickedNode().isUdf() )
    {
	if ( !clickcatcher_->info().getPickedNode().isUdf() )
	{
	    const TrcKey nexttk =
		seedpicker->replaceSeed( clickcatcher_->info().getPickedNode(),
					 seedpos );
	    clickcatcher_->info().setPickedNode( nexttk );
	}
	if ( !shiftclicked && !ctrlclicked &&
	     seedpicker->getTrackMode()==EMSeedPicker::DrawBetweenSeeds )
	{
	    if ( clickcatcher_->info().getPickedNode().isUdf() )
		clickcatcher_->info().setPickedNode( seedpos.tk_ );
	}
	else if ( shiftclicked && ctrlclicked )
	{
	    if ( seedpicker->removeSeed( node, true, false ) )
		engine.updateFlatCubesContainer( newvolume, trackerid, false );
	}
	else if ( shiftclicked || ctrlclicked )
	{
	    if ( seedpicker->removeSeed( node, true, true ) )
		engine.updateFlatCubesContainer( newvolume, trackerid, false );
	}
	else
	{
	    if ( !sowingmode_ && seedpicker->addSeed(seedpos,false) )
	    {
		engine.updateFlatCubesContainer( newvolume, trackerid, true );
		if ( blockcallback )
		    emobj->geometryElement()->blockCallBacks( true, true );
	    }
	    else if ( sowingmode_ && !ctrlbut )
	    {
		seedpicker->addSeedToPatch( seedpos, false );
		engine.updateFlatCubesContainer( newvolume, trackerid, true );
	    }
	}
    }
    else
    {
	const bool doerase = ctrlbut && sowingmode_;
	const bool manualmodeclick = !ctrlbut &&
	    (seedpicker->getTrackMode()==seedpicker->DrawBetweenSeeds ||
	     seedpicker->getTrackMode()==seedpicker->DrawAndSnap);

	if ( doerase || manualmodeclick )
	{
	    seedpicker->addSeedToPatch( seedpos, true );
	    updatePatchDisplay();
	}
	else if ( !ctrlbut && !sowingmode_ )
	{
	    if ( seedpicker->addSeed(seedpos, shiftclicked) )
	    {
		engine.updateFlatCubesContainer( newvolume, trackerid, true );
		if ( blockcallback )
		    emobj->geometryElement()->blockCallBacks( true, true );
	    }
	}
	else if ( sowingmode_ )
	{
	    seedpicker->addSeedToPatch( seedpos, false );
	    engine.updateFlatCubesContainer( newvolume, trackerid, true );
	}
    }
    if ( !clickcatcher_->moreToSow() )
	endSeedClickEvent( emobj );
}


void uiMPEMan::planeChangedCB( CallBacker* )
{
    MPE::EMTracker* tracker = getSelectedTracker();
    if( !tracker ) return;
    MPE::EMSeedPicker* seedpicker = tracker ? tracker->getSeedPicker(true) : 0;
    if ( !seedpicker || !seedpicker->getPatch() ||
	seedpicker->getPatch()->getPath().size()<=0 )
	return;

    seedpicker->endPatch( false );
    cleanPatchDisplay();
}


void uiMPEMan::beginSeedClickEvent( EM::EMObject* emobj )
{
    if ( mIsUdf(cureventnr_) )
    {
	cureventnr_ = EM::EMM().undo(emobj->id()).currentEventID();
	MouseCursorManager::setOverride( MouseCursor::Wait );
	if ( emobj )
	    emobj->setBurstAlert( true );
    }
}


void uiMPEMan::endSeedClickEvent( EM::EMObject* emobj )
{
    if ( clickcatcher_ )
	clickcatcher_->stopSowing();

    if ( !mIsUdf(cureventnr_) )
    {
	if ( emobj )
	    emobj->setBurstAlert( false );

	MouseCursorManager::restoreOverride();
	setUndoLevel( emobj->id(), cureventnr_ );
	cureventnr_ = mUdf(int);
    }
}


void uiMPEMan::changePolySelectionMode()
{
    const bool topolymode = !visserv_->isSelectionModeOn();
    if ( topolymode )
    {
	visserv_->setViewMode( false );
	visserv_->setSelectionMode( uiVisPartServer::Polygon );
    }

    visserv_->turnSelectionModeOn( topolymode );
    turnSeedPickingOn( !topolymode );
}


void uiMPEMan::clearSelection()
{
    visSurvey::HorizonDisplay* hd = getSelectedDisplay();
    if ( visserv_->isSelectionModeOn() )
    {
	visserv_->turnSelectionModeOn( false );
	turnSeedPickingOn( true );
	if ( hd ) hd->clearSelections();
    }
    else
    {
	EM::Horizon3D* hor3d = getSelectedHorizon3D();
	if ( hor3d ) hor3d->resetChildren();

	if ( hd )
	{
	    hd->showSelections( false );
	    hd->showParentLine( false );
	}
    }
}


void uiMPEMan::deleteSelection()
{
    visSurvey::HorizonDisplay* hd = getSelectedDisplay();
    if ( visserv_->isSelectionModeOn() )
    {
	removeInPolygon();
	turnSeedPickingOn( true );
    }
    else
    {
	EM::Horizon3D* hor3d = getSelectedHorizon3D();
	if ( hor3d ) hor3d->deleteChildren();
	if ( hd ) hd->showSelections( false );
    }
}


void uiMPEMan::showParentsPath()
{ visserv_->sendVisEvent( uiVisPartServer::evShowMPEParentPath() ); }

void uiMPEMan::showSetupDlg()
{ visserv_->sendVisEvent( uiVisPartServer::evShowMPESetupDlg() ); }


void uiMPEMan::changeMode( int mode )
{
    MPE::EMTracker* tracker = getSelectedTracker();
    MPE::EMSeedPicker* seedpicker = tracker ? tracker->getSeedPicker(true) : 0;
    if ( !seedpicker )
	return;

    if ( mode==sAutoMode )
	seedpicker->setTrackMode( MPE::EMSeedPicker::TrackFromSeeds );
    else if ( mode==sManMode )
	seedpicker->setTrackMode( MPE::EMSeedPicker::DrawBetweenSeeds );
    else if ( mode==sSnapMode )
	seedpicker->setTrackMode( MPE::EMSeedPicker::DrawAndSnap );

    MPE::engine().settingsChanged.trigger();
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
    if ( !yn && clickcatcher_ )
	clickcatcher_->setEditor( 0 );

    MPE::EMTracker* tracker = getSelectedTracker();

    if ( yn )
    {
	visserv_->setViewMode( false );
	if ( getSelectedDisplay() )
	    visserv_->setCurInterObjID( getSelectedDisplay()->id() );

	updateClickCatcher();
	if ( clickcatcher_ )
	    clickcatcher_->turnOn( true );

	const EM::EMObject* emobj =
			tracker ? EM::EMM().getObject(tracker->objectID()) : 0;
	if ( clickcatcher_ && emobj )
	    clickcatcher_->setTrackerType( emobj->getTypeStr() );
    }
    else
    {
	MPE::EMSeedPicker* seedpicker =
		tracker ? tracker->getSeedPicker( false ) : 0;
	if ( seedpicker )
	    seedpicker->stopSeedPick();

	if ( clickcatcher_ )
	    clickcatcher_->turnOn( false );
    }

    visserv_->sendVisEvent( uiVisPartServer::evPickingStatusChange() );
}


void uiMPEMan::updateClickCatcher( bool create )
{
    if ( !clickcatcher_ && !create )
	return;

    if ( !clickcatcher_ && create )
    {
	TypeSet<VisID> catcherids;
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
	mAttachCB( clickcatcher_->endSowing, uiMPEMan::sowingFinishedCB );
	mAttachCB( clickcatcher_->sowing, uiMPEMan::sowingModeCB );
    }

    const TypeSet<VisID>& selectedids = visBase::DM().selMan().selected();
    if ( selectedids.size() != 1 )
	return;

    mDynamicCastGet(visSurvey::EMObjectDisplay*,
		    surface,visserv_->getObject(selectedids[0]));
    clickcatcher_->setEditor( surface ? surface->getEditor() : 0 );

    const SceneID newsceneid = visserv_->getSceneID( selectedids[0] );
    if ( !newsceneid.isValid() || newsceneid == clickablesceneid_ )
	return;

    visserv_->removeObject( clickcatcher_->id(), clickablesceneid_ );
    visserv_->addObject( clickcatcher_, newsceneid, false );
    clickablesceneid_ = newsceneid;
}


void uiMPEMan::sowingFinishedCB( CallBacker* )
{
    sowingmode_ = false;
    MPE::EMTracker* tracker = getSelectedTracker();
    MPE::EMSeedPicker* seedpicker = tracker ? tracker->getSeedPicker(true) : 0;
    if ( !seedpicker ) return;

    const visBase::EventInfo* eventinfo = clickcatcher_->visInfo();
    const bool doerase = OD::ctrlKeyboardButton( eventinfo->buttonstate_ );
    seedpicker->endPatch( doerase );
    cleanPatchDisplay();
}


void uiMPEMan::sowingModeCB( CallBacker* )
{
    sowingmode_ = true;
}


void uiMPEMan::treeItemSelCB( CallBacker* )
{
    if ( !getSelectedDisplay() && !getSelected2DDisplay() )
    {
	turnSeedPickingOn( false );
	updateClickCatcher( false );
	return;
    }

    validateSeedConMode();
    updateClickCatcher();
    turnSeedPickingOn( true );
}


void uiMPEMan::validateSeedConMode()
{
    MPE::EMTracker* tracker = getSelectedTracker();
    MPE::EMSeedPicker* seedpicker = tracker ? tracker->getSeedPicker(true) : 0;
    if ( !seedpicker ) return;

    const EM::EMObject* emobj = EM::EMM().getObject( tracker->objectID() );
    if ( !emobj )
	return;

    const SectionTracker* sectiontracker =
			tracker->getSectionTracker( true );
    const bool setupavailable = sectiontracker &&
				sectiontracker->hasInitializedSetup();
    if ( setupavailable )
	return;

    seedpicker->setTrackMode( MPE::EMSeedPicker::TrackFromSeeds );
}


void uiMPEMan::cleanPatchDisplay()
{
    visSurvey::EMObjectDisplay* emod = getSelectedEMDisplay();
    visSurvey::MPEEditor* editor = emod ? emod->getEditor() : 0;
    if ( editor )
	editor->cleanPatch();
}


void uiMPEMan::undo()
{
    visSurvey::EMObjectDisplay* emod = getSelectedEMDisplay();
    if ( !emod || !emod->isSelected() )
	return;

    MouseCursorChanger mcc( MouseCursor::Wait );
    uiString errmsg;
    MPE::EMTracker* tracker = getSelectedTracker();
    MPE::EMSeedPicker* seedpicker = tracker ? tracker->getSeedPicker(true) : 0;

    bool update = false;
    if ( seedpicker && seedpicker->canUndo() )
    {
	 seedpicker->horPatchUndo().unDo();
	 updatePatchDisplay();
    }
    else
    {
	mDynamicCastGet(
	    EM::EMUndo*,emundo,&EM::EMM().undo(emod->getObjectID()) );
	if ( !emundo ) return;
	EM::EMM().burstAlertToAll( true );
	update = emundo->unDo( 1, true );
	EM::EMM().burstAlertToAll( false );
    }

    if ( update )
    {
	emod->updateAuxData();
	emod->requestSingleRedraw();
    }
}


void uiMPEMan::redo()
{
    visSurvey::EMObjectDisplay* emod = getSelectedEMDisplay();
    if ( !emod || !emod->isSelected() )
	return;

    MouseCursorChanger mcc( MouseCursor::Wait );

    MPE::EMTracker* tracker = getSelectedTracker();
    MPE::EMSeedPicker* seedpicker = tracker ? tracker->getSeedPicker(true) : 0;

    bool update = false;
    if ( seedpicker && seedpicker->canReDo() )
    {
	 seedpicker->horPatchUndo().reDo();
	 updatePatchDisplay();
    }
    else
    {
	mDynamicCastGet(
	    EM::EMUndo*,emundo,&EM::EMM().undo(emod->getObjectID()) );
	if ( !emundo ) return;
	EM::EMM().burstAlertToAll( true );
	update = emundo->reDo( 1, true );
	EM::EMM().burstAlertToAll( false );
    }

    if ( update )
    {
	emod->updateAuxData();
	emod->requestSingleRedraw();
    }
}


void uiMPEMan::lockAll()
{
    EM::Horizon3D* hor3d = getSelectedHorizon3D();
    visSurvey::HorizonDisplay* hd = getSelectedDisplay();
    const bool preshowlocked = hd->lockedShown();

    if ( hor3d && hd )
    {
	hor3d->lockAll();
	hd->showLocked( true );
	if ( !preshowlocked )
	    lockeddisplaytimer_->start( hd );
    }
}


void uiMPEMan::updatePatchDisplay()
{
    MPE::EMTracker* tracker = getSelectedTracker();
    MPE::EMSeedPicker* seedpicker = tracker ? tracker->getSeedPicker(true) : 0;
    if ( !tracker || !seedpicker )
	return;

    visSurvey::EMObjectDisplay* emod = getSelectedEMDisplay();
    visSurvey::MPEEditor* editor = emod ? emod->getEditor() : 0;
    if ( editor )
	editor->displayPatch( seedpicker->getPatch() );
}


MPE::EMTracker* uiMPEMan::getSelectedTracker()
{
    const TypeSet<VisID>& selectedids = visBase::DM().selMan().selected();
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


visSurvey::EMObjectDisplay* uiMPEMan::getSelectedEMDisplay()
{
    const TypeSet<VisID>& selectedids = visBase::DM().selMan().selected();
    if ( selectedids.size() != 1 )
	return 0;

    mDynamicCastGet(visSurvey::EMObjectDisplay*,emod,
		    visserv_->getObject(selectedids[0]))
    return emod;
}


visSurvey::HorizonDisplay* uiMPEMan::getSelectedDisplay()
{
    mDynamicCastGet(visSurvey::HorizonDisplay*,hd,getSelectedEMDisplay())
    return hd;
}


visSurvey::Horizon2DDisplay* uiMPEMan::getSelected2DDisplay()
{
    mDynamicCastGet(visSurvey::Horizon2DDisplay*,hd,getSelectedEMDisplay())
    return hd;
}


EM::Horizon* uiMPEMan::getSelectedHorizon()
{
    MPE::EMTracker* tracker = getSelectedTracker();
    EM::EMObject* emobj =
		tracker ? EM::EMM().getObject(tracker->objectID()) : 0;
    mDynamicCastGet(EM::Horizon*,hor,emobj)
    return hor;
}


EM::Horizon3D* uiMPEMan::getSelectedHorizon3D()
{
    mDynamicCastGet(EM::Horizon3D*,hor3d,getSelectedHorizon())
    return hor3d;
}


EM::Horizon2D* uiMPEMan::getSelectedHorizon2D()
{
    mDynamicCastGet(EM::Horizon2D*,hor2d,getSelectedHorizon())
    return hor2d;
}


void uiMPEMan::updateSeedPickState()
{
    MPE::EMTracker* tracker = getSelectedTracker();
    MPE::EMSeedPicker* seedpicker = tracker ? tracker->getSeedPicker(true) : 0;

    if ( !seedpicker )
    {
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
}


void uiMPEMan::trackerAddedRemovedCB( CallBacker* )
{
    MPE::EMTracker* tracker = getSelectedTracker();
    if ( !tracker )
    {
	turnSeedPickingOn( false );
	updateClickCatcher( false );
    }
}


void uiMPEMan::visObjectLockedCB( CallBacker* )
{}

void uiMPEMan::trackFromSeedsOnly()
{}

void uiMPEMan::trackFromSeedsAndEdges()
{}

void uiMPEMan::removeInPolygon()
{ visserv_->removeSelection(); }


void uiMPEMan::workAreaChgCB( CallBacker* )
{
    if ( !SI().sampling(true).includes( engine().activeVolume() ) )
    {
	engine().setActiveVolume( SI().sampling(true) );
    }
}


void uiMPEMan::initFromDisplay()
{
    // compatibility for session files where box outside workarea
    workAreaChgCB(0);
}


void uiMPEMan::setUndoLevel( int )
{
}


void uiMPEMan::setUndoLevel( const EM::ObjectID& id, int preveventnr )
{
    Undo& emundo = EM::EMM().undo( id );
    const int currentevent = emundo.currentEventID();
    if ( currentevent != preveventnr )
	    emundo.setUserInteractionEnd(currentevent);
}
