/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "horflatvieweditor3d.h"

#include "emhorizonpainter3d.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emseedpicker.h"
#include "flatauxdataeditor.h"
#include "flatposdata.h"
#include "horflatvieweditor2d.h"
#include "horizon3dtracker.h"
#include "mouseevent.h"
#include "mousecursor.h"
#include "mpeengine.h"
#include "sectionadjuster.h"
#include "sectiontracker.h"
#include "seisdatapack.h"
#include "survinfo.h"
#include "undo.h"

#include "uiflatviewer.h"
#include "uigraphicsview.h"
#include "keyboardevent.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uistrings.h"

namespace MPE
{

HorizonFlatViewEditor3D::HorizonFlatViewEditor3D( FlatView::AuxDataEditor* ed,
						  const EM::ObjectID& emid )
    : editor_(ed)
    , emid_(emid)
    , horpainter_(new EM::HorizonPainter3D(ed->viewer(),emid))
    , updseedpkingstatus_(this)
{
    curcs_.setEmpty();
    mAttachCB( horpainter_->abouttorepaint_,
	       HorizonFlatViewEditor3D::horRepaintATSCB );
    mAttachCB( horpainter_->repaintdone_,
	       HorizonFlatViewEditor3D::horRepaintedCB );
    mAttachCB( editor_->sower().sowingEnd,
	HorizonFlatViewEditor3D::sowingFinishedCB );
    mAttachCB( editor_->sower().sowing,
	HorizonFlatViewEditor3D::sowingModeCB );
    mAttachCB( editor_->movementFinished,
	HorizonFlatViewEditor3D::polygonFinishedCB );
    mAttachCB( editor_->releaseSelection,
	HorizonFlatViewEditor3D::releasePolygonSelectionCB );

    RefMan<EM::EMObject> emobj = EM::EMM().getObject( emid_ );
    if ( emobj )
	mAttachCB(emobj->change,HorizonFlatViewEditor3D::preferColorChangedCB);

    mDynamicCastGet( uiFlatViewer*,vwr, &editor_->viewer() );
    if ( vwr )
    {
	mAttachCB( vwr->rgbCanvas().getKeyboardEventHandler().keyPressed,
		   HorizonFlatViewEditor3D::keyPressedCB );
    }
}


HorizonFlatViewEditor3D::~HorizonFlatViewEditor3D()
{
    detachAllNotifiers();
//	setMouseEventHandler( 0 );
    cleanAuxInfoContainer();
    delete horpainter_;
    deepErase( markeridinfos_ );
    if ( patchdata_ )
	editor_->viewer().removeAuxData( patchdata_ );

    delete patchdata_;
}


void HorizonFlatViewEditor3D::releasePolygonSelectionCB( CallBacker* )
{
    if ( horpainter_ )
	horpainter_->removeSelections();
}


void HorizonFlatViewEditor3D::preferColorChangedCB( CallBacker* cb )
{
    mCBCapsuleUnpack( const EM::EMObjectCallbackData&, cbdata, cb );
    if ( horpainter_ &&
	cbdata.event==EM::EMObjectCallbackData::PrefColorChange )
	horpainter_->updatePreferColors();
}


void HorizonFlatViewEditor3D::setTrcKeyZSampling( const TrcKeyZSampling& cs )
{
    curcs_ = cs;
    horpainter_->setTrcKeyZSampling( cs );
    makePatchEnd( false );
}


void HorizonFlatViewEditor3D::setPath( const TrcKeyPath& path )
{
    curtkpath_ = &path;
    horpainter_->setPath( path );
}


void HorizonFlatViewEditor3D::setFlatPosData( const FlatPosData* fpd )
{
    horpainter_->setFlatPosData( fpd );
}


void HorizonFlatViewEditor3D::setSelSpec( const Attrib::SelSpec* as, bool wva )
{
    if ( !wva )
	vdselspec_ = as;
    else
	wvaselspec_ = as;
}


void HorizonFlatViewEditor3D::setMouseEventHandler( MouseEventHandler* meh )
{
    if ( mehandler_ == meh )
	return;

    if ( mehandler_ )
    {
	mDetachCB( editor_->removeSelected,
		   HorizonFlatViewEditor3D::removePosCB );
	mDetachCB( mehandler_->movement,
		   HorizonFlatViewEditor3D::mouseMoveCB );
	mDetachCB( mehandler_->buttonPressed,
		   HorizonFlatViewEditor3D::mousePressCB );
	mDetachCB( mehandler_->buttonReleased,
		   HorizonFlatViewEditor3D::mouseReleaseCB );
	mDetachCB( mehandler_->doubleClick,
		   HorizonFlatViewEditor3D::doubleClickedCB );
    }

    mehandler_ = meh;

    if ( mehandler_ )
    {
	mAttachCB( editor_->removeSelected,
		   HorizonFlatViewEditor3D::removePosCB );
	mAttachCB( mehandler_->movement,
		   HorizonFlatViewEditor3D::mouseMoveCB );
	mAttachCB( mehandler_->buttonPressed,
		   HorizonFlatViewEditor3D::mousePressCB );
	mAttachCB( mehandler_->buttonReleased,
		   HorizonFlatViewEditor3D::mouseReleaseCB );
	mAttachCB( mehandler_->doubleClick,
		   HorizonFlatViewEditor3D::doubleClickedCB );
    }

    for ( int idx=0; idx<markeridinfos_.size(); idx++ )
	editor_->enablePolySel( markeridinfos_[idx]->markerid_, mehandler_ );
}


void HorizonFlatViewEditor3D::enableLine( bool yn )
{
    horpainter_->enableLine( yn );
}


void HorizonFlatViewEditor3D::enableSeed( bool yn )
{
    horpainter_->enableSeed( yn );
}


bool HorizonFlatViewEditor3D::seedEnable() const
{
    return horpainter_->seedEnable();
}


void HorizonFlatViewEditor3D::paint()
{
    horpainter_->paint();
    fillAuxInfoContainer();
}


static bool allowTracking( const EMTracker* tracker, EM::ObjectID emid )
{
    return tracker && tracker->isEnabled() && tracker->objectID()==emid
		   && !tracker->is2D();
}


void HorizonFlatViewEditor3D::mouseMoveCB( CallBacker* )
{
    const MouseEvent& mouseevent = mehandler_->event();
    if ( !mouseevent.leftButton() )
	return;

    if ( !pickedpos_.isUdf() )
    {
	const Geom::Point2D<int>& mousepos = mouseevent.pos();
	mDynamicCastGet(const uiFlatViewer*,vwr,&editor_->viewer());
	if ( !vwr || !editor_->getMouseArea().isInside(mousepos) )
	    return;

	const Geom::Point2D<double>* markerpos = editor_->markerPosAt(mousepos);
	const uiWorldPoint wp =
	    markerpos ? *markerpos : vwr->getWorld2Ui().transform( mousepos );
	const Coord3 coord = vwr->getCoord( wp );
	RefMan<MPE::EMTracker> tracker = MPE::engine().getActiveTracker();
	if ( !allowTracking(tracker.ptr(),emid_) )
	    return;

	MPE::EMSeedPicker* seedpicker = tracker->getSeedPicker(true);
	if ( !seedpicker )
	    return;

        const TrcKeyValue tkv( TrcKey(SI().transform(coord)), (float)coord.z_ );
	pickedpos_ = seedpicker->replaceSeed( pickedpos_, tkv );
	return;
    }

    if ( editor_ && editor_->sower().accept(mouseevent, false) )
	return;

    RefMan<MPE::EMTracker> tracker = MPE::engine().getTrackerByID( emid_ );
    MPE::engine().setActiveTracker( tracker.ptr() );
}


void HorizonFlatViewEditor3D::mousePressCB( CallBacker* )
{
    if ( !editor_ )
	return;

    mDynamicCastGet(const uiFlatViewer*,vwr,&editor_->viewer());
    if ( !vwr )
	return;

    RefMan<MPE::EMTracker> tracker = MPE::engine().getActiveTracker();
    if ( !allowTracking(tracker.ptr(),emid_) )
	return;

    RefMan<EM::EMObject> emobj = EM::EMM().getObject( emid_ );
    if ( !emobj )
	return;

    const MouseEvent& mouseevent = mehandler_->event();
    const Geom::Point2D<int>& mousepos = mouseevent.pos();
    if ( mouseevent.rightButton() && mouseevent.ctrlStatus() )
    {
	const uiWorldPoint wp = vwr->getWorld2Ui().transform( mousepos );
	const TrcKey tk( SI().transform(vwr->getCoord(wp)) );
	mDynamicCastGet(EM::Horizon3D*,hor3d,emobj.ptr());
	if ( hor3d && hor3d->hasZ(tk) )
	{
	    uiMenu menu;
	    menu.insertAction( new uiAction(tr("Select Children")), 0 );
	    if ( menu.exec() ==0 )
	    {
		hor3d->selectChildren( tk );
		return;
	    }
	}
    }

    if ( editor_->sower().accept(mouseevent,false) || !mouseevent.leftButton() )
	return;

    const bool haspath = curtkpath_ && !curtkpath_->isEmpty();
    const bool nopath = curcs_.isEmpty() && !haspath;
    if ( nopath || !editor_->viewer().appearance().annot_.editable_
			  || editor_->isSelActive() )
	return;

    MPE::EMSeedPicker* seedpicker = tracker->getSeedPicker(true);
    if ( !seedpicker )
	return;

    const Geom::Point2D<double>* markerpos = editor_->markerPosAt( mousepos );
    const bool ctrlorshifclicked =
	mouseevent.shiftStatus() || mouseevent.ctrlStatus();

    if ( seedpicker->getTrackMode()==EMSeedPicker::DrawBetweenSeeds &&
	 markerpos && !ctrlorshifclicked )
    {
	if ( !editor_->getMouseArea().isInside(mousepos) )
	    return;

	const uiWorldPoint wp =
	    markerpos ? *markerpos : vwr->getWorld2Ui().transform( mousepos );
	pickedpos_ = TrcKey( SI().transform( vwr->getCoord(wp) ) );
	return;
    }

    const OD::Color prefcol = emobj->preferredColor();
    const OD::Color sowcolor =
	prefcol != OD::Color::Red() ? OD::Color::Red() : OD::Color::Green();

    if ( editor_ )
    {
	editor_->sower().reInitSettings();
	editor_->sower().setSequentSowMask( true,
			OD::ButtonState(OD::LeftButton+OD::ControlButton) );
	editor_->sower().intersow();
	editor_->sower().reverseSowingOrder();
	if ( editor_->sower().activate(
	    OD::LineStyle( OD::LineStyle::Solid, 4, sowcolor ), mouseevent) )
	    return;
    }
}


void HorizonFlatViewEditor3D::handleMouseClicked( bool dbl )
{
    if ( curcs_.isEmpty() || !editor_->viewer().appearance().annot_.editable_
	 || editor_->isSelActive() )
	return;

    if ( !dbl && !pickedpos_.isUdf() )
    {
	pickedpos_ = TrcKey::udf();
	return;
    }

    RefMan<MPE::EMTracker> tracker = MPE::engine().getActiveTracker();
    if ( !allowTracking(tracker.ptr(),emid_) )
	return;

    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( !emobj ) return;

    MPE::EMSeedPicker* seedpicker = tracker->getSeedPicker(true);
    if ( !seedpicker )
	return;

    const MouseEvent& mouseevent = mehandler_->event();

    if ( !dbl && editor_ )
    {
	const bool sequentsowing = editor_->sower().mode() ==
				   FlatView::Sower::SequentSowing;
	seedpicker->setSowerMode( sequentsowing );
	if ( editor_->sower().accept(mouseevent,true) )
	    return;
    }

    if ( !checkSanity(*tracker.ptr(),*seedpicker,pickinvd_) )
	return;

    const Geom::Point2D<int>& mousepos = mouseevent.pos();
    mDynamicCastGet(const uiFlatViewer*,vwr,&editor_->viewer());
    if ( !vwr || !editor_->getMouseArea().isInside(mousepos) )
	return;

    ConstRefMan<FlatDataPack> dp = vwr->getPack( !pickinvd_ ).get();
    if ( !dp || !prepareTracking(pickinvd_,*tracker.ptr(),*seedpicker,*dp) )
	return;

    const int prevevent = EM::EMM().undo(emobj->id()).currentEventID();
    MouseCursorManager::setOverride( MouseCursor::Wait );
    if ( !emobj->hasBurstAlert() )
	emobj->setBurstAlert( true );

    const Geom::Point2D<double>* markerpos = editor_->markerPosAt( mousepos );
    const uiWorldPoint wp = markerpos ? *markerpos :
			vwr->getWorld2Ui().transform( mousepos );
    Coord3 clickedcrd = vwr->getCoord( wp );
    clickedcrd.z_ = wp.y_;
    const bool action = doTheSeed( *seedpicker, clickedcrd, mouseevent );
    RefMan<MPE::EMTracker> emtracker = MPE::engine().getTrackerByID( emid_ );
    mDynamicCastGet(MPE::Horizon3DTracker*,hor3dtracker,emtracker.ptr());
    hor3dtracker->updateFlatCubesContainer( curcs_, action );

    if ( !editor_->sower().moreToSow() && emobj->hasBurstAlert() &&
	seedpicker->getTrackMode()!=EMSeedPicker::DrawBetweenSeeds &&
	seedpicker->getTrackMode()!=EMSeedPicker::DrawAndSnap )
	emobj->setBurstAlert( false );

    if ( dbl &&
	 seedpicker->getTrackMode()==EMSeedPicker::DrawBetweenSeeds )
	dodropnext_ = true;

    MouseCursorManager::restoreOverride();
    const int currentevent = EM::EMM().undo(emobj->id()).currentEventID();
    if ( !dbl && currentevent != prevevent )
    {
	if ( !editor_ || !editor_->sower().moreToSow() )
	    EM::EMM().undo(emobj->id()).setUserInteractionEnd(currentevent);
    }
}


void HorizonFlatViewEditor3D::doubleClickedCB( CallBacker* cb )
{
    mDynamicCastGet(MouseEventHandler*,meh,cb);
    if ( !meh )
	return;

    const MouseEvent& mev = meh->event();
    if ( !mev.leftButton() )
	return;

    handleMouseClicked( true );

    MPE::EMSeedPicker* seedpicker = getEMSeedPicker();
    if ( !seedpicker )
	return;

    if ( seedpicker->getTrackMode()==EMSeedPicker::DrawBetweenSeeds ||
	 seedpicker->getTrackMode()==seedpicker->DrawAndSnap )
    {
	 makePatchEnd( false );
    }
}


void HorizonFlatViewEditor3D::makePatchEnd( bool doerase )
{
    MPE::EMSeedPicker* seedpicker = getEMSeedPicker();
    if ( !seedpicker )
	return;

    const Patch* patch = seedpicker->getPatch();
    if ( !patch ) return;

    TrcKeySampling tckpath;
    patch->getTrcKeySampling( tckpath );
    if ( tckpath.isEmpty() )
	return;

    horpainter_->setUpdateTrcKeySampling( tckpath );

    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( emobj )
	emobj->setBurstAlert( false );
    seedpicker->endPatch( doerase );
    updatePatchDisplay();
}


EMSeedPicker* HorizonFlatViewEditor3D::getEMSeedPicker() const
{
    RefMan<MPE::EMTracker> tracker = MPE::engine().getActiveTracker();
    if ( !allowTracking(tracker.ptr(),emid_) )
	return nullptr;

    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( !emobj )
	return nullptr;

    EMSeedPicker* picker = tracker->getSeedPicker( true );
    if ( !picker )
	return nullptr;

    return picker;
}


void HorizonFlatViewEditor3D::mouseReleaseCB( CallBacker* cb )
{
    mDynamicCastGet(MouseEventHandler*,meh,cb);
    if ( !meh )
	return;

    const MouseEvent& mev = meh->event();
    if ( !mev.leftButton() )
	return;

    meh->setHandled( true );
    handleMouseClicked( false );
}


void HorizonFlatViewEditor3D::keyPressedCB( CallBacker* cb )
{
    mDynamicCastGet( const KeyboardEventHandler*, keh, cb );
    if ( !keh || !keh->hasEvent() ) return;

    if ( KeyboardEvent::isUnDo(keh->event()) )
	undo();

    if ( KeyboardEvent::isReDo(keh->event()) )
	redo();
}


void HorizonFlatViewEditor3D::undo()
{
    MouseCursorChanger mcc( MouseCursor::Wait );
    MPE::EMSeedPicker* seedpicker = getEMSeedPicker();
    if ( !seedpicker )
	return;

    bool changed = false;
    if ( seedpicker->canUndo() )
    {
	 seedpicker->horPatchUndo().unDo();
	 updatePatchDisplay();
	 changed = true;
    }
    else
    {
	if ( engine().canUnDo() )
	{
	    uiString undoerrmsg;
	    engine().undo( undoerrmsg );
	    if ( !undoerrmsg.isEmpty() )
		uiMSG().message( undoerrmsg );
	    horpainter_->paint();
	    changed = true;
	}
    }

   if ( editor_ && changed )
	editor_->viewer().handleChange( FlatView::Viewer::Auxdata );
}


void HorizonFlatViewEditor3D::redo()
{
    MouseCursorChanger mcc( MouseCursor::Wait );
    MPE::EMSeedPicker* seedpicker = getEMSeedPicker();
    if ( !seedpicker )
	return;

    bool changed = false;
    if ( seedpicker->canReDo() )
    {
	 seedpicker->horPatchUndo().reDo();
	 updatePatchDisplay();
	 changed = true;
    }
    else
    {
	if ( engine().canReDo() )
	{
	    uiString redoerrmsg;
	    engine().redo( redoerrmsg );
	    if ( !redoerrmsg.isEmpty() )
		uiMSG().message( redoerrmsg );
	    horpainter_->paint();
	    changed = true;
	}
    }

    if ( editor_ && changed )
	editor_->viewer().handleChange( FlatView::Viewer::Auxdata );
}


void HorizonFlatViewEditor3D::sowingFinishedCB( CallBacker* )
{
    RefMan<MPE::EMTracker> tracker = MPE::engine().getActiveTracker();
    if ( !allowTracking(tracker.ptr(),emid_) )
	return;

    sowingmode_ = false;
    if ( !mehandler_ )
	return;

    const MouseEvent& mouseevent = mehandler_->event();
    const bool doerase =
	!mouseevent.shiftStatus() && mouseevent.ctrlStatus();
    makePatchEnd( doerase );
}


void HorizonFlatViewEditor3D::sowingModeCB( CallBacker* )
{
    RefMan<MPE::EMTracker> tracker = MPE::engine().getActiveTracker();
    if ( !allowTracking(tracker.ptr(),emid_) )
	return;

    sowingmode_ = true;
}


bool HorizonFlatViewEditor3D::checkSanity( EMTracker& tracker,
					   const EMSeedPicker& spk,
					   bool& pickinvd ) const
{
    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( !emobj ) return false;

    const MPE::SectionTracker* sectiontracker =
				tracker.getSectionTracker(true);

    const Attrib::SelSpec* trackedatsel = sectiontracker
			? sectiontracker->adjuster()->getAttributeSel(0)
			: nullptr;

    Attrib::SelSpec curss;
    if ( trackedatsel )
	curss = *trackedatsel;

    if ( spk.nrSeeds() < 1 )
    {
	if ( !HorizonFlatViewEditor2D::selectSeedData(editor_,pickinvd) )
	    return false;
    }

    const bool vdvisible = editor_->viewer().isVisible(false);
    const bool wvavisible = editor_->viewer().isVisible(true);
    const bool needsdata = spk.getTrackMode() != spk.DrawBetweenSeeds;
    if ( spk.nrSeeds()>0 && trackedatsel && needsdata )
    {
	uiString vdmsg, wvamsg;
	const bool vdres = vdvisible &&
		MPE::engine().pickingOnSameData( curss, *vdselspec_, vdmsg );
	const bool wvares = wvavisible &&
		MPE::engine().pickingOnSameData( curss, *wvaselspec_, wvamsg );
	if ( vdres )
	    pickinvd = true;
	else if ( wvares )
	    pickinvd = false;
	else if ( !vdres && !wvares )
	{
	    const bool res = uiMSG().askContinue( vdmsg );
	    if ( !res )
		return false;

	    const_cast<MPE::EMSeedPicker*>(&spk)->setSelSpec( vdselspec_ );
	    return false;
	}
    }

    return true;
}


bool HorizonFlatViewEditor3D::prepareTracking( bool picinvd,
					       const EMTracker& trker,
					       EMSeedPicker& seedpicker,
					       const FlatDataPack& dp ) const
{
    const Attrib::SelSpec* as = nullptr;
    as = picinvd ? vdselspec_ : wvaselspec_;

    if ( !seedpicker.startSeedPick() )
	return false;

    NotifyStopper notifystopper( MPE::engine().activevolumechange );
    MPE::engine().setActiveVolume( curcs_ );
    mDynamicCastGet(const RandomSeisFlatDataPack*,randfdp,&dp);
    MPE::engine().setActivePath( randfdp ? &randfdp->getPath() : nullptr );
    MPE::engine().setActiveRandomLineID( randfdp ? randfdp->getRandomLineID()
						 : RandomLineID::udf() );
    notifystopper.enableNotification();

    seedpicker.setSelSpec( as );
    MPE::engine().setAttribData( *as, dp );
    MPE::engine().activevolumechange.trigger();

    return true;
}


bool HorizonFlatViewEditor3D::getPosID( const Coord3& crd,
					EM::PosID& pid ) const
{
    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( !emobj ) return false;

    BinID bid = SI().transform( crd );

    for ( int idx=0; idx<emobj->nrSections(); idx++ )
    {
	if ( emobj->isDefined(bid.toInt64()) )
	{
	    pid.setObjectID( emobj->id() );
	    pid.setSubID( bid.toInt64() );
	    return true;
	}
    }

    return false;
}


bool HorizonFlatViewEditor3D::doTheSeed( EMSeedPicker& spk, const Coord3& crd,
					 const MouseEvent& mev )
{
    const TrcKeyValue tkv( TrcKey(SI().transform(crd)), (float)crd.z_ );
    const bool ismarker = editor_->markerPosAt( mev.pos() );
    if ( !ismarker )
    {
	bool drop = ismarker ? false : mev.shiftStatus();
	if ( dodropnext_ )
	{
	    drop = dodropnext_;
	    dodropnext_ = false;
	}

	const TrcKeyValue tkv2( TrcKey(SI().transform(Coord(mev.x(),mev.y()))),
				0.f );
	const MouseEvent& mouseevent = mehandler_->event();

	const bool doerase =
	    !mouseevent.shiftStatus() && mouseevent.ctrlStatus() && sowingmode_;
	const bool manualmodeclick = !mouseevent.ctrlStatus() &&
	    ( spk.getTrackMode()==spk.DrawBetweenSeeds ||
	     spk.getTrackMode()==spk.DrawAndSnap );

	if ( doerase || manualmodeclick )
	{
	    spk.addSeedToPatch( tkv, true );
	    RefMan<MPE::EMTracker> tracker = MPE::engine().getActiveTracker();
	    if ( tracker )
	    {
		const MPE::EMSeedPicker* seedpicker =
					    tracker->getSeedPicker(true);
		if ( seedpicker && !seedpicker->getSowerMode() )
		    updatePatchDisplay();
	    }
	}
	else if ( !sowingmode_ && !mouseevent.ctrlStatus() )
	{
		spk.addSeed( tkv, drop, tkv2 );
	}
	else if ( sowingmode_ )
	{
	    spk.addSeedToPatch( tkv, false );
	}
    }
    else if ( mev.shiftStatus() || mev.ctrlStatus() )
    {
	const bool ctrlshifclicked = mev.shiftStatus() && mev.ctrlStatus();
	if ( spk.removeSeed(tkv.tk_,true,!ctrlshifclicked) )
	    return false;
    }

    return true;
}


void HorizonFlatViewEditor3D::setupPatchDisplay()
{
    RefMan<EM::EMObject> emobj = EM::EMM().getObject(emid_);
    if ( !emobj || !editor_ ) return;

    OD::Color patchcolor = OD::Color::Red();
    const OD::Color mkclr = emobj->preferredColor();
    if ( Math::Abs(patchcolor.g()-mkclr.g())<30 )
	    patchcolor = OD::Color::Green();

    if ( !patchdata_ )
    {
	patchdata_ = editor_->viewer().createAuxData(0);
	editor_->viewer().addAuxData(patchdata_);
    }
    patchdata_->empty();
    patchdata_->enabled_ = true;

    const int linewidth = sowingmode_ ? 0 : 4;
    patchdata_->linestyle_ =
	OD::LineStyle( OD::LineStyle::Solid, linewidth, patchcolor );
}


void HorizonFlatViewEditor3D::updatePatchDisplay()
{
    MPE::EMSeedPicker* seedpicker = getEMSeedPicker();
    if ( !seedpicker )
	return;

    const Patch* patch = seedpicker->getPatch();
    if ( !patch )
	return;

    setupPatchDisplay();

    RefMan<EM::EMObject> emobj = EM::EMM().getObject(emid_);
    if ( !emobj ) return;

    TypeSet<TrcKeyValue> path = patch->getPath();
    ConstRefMan<FlatDataPack> fdp =
				editor_->viewer().getPack( true, true ).get();
    mDynamicCastGet(const RandomSeisFlatDataPack*,randfdp,fdp.ptr());
    for ( int idx=0; idx<path.size(); idx++ )
    {
	const TrcKeyValue tkzs = path[idx];
	if ( tkzs.isUdf() )
	    continue;
	double x= 0.0;
	if ( randfdp )
	{
	    const int bidindex = randfdp->getNearestGlobalIdx( tkzs.tk_ );
	    const FlatPosData& flatposdata = randfdp->posData();
	    x = flatposdata.position( true, bidindex );
	}
	else if ( curcs_.nrInl()==1 )
	    x = tkzs.tk_.trcNr();
	else if ( curcs_.nrCrl()==1 )
	    x = tkzs.tk_.lineNr();

	MarkerStyle2D markerstyle(
	    MarkerStyle2D::Square, 4, emobj->preferredColor() );
	patchdata_->markerstyles_ += markerstyle;
	patchdata_->poly_ += FlatView::Point( x, tkzs.val_ );
    }
    editor_->viewer().handleChange( FlatView::Viewer::Auxdata );
}


void HorizonFlatViewEditor3D::cleanAuxInfoContainer()
{
    for ( int idx=0; idx<markeridinfos_.size(); idx++ )
	editor_->removeAuxData( markeridinfos_[idx]->markerid_ );

    if ( markeridinfos_.size() )
	deepErase( markeridinfos_ );
}


void HorizonFlatViewEditor3D::fillAuxInfoContainer()
{
    cleanAuxInfoContainer();
    ObjectSet<EM::HorizonPainter3D::Marker3D> disphormrkinfos;
    horpainter_->getDisplayedHor( disphormrkinfos );

    for ( int idx=0; idx<disphormrkinfos.size(); idx++ )
    {
	Hor3DMarkerIdInfo* markeridinfo = new Hor3DMarkerIdInfo;
	markeridinfo->markerid_ = editor_->addAuxData(
					disphormrkinfos[idx]->marker_, true );
	markeridinfo->marker_ = disphormrkinfos[idx]->marker_;
	editor_->enableEdit( markeridinfo->markerid_, false, false, false );
	editor_->enablePolySel( markeridinfo->markerid_, mehandler_ );

	markeridinfos_ += markeridinfo;
    }
}


void HorizonFlatViewEditor3D::horRepaintATSCB( CallBacker* )
{
    cleanAuxInfoContainer();
}


void HorizonFlatViewEditor3D::horRepaintedCB( CallBacker* )
{
    fillAuxInfoContainer();
}


FlatView::AuxData* HorizonFlatViewEditor3D::getAuxData( int markid )
{
    for ( int idx=0; idx<markeridinfos_.size(); idx++ )
    {
	if ( markeridinfos_[idx]->markerid_ == markid )
	    return markeridinfos_[idx]->marker_;
    }

    return 0;
}


EM::SectionID HorizonFlatViewEditor3D::getSectionID( int )
{
    return EM::SectionID::def();
}


void HorizonFlatViewEditor3D::movementEndCB( CallBacker* )
{}


void HorizonFlatViewEditor3D::removePosCB( CallBacker* )
{
    if ( pointselections_.isEmpty() )
	return;

    RefMan<EM::EMObject> emobj = EM::EMM().getObject( emid_ );
    if ( !emobj ) return;

    mDynamicCastGet( EM::Horizon3D*, hor3d,emobj.ptr() );
    if ( !hor3d ) return;

    Undo& doundo = EM::EMM().undo( emobj->id() );
    const int lastid = doundo.currentEventID();

    hor3d->setBurstAlert( true );
    for ( int idx=0; idx<pointselections_.size(); idx++ )
	emobj->unSetPos( pointselections_[idx], true );

    hor3d->setBurstAlert( false );

    if ( lastid!=doundo.currentEventID() )
	doundo.setUserInteractionEnd( doundo.currentEventID() );

    horpainter_->removeSelections();
}


void HorizonFlatViewEditor3D::polygonFinishedCB( CallBacker* )
{
    if ( editor_->getSelPtDataID()!=-1 )
	return;

    pointselections_.setEmpty();

    TypeSet<int> selectedids;
    TypeSet<int> selectedidxs;
    editor_->getPointSelections( selectedids, selectedidxs );

    editor_->setSelectionPolygonVisible( false );

    if ( !selectedids.size() && horpainter_ )
    {
	horpainter_->removeSelections();
	return;
    }

    BinID bid;
    ConstRefMan<FlatDataPack> fdp =
			      editor_->viewer().getPack( true, true ).get();
    mDynamicCastGet( const RandomSeisFlatDataPack*, randfdp, fdp.ptr() );

    for ( int ids=0; ids<selectedids.size(); ids++ )
    {
	const FlatView::AuxData* auxdata = getAuxData(selectedids[ids]);
	if ( !auxdata || !auxdata->poly_.validIdx(selectedidxs[ids]) ) continue;

        const double posx = auxdata->poly_[selectedidxs[ids]].x_;
	if ( curcs_.nrInl() == 1 )
	{
	    bid.inl() = curcs_.hsamp_.start_.inl();
	    bid.crl() = mNINT32(posx);
	}
	else if ( curcs_.nrCrl() == 1 )
	{
	    bid.inl() = mNINT32(posx);
	    bid.crl() = curcs_.hsamp_.start_.crl();
	}
	else if ( randfdp )
	{
	    const TrcKeyPath& rdlpath = randfdp->getPath();
	    IndexInfo ix = randfdp->posData().indexInfo( true, posx );
	    bid = rdlpath[ix.nearest_].position();
	}

	EM::PosID posid( emid_, bid );
	pointselections_ += posid;
    }

    if ( pointselections_.size()>0 )
	horpainter_->displaySelections( pointselections_ );

}


} // namespace MPE
