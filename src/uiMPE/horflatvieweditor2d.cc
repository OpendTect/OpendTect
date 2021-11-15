/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		May 2010
________________________________________________________________________

-*/

#include "horflatvieweditor2d.h"

#include "attribstorprovider.h"
#include "emhorizon2d.h"
#include "emhorizonpainter2d.h"
#include "emobject.h"
#include "emmanager.h"
#include "emseedpicker.h"
#include "emtracker.h"
#include "flatauxdataeditor.h"
#include "horizon2dseedpicker.h"
#include "ioman.h"
#include "ioobj.h"
#include "linesetposinfo.h"
#include "mouseevent.h"
#include "mousecursor.h"
#include "mpeengine.h"
#include "posinfo2d.h"
#include "sectionadjuster.h"
#include "sectiontracker.h"
#include "survgeom2d.h"
#include "survinfo.h"
#include "undo.h"
#include "uiflatviewer.h"
#include "uigraphicsview.h"
#include "keyboardevent.h"
#include "uimsg.h"
#include "uistrings.h"

namespace MPE
{

HorizonFlatViewEditor2D::HorizonFlatViewEditor2D( FlatView::AuxDataEditor* ed,
						  const EM::ObjectID& emid )
    : editor_(ed)
    , emid_(emid)
    , horpainter_( new EM::HorizonPainter2D(ed->viewer(),emid) )
    , mehandler_(0)
    , vdselspec_(0)
    , wvaselspec_(0)
    , geomid_(Survey::GeometryManager::cUndefGeomID())
    , seedpickingon_(false)
    , trackersetupactive_(false)
    , dodropnext_(false)
    , pickedpos_(TrcKey::udf())
    , updseedpkingstatus_(this)
    , patchdata_(0)
    , sowingmode_(false)
    , pickinvd_(true)
{
    curcs_.setEmpty();
    horpainter_->abouttorepaint_.notify(
	    mCB(this,HorizonFlatViewEditor2D,horRepaintATSCB) );
    horpainter_->repaintdone_.notify(
	    mCB(this,HorizonFlatViewEditor2D,horRepaintedCB) );
    mAttachCB( editor_->sower().sowingEnd,
	HorizonFlatViewEditor2D::sowingFinishedCB );
    mAttachCB( editor_->sower().sowing,
	HorizonFlatViewEditor2D::sowingModeCB );
    mAttachCB( editor_->releaseSelection,
	HorizonFlatViewEditor2D::releasePolygonSelectionCB );

    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( emobj )
	mAttachCB(emobj->change,HorizonFlatViewEditor2D::preferColorChangedCB);

    mAttachCB( editor_->movementFinished,
	HorizonFlatViewEditor2D::polygonFinishedCB );
    mDynamicCastGet( uiFlatViewer*,vwr, &editor_->viewer() );
    if ( vwr )
    mAttachCB(
	vwr->rgbCanvas().getKeyboardEventHandler().keyPressed,
	HorizonFlatViewEditor2D::keyPressedCB );
}


HorizonFlatViewEditor2D::~HorizonFlatViewEditor2D()
{
    detachAllNotifiers();
    if ( mehandler_ )
    {
	editor_->removeSelected.remove(
		mCB(this,HorizonFlatViewEditor2D,removePosCB) );
	mehandler_->movement.remove(
		mCB(this,HorizonFlatViewEditor2D,mouseMoveCB) );
	mehandler_->buttonPressed.remove(
		mCB(this,HorizonFlatViewEditor2D,mousePressCB) );
	mehandler_->buttonReleased.remove(
		mCB(this,HorizonFlatViewEditor2D,mouseReleaseCB) );
	mehandler_->doubleClick.remove(
		mCB(this,HorizonFlatViewEditor2D,doubleClickedCB) );
    }
//	setMouseEventHandler( 0 );
    cleanAuxInfoContainer();
    delete horpainter_;
    deepErase( markeridinfos_ );
    if ( patchdata_ )
    {
	editor_->viewer().removeAuxData( patchdata_ );
	delete patchdata_;
	patchdata_ = 0;
    }
}


void HorizonFlatViewEditor2D::releasePolygonSelectionCB( CallBacker* )
{
    if ( horpainter_ )
	horpainter_->removeSelections();
}


void HorizonFlatViewEditor2D::preferColorChangedCB( CallBacker* cb )
{
    mCBCapsuleUnpack( const EM::EMObjectCallbackData&, cbdata, cb );
    if ( horpainter_ &&
	cbdata.event==EM::EMObjectCallbackData::PrefColorChange )
	horpainter_->updatePreferColors();
}


void HorizonFlatViewEditor2D::setTrcKeyZSampling( const TrcKeyZSampling& cs )
{
    curcs_ = cs;
    horpainter_->setTrcKeyZSampling( cs );
}


void HorizonFlatViewEditor2D::setGeomID( Pos::GeomID geomid )
{
    geomid_ = geomid;
    horpainter_->setGeomID( geomid );
}


TypeSet<int>& HorizonFlatViewEditor2D::getPaintingCanvTrcNos()
{
    return horpainter_->getTrcNos();
}


TypeSet<float>& HorizonFlatViewEditor2D::getPaintingCanDistances()
{
    return horpainter_->getDistances();
}


void HorizonFlatViewEditor2D::enableLine( bool yn )
{
    horpainter_->enableLine( yn );
}


void HorizonFlatViewEditor2D::enableSeed( bool yn )
{
    horpainter_->enableSeed( yn );
}


void HorizonFlatViewEditor2D::enableIntersectionMarker( bool yn )
{
    horpainter_->displayIntersection( yn );
}


bool HorizonFlatViewEditor2D::seedEnable() const
{
    return horpainter_->seedEnable();
}


void HorizonFlatViewEditor2D::paint()
{
    horpainter_->setLine2DInterSectionSet( line2dintersectionset_ );
    horpainter_->paint();
    fillAuxInfoContainer();
}


void HorizonFlatViewEditor2D::setSelSpec( const Attrib::SelSpec* as, bool wva )
{
    if ( !wva )
	vdselspec_ = as;
    else
	wvaselspec_ = as;
}


void HorizonFlatViewEditor2D::setMouseEventHandler( MouseEventHandler* meh )
{
    if ( mehandler_ )
    {
	editor_->removeSelected.remove(
		mCB(this,HorizonFlatViewEditor2D,removePosCB) );
	mehandler_->movement.remove(
		mCB(this,HorizonFlatViewEditor2D,mouseMoveCB) );
	mehandler_->buttonPressed.remove(
		mCB(this,HorizonFlatViewEditor2D,mousePressCB) );
	mehandler_->buttonReleased.remove(
		mCB(this,HorizonFlatViewEditor2D,mouseReleaseCB) );
	mehandler_->doubleClick.remove(
		mCB(this,HorizonFlatViewEditor2D,doubleClickedCB) );
    }

    mehandler_ = meh;

    if ( mehandler_ )
    {
	editor_->removeSelected.notify(
		mCB(this,HorizonFlatViewEditor2D,removePosCB) );
	mehandler_->movement.notify(
		mCB(this,HorizonFlatViewEditor2D,mouseMoveCB) );
	mehandler_->buttonPressed.notify(
		mCB(this,HorizonFlatViewEditor2D,mousePressCB) );
	mehandler_->buttonReleased.notify(
		mCB(this,HorizonFlatViewEditor2D,mouseReleaseCB) );
	mehandler_->doubleClick.notify(
		mCB(this,HorizonFlatViewEditor2D,doubleClickedCB) );
    }

    for ( int idx=0; idx<markeridinfos_.size(); idx++ )
	editor_->enablePolySel( markeridinfos_[idx]->markerid_, mehandler_ );
}


void HorizonFlatViewEditor2D::setSeedPicking( bool yn )
{ seedpickingon_ = yn; }


static bool allowTracking( const EMTracker* tracker, EM::ObjectID emid )
{
    return tracker && tracker->isEnabled() && tracker->objectID()==emid
		   && tracker->is2D();
}


void HorizonFlatViewEditor2D::mouseMoveCB( CallBacker* )
{
    const MouseEvent& mouseevent = mehandler_->event();

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
	MPE::EMTracker* tracker = MPE::engine().getActiveTracker();
	if ( !allowTracking(tracker,emid_) )
	    return;

	MPE::EMSeedPicker* seedpicker = tracker->getSeedPicker(true);
	if ( !seedpicker )
	    return;

	const TrcKeyValue tkv( TrcKey(SI().transform(coord)), (float)coord.z );
	pickedpos_ = seedpicker->replaceSeed( pickedpos_, tkv );
	return;
    }

