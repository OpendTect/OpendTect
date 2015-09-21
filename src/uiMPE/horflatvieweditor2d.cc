/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		May 2010
 RCS:		$Id$
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
{
    curcs_.setEmpty();
    horpainter_->abouttorepaint_.notify(
	    mCB(this,HorizonFlatViewEditor2D,horRepaintATSCB) );
    horpainter_->repaintdone_.notify(
	    mCB(this,HorizonFlatViewEditor2D,horRepaintedCB) );
}


HorizonFlatViewEditor2D::~HorizonFlatViewEditor2D()
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
//	setMouseEventHandler( 0 );
    cleanAuxInfoContainer();
    delete horpainter_;
    deepErase( markeridinfos_ );
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


void HorizonFlatViewEditor2D::paint()
{
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

	if ( MPE::engine().getTrackerByObject(emid_) != -1 )
	{
	    int trackeridx = MPE::engine().getTrackerByObject( emid_ );

	    if ( MPE::engine().getTracker(trackeridx) )
		MPE::engine().setActiveTracker( emid_ );
	}
	else
	    MPE::engine().setActiveTracker( -1 );
    }

    for ( int idx=0; idx<markeridinfos_.size(); idx++ )
	editor_->enablePolySel( markeridinfos_[idx]->markerid_, mehandler_ );
}


void HorizonFlatViewEditor2D::setSeedPicking( bool yn )
{ seedpickingon_ = yn; }


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
	if ( !tracker || !tracker->is2D() || tracker->objectID() != emid_ )
	    return;

	MPE::EMSeedPicker* seedpicker = tracker->getSeedPicker(true);
	if ( !seedpicker )
	    return;

	const TrcKeyValue tkv( SI().transform(coord), (float)coord.z );
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
    if ( editor_ && editor_->sower().accept(mehandler_->event(), false) )
	return;

    if ( curcs_.isEmpty() || !editor_->viewer().appearance().annot_.editable_
			  || editor_->isSelActive() )
	return;

    //if ( !seedpickingon_ ) return;

    MPE::EMTracker* tracker = MPE::engine().getActiveTracker();
    if ( !tracker || tracker->objectID()!=emid_  || !tracker->is2D() )
	return;

    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( !emobj ) return;

    MPE::EMSeedPicker* seedpicker = tracker->getSeedPicker(true);
    if ( !seedpicker )
	return;

    seedpicker->setSectionID( emobj->sectionID(0) );
    bool pickinvd = true;
    if ( !checkSanity(*tracker,*seedpicker,pickinvd) )
	return;

    const MouseEvent& mouseevent = mehandler_->event();
    const Geom::Point2D<int>& mousepos = mouseevent.pos();
    const Geom::Point2D<double>* markerpos = editor_->markerPosAt( mousepos );
    const bool ctrlorshifclicked =
	mouseevent.shiftStatus() || mouseevent.ctrlStatus();
    if ( seedpicker->getTrackMode()==EMSeedPicker::DrawBetweenSeeds &&
	 markerpos && !ctrlorshifclicked )
    {
	mDynamicCastGet(const uiFlatViewer*,vwr,&editor_->viewer());
	if ( !vwr || !editor_->getMouseArea().isInside(mousepos) )
	    return;

	const uiWorldPoint wp =
	    markerpos ? *markerpos : vwr->getWorld2Ui().transform( mousepos );
	pickedpos_ = SI().transform( vwr->getCoord(wp) );
	return;
    }

    const Color& prefcol = emobj->preferredColor();

    if ( editor_ )
    {
	editor_->sower().reInitSettings();
	editor_->sower().intersow();
	editor_->sower().reverseSowingOrder();
	if ( editor_->sower().activate(prefcol, mouseevent) )
	    return;
    }
}


void HorizonFlatViewEditor2D::doubleClickedCB( CallBacker* )
{
    handleMouseClicked( true );
}


void HorizonFlatViewEditor2D::mouseReleaseCB( CallBacker* )
{
    handleMouseClicked( false );
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
    if ( !tracker || tracker->objectID()!=emid_ || !tracker->is2D() )
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

    const Geom::Point2D<int>& mousepos = mouseevent.pos();
    mDynamicCastGet(const uiFlatViewer*,vwr,&editor_->viewer());
    if ( !vwr || !editor_->getMouseArea().isInside(mousepos) )
	return;

    bool pickinvd = true;
    ConstDataPackRef<FlatDataPack> dp = vwr->obtainPack( !pickinvd );
    if ( !dp || !prepareTracking(pickinvd,*tracker,*seedpicker,*dp) )
	return;

    const int prevevent = EM::EMM().undo().currentEventID();
    MouseCursorManager::setOverride( MouseCursor::Wait );
    if ( !emobj->hasBurstAlert() )
	emobj->setBurstAlert( true );

    const Geom::Point2D<double>* markerpos = editor_->markerPosAt( mousepos );
    const uiWorldPoint wp = markerpos ? *markerpos :
			vwr->getWorld2Ui().transform(mousepos);
    doTheSeed( *seedpicker, vwr->getCoord(wp), mouseevent );

    if ( !editor_->sower().moreToSow() && emobj->hasBurstAlert() )
	emobj->setBurstAlert( false );

    if ( dbl && seedpicker->getTrackMode()==EMSeedPicker::DrawBetweenSeeds )
	dodropnext_ = true;

    MouseCursorManager::restoreOverride();
    const int currentevent = EM::EMM().undo().currentEventID();
    if ( !dbl && currentevent != prevevent )
    {
	if ( !editor_ || !editor_->sower().moreToSow() )
	    EM::EMM().undo().setUserInteractionEnd(currentevent);
    }
}


