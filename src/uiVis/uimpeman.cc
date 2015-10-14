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

uiMPEMan::uiMPEMan( uiParent* p, uiVisPartServer* ps )
    : parent_(p)
    , clickcatcher_(0)
    , clickablesceneid_(-1)
    , visserv_(ps)
    , seedpickwason_(false)
    , oldactivevol_(false)
    , cureventnr_(mUdf(int))
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
}


uiMPEMan::~uiMPEMan()
{
    detachAllNotifiers();
    deleteVisObjects();
}


void uiMPEMan::mpeActionCalledCB( CallBacker* )
{
    visSurvey::HorizonDisplay* hd = getSelectedDisplay();
    if ( !hd ) return;

    if ( engine().getState() == Engine::Started )
    {
	hd->setOnlyAtSectionsDisplay( false );
    }
}


void uiMPEMan::mpeActionFinishedCB( CallBacker* )
{
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
static const int sRest = 17;
static const int sFull = 18;
static const int sSett = 19;


void uiMPEMan::keyEventCB( CallBacker* )
{
    if ( MPE::engine().nrTrackersAlive() == 0 ) return;

    int action = -1;
    const KeyboardEvent& kev = visserv_->getKeyboardEvent();

    if ( KeyboardEvent::isUnDo(kev) )
	undo();
    else if ( KeyboardEvent::isReDo(kev) )
	redo();
    else if ( kev.key_ == OD::K )
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
    else if ( kev.key_ == OD::Y )
    {
	action = sPoly;
    }
    else if ( kev.key_ == OD::A )
	action = sClear;
    else if ( kev.key_==OD::D || kev.key_==OD::Delete )
	action = sDelete;
    else if ( kev.key_ == OD::L )
	action = sLock;
    else if ( kev.key_ == OD::U )
	action = sUnlock;
    else if ( kev.key_ == OD::S && OD::ctrlKeyboardButton(kev.modifier_) )
	action = sSave;
    else if ( kev.key_ == OD::S && OD::ctrlKeyboardButton(kev.modifier_)
				&& OD::shiftKeyboardButton(kev.modifier_) )
	action = sSaveAs;

    if ( action != -1 )
	handleAction( action );
}


#define mAddAction(txt,sc,id,icon,enab) \
{ \
    uiAction* action = new uiAction( txt ); \
    mnu.insertAction( action, id ); \
    action->setEnabled( enab ); \
    action->setIcon( icon ); \
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


int uiMPEMan::popupMenu()
{
    EM::Horizon3D* hor3d = getSelectedHorizon3D();
    if ( !hor3d ) return -1;

    visSurvey::HorizonDisplay* hd = getSelectedDisplay();
    visSurvey::Scene* scene = hd ? hd->getScene() : 0;
    if ( !scene ) return -1;

    uiMenu mnu( tr("Tracking Menu") );
    const bool istracking = MPE::engine().trackingInProgress();
    if ( istracking )
	mAddAction( tr("Stop Auto Tracking"), "k", sStop, "stop", true )
    else
    {
	const Coord3& clickedpos = scene->getMousePos( true );
	const bool haspos = !clickedpos.isUdf();
	mAddAction( tr("Start Auto Tracking"), "k", sStart, "autotrack", true )
	mAddAction( tr("Retrack From Seeds"), "ctrl+k", sRetrack,
		    "retrackhorizon", true )
	mAddAction( tr("Select With Polygon"), "y", sPoly,
		    "polygonselect", true )
	if ( haspos )
	{
	    mAddAction( tr("Select Parents"), "", sParent, 0, true )
	    mAddAction( tr("Show Parents Path"), "", sParPath, 0, true )
	    mAddAction( tr("Select Children"), "", sChild, 0, true )
	}
	mAddAction( tr("Clear Selection"), "a", sClear, "clear", true )
	mAddAction( tr("Delete Selected"), "d", sDelete, "clearselection", true)
	mAddAction( tr("Undo"), "ctrl+z", sUndo, "undo",
		    EM::EMM().undo().canUnDo())
	mAddAction( tr("Redo"), "ctrl+y", sRedo, "redo",
		    EM::EMM().undo().canReDo())
	mAddAction( tr("Lock"), "l", sLock, "lock", true )
	mAddAction( tr("Unlock"), "u", sUnlock, "unlock", true )
	if ( hd->lockedShown() )
	    mAddAction( tr("Hide Locked"), "", sHideLocked, 0, true )
	else
	    mAddAction( tr("Show Locked"), "", sShowLocked, 0, true )
	
	mAddAction( tr("Save"), "ctrl+s", sSave, "save", hor3d->isChanged() )
	mAddAction( tr("Save As ..."), "ctrl+shift+s", sSaveAs, "saveas", true )
	if ( !hd->displayedOnlyAtSections() )
	    mAddAction( tr("Display Only at Sections"), "r", sRest,
			"sectiononly", true )
	else
	    mAddAction( tr("Display in Full"), "r", sFull, "sectionoff", true )
	mAddAction( tr("Show Settings ..."), "", sSett, "tools", true )
    }

    return mnu.exec();
}


void uiMPEMan::handleAction( int res )
{
    MPE::EMTracker* tracker = getSelectedTracker();
    EM::EMObject* emobj =
		tracker ? EM::EMM().getObject(tracker->objectID()) : 0;
    mDynamicCastGet(EM::Horizon3D*,hor3d,emobj)
    if ( !hor3d ) return;

    visSurvey::HorizonDisplay* hd = getSelectedDisplay();
    visSurvey::Scene* scene = hd ? hd->getScene() : 0;
    if ( !scene ) return;

    const Coord3& clickedpos = scene->getMousePos( true );
    const TrcKey tk = SI().transform( clickedpos.coord() );

    switch ( res )
    {
    case sStart: startTracking(); break;
    case sRetrack: startRetrack(); break;
    case sStop: stopTracking(); break;
    case sPoly: changePolySelectionMode(); break;
    case sChild: hor3d->selectChildren(tk); break;
    case sParent: hd->selectParent(tk); break;
    case sParPath: showParentsPath(); break;
    case sClear: clearSelection(); break;
    case sDelete: deleteSelection(); break;
    case sUndo: undo(); break;
    case sRedo: redo(); break;
    case sLock: hor3d->lockAll(); break;
    case sUnlock: hor3d->unlockAll(); break;
    case sShowLocked: hd->showLocked( true ); break;
    case sHideLocked: hd->showLocked( false ); break;
    case sSave: visserv_->storeEMObject( false ); break;
    case sSaveAs: visserv_->storeEMObject( true ); break;
    case sRest: hd->setOnlyAtSectionsDisplay( true ); break;
    case sFull: hd->setOnlyAtSectionsDisplay( false ); break;
    case sSett: showSetupDlg(); break;
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
    mDynamicCastGet(EM::Horizon*,hor,emobj)
    if ( !hor )
	mSeedClickReturn();

    while ( emobj->hasBurstAlert() )
	emobj->setBurstAlert( false );

    if ( !clickcatcher_ )
	mSeedClickReturn();

    const int trackerid =
		MPE::engine().getTrackerByObject( tracker->objectID() );

    const int clickedobject = clickcatcher_->info().getObjID();
    if ( clickedobject == -1 )
	mSeedClickReturn();

    const EM::ObjectID emobjid  = clickcatcher_->info().getEMObjID();
    mDynamicCastGet(EM::Horizon*,clickedhor,EM::EMM().getObject(emobjid))
    const bool clickedonhorizon = clickedhor;
    if ( clickedhor && clickedhor!=hor )
	mSeedClickReturn();

    MPE::EMSeedPicker* seedpicker = tracker->getSeedPicker(true);
    if ( !seedpicker )
	mSeedClickReturn();

    seedpicker->setSectionID( emobj->sectionID(0) );
    if ( clickcatcher_->info().isDoubleClicked() && 
	seedpicker->getTrackMode()==seedpicker->DrawBetweenSeeds )
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
	tracker->getSectionTracker(emobj->sectionID(0), true);
    const Attrib::SelSpec* trackedatsel = sectiontracker
	? sectiontracker->adjuster()->getAttributeSel(0)
	: 0;

    if ( !visserv_->isTrackingSetupActive() && (seedpicker->nrSeeds()==0) )
    {
	if ( trackedatsel &&
	     (seedpicker->getTrackMode()!=seedpicker->DrawBetweenSeeds) )
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
	     (seedpicker->getTrackMode()!=seedpicker->DrawBetweenSeeds) )
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

    const Pos::GeomID geomid = clickcatcher_->info().getGeomID();
    const bool undefgeomid = geomid == Survey::GM().cUndefGeomID();
    TrcKeyValue seedpos( undefgeomid ? SI().transform(seedcrd) : node,
			 (float)seedcrd.z );
    bool shiftclicked = clickcatcher_->info().isShiftClicked();
    
    const Color clr= seedpicker->getTrackMode()==seedpicker->DrawBetweenSeeds ? 
	Color::Green() : emobj->preferredColor();
    if ( !clickedonhorizon && !shiftclicked &&
	 clickcatcher_->activateSower( clr, &seedpicker->getSeedPickArea()) )
    {
	 mSeedClickReturn();
    }

    TrcKeyZSampling newvolume;
    if ( tracker->is2D() )
    {
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

	    if ( clickedas )
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
    beginSeedClickEvent( emobj );

    if ( clickedonhorizon || !clickcatcher_->info().getPickedNode().isUdf() )
    {
	const bool ctrlclicked = clickcatcher_->info().isCtrlClicked();
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
	    if ( seedpicker->addSeed( seedpos, false ) )
		engine.updateFlatCubesContainer( newvolume, trackerid, true );
	}
    }
    else
    {
	if ( seedpicker->getTrackMode()==seedpicker->DrawBetweenSeeds )
	{
	    seedpicker->addSeedToPatch( seedpos );
	    updatePatchDisplay();
	}
	else if ( seedpicker->addSeed(seedpos,shiftclicked) )
	    engine.updateFlatCubesContainer(newvolume,trackerid,true);
    }
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
    if ( clickcatcher_ )
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
	mAttachCB( clickcatcher_->endSowing, uiMPEMan::sowingFinishedCB );
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


void uiMPEMan::sowingFinishedCB( CallBacker* )
{
    MPE::EMTracker* tracker = getSelectedTracker();
    MPE::EMSeedPicker* seedpicker = tracker ? tracker->getSeedPicker(true) : 0;
    if ( !seedpicker ) return;

    if ( seedpicker->getTrackMode()==seedpicker->DrawBetweenSeeds )
    {
	const visBase::EventInfo* eventinfo = clickcatcher_->visInfo();
	const bool doerase = OD::ctrlKeyboardButton( eventinfo->buttonstate_ );
	seedpicker->endPatch( doerase );
	cleanPatchDisplay();
    }

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
			tracker->getSectionTracker( emobj->sectionID(0), true );
    const bool setupavailable = sectiontracker &&
				sectiontracker->hasInitializedSetup();
    if ( setupavailable )
	return;

    seedpicker->setTrackMode( MPE::EMSeedPicker::TrackFromSeeds );
}


void uiMPEMan::cleanPatchDisplay()
{
    visSurvey::HorizonDisplay* hor = getSelectedDisplay();
    if ( hor && hor->getEditor() )
    {
	visSurvey::MPEEditor* editor = hor->getEditor();
	editor->cleanPatch();
    }
}


void uiMPEMan::undo()
{
    MouseCursorChanger mcc( MouseCursor::Wait );
    uiString errmsg;
    MPE::EMTracker* tracker = getSelectedTracker();
    MPE::EMSeedPicker* seedpicker = tracker ? tracker->getSeedPicker(true) : 0;
    if ( seedpicker && seedpicker->canUndo() )
    {
	 seedpicker->horPatchUndo().unDo();
	 updatePatchDisplay();
    }
    else
    {
	uiString undoerrmsg;
	engine().undo( undoerrmsg );
	if ( !undoerrmsg.isEmpty() )
	    uiMSG().message( undoerrmsg );
    }
}


void uiMPEMan::redo()
{
    MouseCursorChanger mcc( MouseCursor::Wait );
    uiString redoerrmsg;
    engine().redo( redoerrmsg );
    if ( !redoerrmsg.isEmpty() )
	uiMSG().message( redoerrmsg );

    MPE::EMTracker* tracker = getSelectedTracker();
    MPE::EMSeedPicker* seedpicker = tracker ? tracker->getSeedPicker(true) : 0;
    if ( seedpicker && seedpicker->canReDo() )
    {
	 seedpicker->horPatchUndo().reDo();
	 updatePatchDisplay();
    }
}


void uiMPEMan::updatePatchDisplay()
{
    MPE::EMTracker* tracker = getSelectedTracker();
    MPE::EMSeedPicker* seedpicker = tracker ? tracker->getSeedPicker(true) : 0;
    if ( !tracker || !seedpicker )
	return;

    visSurvey::HorizonDisplay* hor = getSelectedDisplay();
    if ( hor && hor->getEditor() )
    {
	visSurvey::MPEEditor* editor = hor->getEditor();
	editor->displayPatch( seedpicker->getPatch() );
    }

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


visSurvey::HorizonDisplay* uiMPEMan::getSelectedDisplay()
{
    const TypeSet<int>& selectedids = visBase::DM().selMan().selected();
    if ( selectedids.size() != 1 )
	return 0;

    mDynamicCastGet(visSurvey::HorizonDisplay*,hd,
		    visserv_->getObject(selectedids[0]))
    return hd;
}


visSurvey::Horizon2DDisplay* uiMPEMan::getSelected2DDisplay()
{
    const TypeSet<int>& selectedids = visBase::DM().selMan().selected();
    if ( selectedids.size() != 1 )
	return 0;

    mDynamicCastGet(visSurvey::Horizon2DDisplay*,hd,
		    visserv_->getObject(selectedids[0]))
    return hd;
}


EM::Horizon3D* uiMPEMan::getSelectedHorizon3D()
{
    MPE::EMTracker* tracker = getSelectedTracker();
    EM::EMObject* emobj =
		tracker ? EM::EMM().getObject(tracker->objectID()) : 0;
    mDynamicCastGet(EM::Horizon3D*,hor3d,emobj)
    return hor3d;
}


EM::Horizon2D* uiMPEMan::getSelectedHorizon2D()
{
    MPE::EMTracker* tracker = getSelectedTracker();
    EM::EMObject* emobj =
		tracker ? EM::EMM().getObject(tracker->objectID()) : 0;
    mDynamicCastGet(EM::Horizon2D*,hor2d,emobj)
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


void uiMPEMan::setUndoLevel( int preveventnr )
{
    Undo& emundo = EM::EMM().undo();
    const int currentevent = emundo.currentEventID();
    if ( currentevent != preveventnr )
	    emundo.setUserInteractionEnd(currentevent);
}