    if ( editor_ && editor_->sower().accept(mouseevent, false) )
	return;

    if ( MPE::engine().getTrackerByObject(emid_) != -1 )
    {
	int trackeridx = MPE::engine().getTrackerByObject( emid_ );
	if ( MPE::engine().getTracker(trackeridx) )
	    MPE::engine().setActiveTracker( emid_ );
    }
    else
	MPE::engine().setActiveTracker( -1 );
}


void HorizonFlatViewEditor2D::mousePressCB( CallBacker* )
{
    const MouseEvent& mouseevent = mehandler_->event();
    if ( (editor_ && editor_->sower().accept(mehandler_->event(),false)) ||
	  mouseevent.middleButton() )
	return;

    if ( curcs_.isEmpty() || !editor_->viewer().appearance().annot_.editable_
			  || editor_->isSelActive() )
	return;

    MPE::EMTracker* tracker = MPE::engine().getActiveTracker();
    if ( !allowTracking(tracker,emid_) )
	return;

    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( !emobj ) return;

    MPE::EMSeedPicker* seedpicker = tracker->getSeedPicker(true);
    if ( !seedpicker )
	return;

    seedpicker->setSectionID( emobj->sectionID(0) );

    mDynamicCastGet(const uiFlatViewer*,vwr,&editor_->viewer());
    if ( !vwr ) return;

    const Geom::Point2D<int>& mousepos = mouseevent.pos();
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
	prefcol !=OD::Color::Red() ? OD::Color::Red() : OD::Color::Green();

    if ( editor_ )
    {
	editor_->sower().reInitSettings();
	editor_->sower().setSequentSowMask(
	    true,OD::ButtonState( OD::LeftButton+OD::ControlButton) );
	editor_->sower().intersow();
	editor_->sower().reverseSowingOrder();
	if ( editor_->sower().activate(
	    OD::LineStyle( OD::LineStyle::Solid, 4, sowcolor ), mouseevent) )
	    return;
    }
}