static bool selectSeedData(
		const FlatView::AuxDataEditor* editor, bool& pickinvd )
{
    if ( !editor )
	return false;

    const bool vdvisible = editor->viewer().isVisible(false);
    const bool wvavisible = editor->viewer().isVisible(true);

    if ( vdvisible && wvavisible )
	pickinvd = uiMSG().question( "Which one is your seed data?",
				     "VD", "Wiggle" );
    else if ( vdvisible )
	pickinvd = true;
    else if ( wvavisible )
	pickinvd = false;
    else
    {
	uiMSG().error( "No data to choose from" );
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

    const Attrib::SelSpec* atsel = 0;

    const MPE::SectionTracker* sectiontracker =
	tracker.getSectionTracker(emobj->sectionID(0), true);

    const Attrib::SelSpec* trackedatsel = sectiontracker
			? sectiontracker->adjuster()->getAttributeSel(0)
			: 0;

    Attrib::SelSpec newatsel;
    if ( trackedatsel )
	newatsel = *trackedatsel;

    if ( spk.nrSeeds() < 1 )
    {
	if ( !selectSeedData(editor_,pickinvd) )
	    return false;

	atsel = pickinvd ? vdselspec_ : wvaselspec_;

	if ( !trackersetupactive_ && atsel && trackedatsel &&
	     (newatsel!=*atsel) &&
	     (spk.getTrackMode()!=spk.DrawBetweenSeeds) )
	{
	    uiMSG().error( tr("Saved setup has different attribute. \n"
			      "Either change setup attribute or change\n"
			      "display attribute you want to track on") );
	    return false;
	}
    }
    else
    {
	if ( vdselspec_ && trackedatsel && (newatsel==*vdselspec_) )
	    pickinvd = true;
	else if ( wvaselspec_ && trackedatsel && (newatsel==*wvaselspec_) )
	    pickinvd = false;
	else if ( spk.getTrackMode() !=spk.DrawBetweenSeeds )
	{
	    uiString warnmsg = tr("Setup suggests tracking is done on '%1.\n"
				  "But what you see is: '%2'.\n"
				  "To continue seed picking either "
				  "change displayed attribute or\n"
				  "change input data in Tracking Setup.")
			     .arg(newatsel.userRef());
	    if (vdselspec_ && pickinvd)
		warnmsg.arg(vdselspec_->userRef());
	    else if (wvaselspec_ && !pickinvd)
		warnmsg.arg(wvaselspec_->userRef());

	    uiMSG().error(warnmsg);
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
    mDynamicCastGet( MPE::Horizon2DSeedPicker*, h2dsp, &seedpicker );
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
					 const MouseEvent& mev ) const
{
    const TrcKeyValue tkv( SI().transform(crd), (float)crd.z );
    const bool ismarker = editor_->markerPosAt( mev.pos() );

    if ( !ismarker )
    {
	bool drop = ismarker ? false : mev.shiftStatus();
	if ( dodropnext_ )
	{
	    drop = dodropnext_;
	    dodropnext_ = false;
	}

	const TrcKeyValue tkv2( SI().transform(Coord(mev.x(),mev.y())), 0.f );
	if ( spk.addSeed(tkv,drop,tkv2) )
	    return true;
    }
    else if ( mev.shiftStatus() || mev.ctrlStatus() )
    {
	const bool ctrlshifclicked = mev.shiftStatus() && mev.ctrlStatus();
	if ( spk.removeSeed(tkv.tk_,true,!ctrlshifclicked) )
	    return false;
    }

    return true;
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
    TypeSet<int> selectedids;
    TypeSet<int> selectedidxs;
    editor_->getPointSelections( selectedids, selectedidxs );

    if ( !selectedids.size() ) return;

    RefMan<EM::EMObject> emobj = EM::EMM().getObject( emid_ );
    if ( !emobj ) return;

    mDynamicCastGet(EM::Horizon2D*,hor2d,emobj.ptr());
    if ( !hor2d ) return;

    hor2d->setBurstAlert( true );

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
	emobj->unSetPos( posid, false );
    }

    hor2d->setBurstAlert( false );
}

}//namespace MPE