void HorizonFlatViewEditor2D::doubleClickedCB( CallBacker* cb )
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
	EM::EMObject* emobj = EM::EMM().getObject( emid_ );
	if ( !emobj ) return;
	emobj->setBurstAlert( false );
	seedpicker->endPatch( false );
	updatePatchDisplay();
    }
}


EMSeedPicker* HorizonFlatViewEditor2D::getEMSeedPicker() const
{
    MPE::EMTracker* tracker = MPE::engine().getActiveTracker();
    if ( !allowTracking(tracker,emid_) )
	return 0;

    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( !emobj ) return 0;

    EMSeedPicker* picker = tracker->getSeedPicker( true );
    if ( !picker ) return 0;

    picker->setSectionID( emobj->sectionID(0) );
    return picker;
}


void HorizonFlatViewEditor2D::mouseReleaseCB( CallBacker* cb )
{
    mDynamicCastGet(MouseEventHandler*,meh,cb);
    if ( !meh )
	return;

    const MouseEvent& mev = meh->event();
    if ( !mev.leftButton() )
	return;

    handleMouseClicked( false );
}


void HorizonFlatViewEditor2D::keyPressedCB( CallBacker* cb )
{
    mDynamicCastGet( const KeyboardEventHandler*, keh, cb );
    if ( !keh || !keh->hasEvent() ) return;

    if ( KeyboardEvent::isUnDo(keh->event()) )
	undo();

    if ( KeyboardEvent::isReDo(keh->event()) )
	redo();
}


void HorizonFlatViewEditor2D::undo()
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


void HorizonFlatViewEditor2D::redo()
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
	    engine().redo(redoerrmsg);
	    if ( !redoerrmsg.isEmpty() )
		uiMSG().message( redoerrmsg );
	    horpainter_->paint();
	    changed = true;
	}
    }

    if ( editor_ && changed )
	editor_->viewer().handleChange( FlatView::Viewer::Auxdata );
}


void HorizonFlatViewEditor2D::sowingFinishedCB( CallBacker* )
{
    MPE::EMTracker* tracker = MPE::engine().getActiveTracker();
    if ( !allowTracking(tracker,emid_) )
	return;

    sowingmode_ = false;

    MPE::EMSeedPicker* seedpicker = getEMSeedPicker();
    if ( !seedpicker || !mehandler_ )
	return;

    const MouseEvent& mouseevent = mehandler_->event();
    const bool doerase = !mouseevent.shiftStatus() && mouseevent.ctrlStatus();
    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( !emobj ) return;
    emobj->setBurstAlert( false );
    seedpicker->endPatch( doerase );
    updatePatchDisplay();
}


void HorizonFlatViewEditor2D::sowingModeCB( CallBacker* )
{
    MPE::EMTracker* tracker = MPE::engine().getActiveTracker();
    if ( !allowTracking(tracker,emid_) )
	return;

    sowingmode_ = true;
}


void HorizonFlatViewEditor2D::handleMouseClicked( bool dbl )
{
    if ( curcs_.isEmpty() || !editor_->viewer().appearance().annot_.editable_
	 || editor_->isSelActive() )
	return;

    if ( !dbl && !pickedpos_.isUdf() )
    {
	pickedpos_ = TrcKey::udf();
	return;
    }

    MPE::EMTracker* tracker = MPE::engine().getActiveTracker();
    if ( !allowTracking(tracker,emid_) )
	return;

    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( !emobj )
	return;

    MPE::EMSeedPicker* seedpicker = tracker->getSeedPicker(true);
    if ( !seedpicker )
	return;

    seedpicker->setSectionID( emobj->sectionID(0) );

    const MouseEvent& mouseevent = mehandler_->event();
    if ( !dbl && editor_ )
    {
	const bool sequentsowing = editor_->sower().mode() ==
				   FlatView::Sower::SequentSowing;
	seedpicker->setSowerMode( sequentsowing );
	if ( editor_->sower().accept(mouseevent,true) )
	    return;
    }

    if ( !checkSanity(*tracker,*seedpicker,pickinvd_) )
	return;

    const Geom::Point2D<int>& mousepos = mouseevent.pos();
    mDynamicCastGet(const uiFlatViewer*,vwr,&editor_->viewer());
    if ( !vwr || !editor_->getMouseArea().isInside(mousepos) )
	return;

    ConstDataPackRef<FlatDataPack> dp = vwr->obtainPack( !pickinvd_ );
    if ( !dp || !prepareTracking(pickinvd_,*tracker,*seedpicker,*dp) )
	return;

    const int prevevent = EM::EMM().undo(emobj->id()).currentEventID();
    MouseCursorManager::setOverride( MouseCursor::Wait );
    if ( !emobj->hasBurstAlert() )
	emobj->setBurstAlert( true );

    const Geom::Point2D<double>* markerpos = editor_->markerPosAt( mousepos );
    const uiWorldPoint wp = markerpos ? *markerpos :
			vwr->getWorld2Ui().transform(mousepos);
    Coord3 clickedcrd = vwr->getCoord(wp);
    clickedcrd.z = wp.y;
    doTheSeed( *seedpicker, clickedcrd, mouseevent );

    if ( !editor_->sower().moreToSow() && emobj->hasBurstAlert() &&
	seedpicker->getTrackMode()!=EMSeedPicker::DrawBetweenSeeds &&
	seedpicker->getTrackMode()!=EMSeedPicker::DrawAndSnap )
	emobj->setBurstAlert( false );

    if ( dbl && seedpicker->getTrackMode()==EMSeedPicker::DrawBetweenSeeds )
	dodropnext_ = true;

    MouseCursorManager::restoreOverride();
    const int currentevent = EM::EMM().undo(emobj->id()).currentEventID();
    if ( !dbl && currentevent != prevevent )
    {
	if ( !editor_ || !editor_->sower().moreToSow() )
	    EM::EMM().undo(emobj->id()).setUserInteractionEnd(currentevent);
    }
}


bool HorizonFlatViewEditor2D::selectSeedData(
		const FlatView::AuxDataEditor* editor, bool& pickinvd )
{
    if ( !editor )
	return false;

    const bool vdvisible = editor->viewer().isVisible(false);
    const bool wvavisible = editor->viewer().isVisible(true);

    if ( vdvisible && wvavisible )
	pickinvd = uiMSG().question( tr("Which one is your seed data?"),
				     tr("VD"), uiStrings::sWiggle() );
    else if ( vdvisible )
	pickinvd = true;
    else if ( wvavisible )
	pickinvd = false;
    else
    {
	uiMSG().error( tr("No data to choose from") );
	return false;
    }

    return true;
}


bool HorizonFlatViewEditor2D::checkSanity( EMTracker& tracker,
					   const EMSeedPicker& spk,
					   bool& pickinvd ) const
{
    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( !emobj ) return false;

    const MPE::SectionTracker* sectiontracker =
	tracker.getSectionTracker(emobj->sectionID(0), true);

    const Attrib::SelSpec* trackedatsel = sectiontracker
			? sectiontracker->adjuster()->getAttributeSel(0)
			: 0;

    Attrib::SelSpec curss;
    if ( trackedatsel )
	curss = *trackedatsel;

    if ( spk.nrSeeds() < 1 )
    {
	if ( !selectSeedData(editor_,pickinvd) )
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


bool HorizonFlatViewEditor2D::prepareTracking( bool picinvd,
					       const EMTracker& trker,
					       EMSeedPicker& seedpicker,
					       const FlatDataPack& dp ) const
{
    const Attrib::SelSpec* as = 0;
    as = picinvd ? vdselspec_ : wvaselspec_;

    MPE::engine().setActive2DLine( geomid_ );
    mDynamicCastGet(MPE::Horizon2DSeedPicker*,h2dsp,&seedpicker);
    if ( h2dsp )
	h2dsp->setSelSpec( as );

    if ( dp.id() > DataPack::cNoID() )
	MPE::engine().setAttribData( *as, dp.id() );

    if ( !h2dsp || !h2dsp->canAddSeed(*as) )
	return false;

    h2dsp->setLine( geomid_ );
    if ( !h2dsp->startSeedPick() )
	return false;

    return true;
}


bool HorizonFlatViewEditor2D::doTheSeed( EMSeedPicker& spk, const Coord3& crd,
					 const MouseEvent& mev )
{
    const TrcKeyValue tkv( getTrcKey(crd.coord()), (float)crd.z );
    const bool ismarker = editor_->markerPosAt( mev.pos() );

    if ( !ismarker )
    {
	bool drop = ismarker ? false : mev.shiftStatus();
	if ( dodropnext_ )
	{
	    drop = dodropnext_;
	    dodropnext_ = false;
	}

	const TrcKeyValue tkv2( getTrcKey(Coord(mev.x(),mev.y())), 0.f );
	const MouseEvent& mouseevent = mehandler_->event();
	const bool doerase =
	    !mouseevent.shiftStatus() && mouseevent.ctrlStatus() && sowingmode_;
	const bool manualmodeclick = !mouseevent.ctrlStatus() &&
	    ( spk.getTrackMode()==spk.DrawBetweenSeeds ||
	     spk.getTrackMode()==spk.DrawAndSnap );

	if ( doerase || manualmodeclick )
	{
	    spk.addSeedToPatch( tkv, true );
	    MPE::EMTracker* tracker = MPE::engine().getActiveTracker();
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


void HorizonFlatViewEditor2D::setupPatchDisplay()
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


void HorizonFlatViewEditor2D::updatePatchDisplay()
{
    MPE::EMTracker* tracker = MPE::engine().getActiveTracker();
    if ( !allowTracking(tracker,emid_) )
	return;

    MPE::EMSeedPicker* seedpicker = tracker->getSeedPicker( true );
    if ( !seedpicker )
	return;

    const Patch* patch = seedpicker->getPatch();
    if ( !patch )
	return;

    setupPatchDisplay();

    const EM::EMObject* emobj = EM::EMM().getObject(emid_);
    if ( !emobj ) return;

    TypeSet<TrcKeyValue> path = patch->getPath();
    for ( int idx=0; idx<path.size(); idx++ )
    {
	const TrcKeyValue tkzs = path[idx];
	if ( tkzs.isUdf() )
	    continue;
	const TrcKey tk = tkzs.tk_;
	const int pidx = horpainter_->getTrcNos().indexOf(tk.crl());
	if ( pidx == -1 )
	    continue;
	const double x = horpainter_->getDistances()[pidx];

	MarkerStyle2D markerstyle(
		MarkerStyle2D::Square, 4, emobj->preferredColor() );
	patchdata_->markerstyles_ += markerstyle;
	patchdata_->poly_ += FlatView::Point( x, tkzs.val_ );
    }
    editor_->viewer().handleChange( FlatView::Viewer::Auxdata );
}


TrcKey HorizonFlatViewEditor2D::getTrcKey( const Coord& crd ) const
{
    const Survey::Geometry* geom = Survey::GM().getGeometry( geomid_ );
    return geom ? geom->nearestTrace( crd ) : TrcKey::udf();
}


void HorizonFlatViewEditor2D::cleanAuxInfoContainer()
{
    for ( int idx=0; idx<markeridinfos_.size(); idx++ )
	editor_->removeAuxData( markeridinfos_[idx]->markerid_ );

    if ( markeridinfos_.size() )
	deepErase( markeridinfos_ );
}


void HorizonFlatViewEditor2D::fillAuxInfoContainer()
{
    ObjectSet<EM::HorizonPainter2D::Marker2D> disphormrkinfos;
    horpainter_->getDisplayedHor( disphormrkinfos );

    for ( int idx=0; idx<disphormrkinfos.size(); idx++ )
    {
	Hor2DMarkerIdInfo* markeridinfo = new Hor2DMarkerIdInfo;
	markeridinfo->markerid_ = editor_->addAuxData(
					disphormrkinfos[idx]->marker_, true );
	markeridinfo->marker_ = disphormrkinfos[idx]->marker_;
	markeridinfo->sectionid_ = disphormrkinfos[idx]->sectionid_;
	editor_->enableEdit( markeridinfo->markerid_, false, false, false );
	editor_->enablePolySel( markeridinfo->markerid_, mehandler_ );

	markeridinfos_ += markeridinfo;
    }
}


void HorizonFlatViewEditor2D::horRepaintATSCB( CallBacker* )
{
    cleanAuxInfoContainer();
}


void HorizonFlatViewEditor2D::horRepaintedCB( CallBacker* )
{
    fillAuxInfoContainer();
}


FlatView::AuxData* HorizonFlatViewEditor2D::getAuxData( int markid )
{
    for ( int idx=0; idx<markeridinfos_.size(); idx++ )
    {
	if ( markeridinfos_[idx]->markerid_ == markid )
	    return markeridinfos_[idx]->marker_;
    }

    return 0;
}


EM::SectionID HorizonFlatViewEditor2D::getSectionID( int markid )
{
    for ( int idx=0; idx<markeridinfos_.size(); idx++ )
    {
	if ( markeridinfos_[idx]->markerid_ == markid )
	    return markeridinfos_[idx]->sectionid_;
    }

    return -1;
}


bool HorizonFlatViewEditor2D::getPosID( const Coord3& crd,
					EM::PosID& pid ) const
{
    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( !emobj ) return false;

    mDynamicCastGet(const Survey::Geometry2D*,geom2d,
		    Survey::GM().getGeometry(geomid_) );
    if ( !geom2d )
	return false;

    PosInfo::Line2DPos pos;
    geom2d->data().getPos( crd, pos, mUdf(float) );
    mDynamicCastGet(const EM::Horizon2D*,hor2d,emobj);

    if ( !hor2d ) return false;

    BinID bid;
    bid.inl() = hor2d->geometry().lineIndex( geomid_ );
    bid.crl() = pos.nr_;

    for ( int idx=0; idx<emobj->nrSections(); idx++ )
    {
	if ( emobj->isDefined(emobj->sectionID(idx),bid.toInt64()) )
	{
	    pid.setObjectID( emobj->id() );
	    pid.setSectionID( emobj->sectionID(idx) );
	    pid.setSubID( bid.toInt64() );
	    return true;
	}
    }

    return false;
}


void HorizonFlatViewEditor2D::movementEndCB( CallBacker* )
{}


void HorizonFlatViewEditor2D::removePosCB( CallBacker* )
{
    if ( pointselections_.isEmpty() )
	return;

    RefMan<EM::EMObject> emobj = EM::EMM().getObject( emid_ );
    if ( !emobj ) return;

    mDynamicCastGet( EM::Horizon2D*, hor2d,emobj.ptr() );
    if ( !hor2d ) return;

    Undo& doundo = EM::EMM().undo(emobj->id());
    const int lastid = doundo.currentEventID();

    hor2d->setBurstAlert( true );
    for ( int idx=0; idx<pointselections_.size(); idx++ )
	emobj->unSetPos(pointselections_[idx], true );

    hor2d->setBurstAlert( false );

    if ( lastid!=doundo.currentEventID() )
	doundo.setUserInteractionEnd( doundo.currentEventID() );

    horpainter_->removeSelections();
}


void HorizonFlatViewEditor2D::polygonFinishedCB( CallBacker* )
{
    pointselections_.setEmpty();

    TypeSet<int> selectedids;
    TypeSet<int> selectedidxs;
    editor_->getPointSelections( selectedids, selectedidxs );

    if ( !selectedids.size() && horpainter_ )
    {
	horpainter_->removeSelections();
	return;
    }

    RefMan<EM::EMObject> emobj = EM::EMM().getObject( emid_ );
    if ( !emobj ) return;

    mDynamicCastGet( EM::Horizon2D*, hor2d, emobj.ptr() );
    if ( !hor2d ) return;

    BinID bid;

    for ( int ids=0; ids<selectedids.size(); ids++ )
    {
	const FlatView::AuxData* auxdata = getAuxData(selectedids[ids]);
	if ( !auxdata || !auxdata->poly_.validIdx(selectedidxs[ids]) )
	    continue;

	const int posidx = horpainter_->getDistances().indexOf(
			mCast(float,auxdata->poly_[selectedidxs[ids]].x) );
	const TypeSet<int>& trcnrs = horpainter_->getTrcNos();
	if ( !trcnrs.validIdx(posidx) )
	    continue;

	bid.inl() = hor2d->geometry().lineIndex( geomid_ );
	bid.crl() = trcnrs[posidx];

	EM::PosID posid( emid_, getSectionID(selectedids[ids]), bid.toInt64() );
	pointselections_ += posid;
    }

    if ( pointselections_.size()>0 )
	horpainter_->displaySelections( pointselections_ );

    editor_->setSelectionPolygonVisible( false );
}

} // namespace MPE
