/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		May 2010
 RCS:		$Id: horflatvieweditor3d.cc,v 1.11 2012/07/10 13:06:07 cvskris Exp $
________________________________________________________________________

-*/

#include "horflatvieweditor3d.h"

#include "emhorizonpainter3d.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emseedpicker.h"
#include "emtracker.h"
#include "flatauxdataeditor.h"
#include "flatposdata.h"
#include "mouseevent.h"
#include "mousecursor.h"
#include "mpeengine.h"
#include "sectionadjuster.h"
#include "sectiontracker.h"
#include "survinfo.h"
#include "undo.h"

#include "uimsg.h"
#include "uiworld2ui.h"

namespace MPE
{

HorizonFlatViewEditor3D::HorizonFlatViewEditor3D( FlatView::AuxDataEditor* ed,
						  const EM::ObjectID& emid )
    : editor_(ed)
    , emid_(emid)
    , horpainter_( new EM::HorizonPainter3D(ed->viewer(),emid) )
    , mehandler_(0)
    , vdselspec_(0)
    , wvaselspec_(0)
    , seedpickingon_(false)
    , trackersetupactive_(false)
    , updseedpkingstatus_(this)
{
    curcs_.setEmpty();
    horpainter_->abouttorepaint_.notify(
	    mCB(this,HorizonFlatViewEditor3D,horRepaintATSCB) );
    horpainter_->repaintdone_.notify(
	    mCB(this,HorizonFlatViewEditor3D,horRepaintedCB) );
}


HorizonFlatViewEditor3D::~HorizonFlatViewEditor3D()
{
    if ( mehandler_ )
    {
	editor_->removeSelected.remove(
		 mCB(this,HorizonFlatViewEditor3D,removePosCB) );
	mehandler_->movement.remove(
		mCB(this,HorizonFlatViewEditor3D,mouseMoveCB) );
	mehandler_->buttonPressed.remove(
		mCB(this,HorizonFlatViewEditor3D,mousePressCB) );
	mehandler_->buttonReleased.remove(
		mCB(this,HorizonFlatViewEditor3D,mouseReleaseCB) );
    }
//	setMouseEventHandler( 0 );
    cleanAuxInfoContainer();
    delete horpainter_;
    deepErase( markeridinfos_ );
}


void HorizonFlatViewEditor3D::setCubeSampling( const CubeSampling& cs )
{
    curcs_ = cs;
    horpainter_->setCubeSampling( cs );
}


void HorizonFlatViewEditor3D::setPath( const TypeSet<BinID>* path )
{
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
    if ( mehandler_ )
    {
	editor_->removeSelected.remove(
		mCB(this,HorizonFlatViewEditor3D,removePosCB) );
	mehandler_->movement.remove(
		mCB(this,HorizonFlatViewEditor3D,mouseMoveCB) );
	mehandler_->buttonPressed.remove(
		mCB(this,HorizonFlatViewEditor3D,mousePressCB) );
	mehandler_->buttonReleased.remove(
		mCB(this,HorizonFlatViewEditor3D,mouseReleaseCB) );
    }

    mehandler_ = meh;

    if ( mehandler_ )
    {
	editor_->removeSelected.notify(
		mCB(this,HorizonFlatViewEditor3D,removePosCB) );
	mehandler_->movement.notify(
		mCB(this,HorizonFlatViewEditor3D,mouseMoveCB) );
	mehandler_->buttonPressed.notify(
		mCB(this,HorizonFlatViewEditor3D,mousePressCB) );
	mehandler_->buttonReleased.notify(
		mCB(this,HorizonFlatViewEditor3D,mouseReleaseCB) );

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
	editor_->enablePolySel( markeridinfos_[idx]->merkerid_, mehandler_ );
}


void HorizonFlatViewEditor3D::enableLine( bool yn )
{
    horpainter_->enableLine( yn );
}


void HorizonFlatViewEditor3D::enableSeed( bool yn )
{
    horpainter_->enableSeed( yn );
}


void HorizonFlatViewEditor3D::paint()
{
    horpainter_->paint();
    fillAuxInfoContainer();
}


void HorizonFlatViewEditor3D::setSeedPicking( bool yn )
{ seedpickingon_ = yn; }


void HorizonFlatViewEditor3D::mouseMoveCB( CallBacker* )
{
    const MouseEvent& mouseevent = mehandler_->event();
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


void HorizonFlatViewEditor3D::mousePressCB( CallBacker* )
{
    if ( editor_ && editor_->sower().accept(mehandler_->event(), false) )
	return;

    if ( curcs_.isEmpty() || !editor_->viewer().appearance().annot_.editable_
			  || editor_->isSelActive() )
	return;

    //if ( !seedpickingon_ ) return;

    MPE::EMTracker* tracker = MPE::engine().getActiveTracker();
    if ( !tracker || tracker->objectID() != emid_ || tracker->is2D() )
	return;

    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( !emobj ) return;

    MPE::EMSeedPicker* seedpicker = tracker->getSeedPicker(true);

    if ( !seedpicker || !seedpicker->canAddSeed() ||
	 !seedpicker->canSetSectionID() ||
	 !seedpicker->setSectionID(emobj->sectionID(0)) )
	return;

    bool pickinvd = true;
    if ( !checkSanity(*tracker,*seedpicker,pickinvd) )
	return;

    const MouseEvent& mouseevent = mehandler_->event();
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


void HorizonFlatViewEditor3D::mouseReleaseCB( CallBacker* )
{
    if ( curcs_.isEmpty() || !editor_->viewer().appearance().annot_.editable_
	    || editor_->isSelActive() )
	return;

    MPE::EMTracker* tracker = MPE::engine().getActiveTracker();
    if ( !tracker ) return;

    if ( tracker->objectID() != emid_ ) return;

    if ( tracker->is2D() ) return;

    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( !emobj ) return;

    MPE::EMSeedPicker* seedpicker = tracker->getSeedPicker(true);

    if ( !seedpicker || !seedpicker->canAddSeed() ) return;

    if ( !seedpicker->canSetSectionID() ||
	 !seedpicker->setSectionID(emobj->sectionID(0)) )
	return;

    const MouseEvent& mouseevent = mehandler_->event();

    if ( editor_ )
    {
	const bool sequentsowing = editor_->sower().mode() ==
				   FlatView::Sower::SequentSowing;
	seedpicker->setSowerMode( sequentsowing );
	if ( editor_->sower().accept(mouseevent,true) )
	    return;
    }

    //if ( !seedpickingon_ ) return;

    bool pickinvd = true;

    if ( !checkSanity(*tracker,*seedpicker,pickinvd) )
	return;

    const FlatDataPack* dp = editor_->viewer().pack( !pickinvd );
    if ( !dp ) return;

    const uiRect datarect( editor_->getMouseArea() );
    if ( !datarect.isInside(mouseevent.pos()) ) return;

    const Geom::Point2D<int> mousepos = mouseevent.pos();
    const Geom::Point2D<double>* markerpos = editor_->markerPosAt( mousepos );

    const uiWorld2Ui w2u( datarect.size(), editor_->getWorldRect(mUdf(int)) );
    const uiWorldPoint wp = markerpos ? *markerpos :
				w2u.transform( mousepos-datarect.topLeft() );

    const FlatPosData& pd = dp->posData();
    const IndexInfo ix = pd.indexInfo( true, wp.x );
    const IndexInfo iy = pd.indexInfo( false, wp.y );
    Coord3 clickedcrd = dp->getCoord( ix.nearest_, iy.nearest_ );
    clickedcrd.z = wp.y;

    if ( !prepareTracking(pickinvd,*tracker,*seedpicker,*dp) )
	return;

    const int prevevent = EM::EMM().undo().currentEventID();
    MouseCursorManager::setOverride( MouseCursor::Wait );
    emobj->setBurstAlert( true );

    const int trackerid = MPE::engine().getTrackerByObject( emid_ );
    
    bool action = doTheSeed( *seedpicker, clickedcrd, mouseevent );

    engine().updateFlatCubesContainer( curcs_, trackerid, action );
    
    emobj->setBurstAlert( false );
    MouseCursorManager::restoreOverride();

    const int currentevent = EM::EMM().undo().currentEventID();
    if ( currentevent != prevevent )
    {
	if ( !editor_ || !editor_->sower().moreToSow() )
	    EM::EMM().undo().setUserInteractionEnd(currentevent);
    }
}


bool HorizonFlatViewEditor3D::checkSanity( EMTracker& tracker,
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
	if ( editor_->viewer().pack(false) && editor_->viewer().pack(true) )
	{
	    if ( !uiMSG().question("Which one is your seed data.",
				   "VD", "Wiggle") )
		pickinvd = false;
	}
	else if ( editor_->viewer().pack(false) )
	    pickinvd = true;
	else if ( editor_->viewer().pack(true) )
	    pickinvd = false;
	else
	{
	    uiMSG().error( "No data to choose from" );
	    return false;
	}

	atsel = pickinvd ? vdselspec_ : wvaselspec_;

	if ( !trackersetupactive_ && atsel && trackedatsel &&
	     (newatsel!=*atsel) &&
	     (spk.getSeedConnectMode()!=spk.DrawBetweenSeeds) )
	{
	    uiMSG().error( "Saved setup has different attribute. \n"
		    	   "Either change setup attribute or change\n"
			   "display attribute you want to track on" );
	    return false;
	}
    }
    else
    {
	if ( vdselspec_ && trackedatsel && (newatsel==*vdselspec_) )
	    pickinvd = true;
	else if ( wvaselspec_ && trackedatsel && (newatsel==*wvaselspec_) )
	    pickinvd = false;
	else if ( spk.getSeedConnectMode() !=spk.DrawBetweenSeeds )
	{
	    BufferString warnmsg( "Setup suggests tracking is done on '" );
	    warnmsg.add( newatsel.userRef() ).add( "'.\n" )
		   .add(  "But what you see is: '" );
	    if ( vdselspec_ && pickinvd )
		warnmsg += vdselspec_->userRef();
	    else if ( wvaselspec_ && !pickinvd )
		warnmsg += wvaselspec_->userRef();
	    warnmsg.add( "'.\n" )
		   .add( "To continue seed picking either " )
		   .add( "change displayed attribute or\n" )
		   .add( "change input data in Tracking Setup." );

	    uiMSG().error( warnmsg.buf() );
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
    const Attrib::SelSpec* as = 0;
    as = picinvd ? vdselspec_ : wvaselspec_;

    if ( !seedpicker.startSeedPick() )
	return false;

    NotifyStopper notifystopper( MPE::engine().activevolumechange );
    MPE::engine().setActiveVolume( curcs_ );
    notifystopper.restore();

    seedpicker.setSelSpec( as );

    if ( !MPE::engine().cacheIncludes(*as,curcs_) )
	if ( dp.id() > DataPack::cNoID() )
	    MPE::engine().setAttribData( *as, dp.id() );

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


bool HorizonFlatViewEditor3D::doTheSeed( EMSeedPicker& spk, const Coord3& crd,
					 const MouseEvent& mev ) const
{
    EM::PosID pid;
    getPosID( crd, pid );

    const bool ctrlshiftclicked = mev.ctrlStatus() && mev.shiftStatus();
    const bool ismarker = editor_->markerPosAt( mev.pos() );

    if ( !ismarker || (ismarker && !mev.ctrlStatus() && !mev.shiftStatus()) )
    {
	const bool drop = ismarker ? false : ctrlshiftclicked;
	if ( spk.addSeed(crd, drop, Coord3(mev.x(),mev.y(),0)) )
	    return true;
    }
    else
    {
	bool env = false;
	bool retrack = false;

	if ( !ctrlshiftclicked )
	{
	    if ( mev.ctrlStatus() )
	    {
		env = true;
		retrack = true;
	    }
	    else if ( mev.shiftStatus() )
		env = true;
	}

	if ( spk.removeSeed(pid,env,retrack) )
	    return false;
    }

    return true;
}


void HorizonFlatViewEditor3D::cleanAuxInfoContainer()
{
    for ( int idx=0; idx<markeridinfos_.size(); idx++ )
	editor_->removeAuxData( markeridinfos_[idx]->merkerid_ );

    if ( markeridinfos_.size() )
	deepErase( markeridinfos_ );
}


void HorizonFlatViewEditor3D::fillAuxInfoContainer()
{
    ObjectSet<EM::HorizonPainter3D::Marker3D> disphormrkinfos;
    horpainter_->getDisplayedHor( disphormrkinfos );

    for ( int idx=0; idx<disphormrkinfos.size(); idx++ )
    {
	Hor3DMarkerIdInfo* markeridinfo = new Hor3DMarkerIdInfo;
	markeridinfo->merkerid_ = editor_->addAuxData(
					disphormrkinfos[idx]->marker_, true );
	markeridinfo->marker_ = disphormrkinfos[idx]->marker_;
	markeridinfo->sectionid_ = disphormrkinfos[idx]->sectionid_;
	editor_->enableEdit( markeridinfo->merkerid_, false, false, false );
	editor_->enablePolySel( markeridinfo->merkerid_, mehandler_ );

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


FlatView::Annotation::AuxData* HorizonFlatViewEditor3D::getAuxData( int markid )
{
    for ( int idx=0; idx<markeridinfos_.size(); idx++ )
    {
	if ( markeridinfos_[idx]->merkerid_ == markid )
	    return markeridinfos_[idx]->marker_;
    }

    return 0;
}


EM::SectionID HorizonFlatViewEditor3D::getSectionID( int markid )
{
    for ( int idx=0; idx<markeridinfos_.size(); idx++ )
    {
	if ( markeridinfos_[idx]->merkerid_ == markid )
	    return markeridinfos_[idx]->sectionid_;
    }

    return -1;
}


void HorizonFlatViewEditor3D::movementEndCB( CallBacker* )
{}


void HorizonFlatViewEditor3D::removePosCB( CallBacker* )
{
    TypeSet<int> selectedids;
    TypeSet<int> selectedidxs;
    editor_->getPointSelections( selectedids, selectedidxs );

    if ( !selectedids.size() ) return;

    RefMan<EM::EMObject> emobj = EM::EMM().getObject( emid_ );
    if ( !emobj ) return;

    mDynamicCastGet(EM::Horizon3D*,hor3d,emobj.ptr());
    if ( !hor3d ) return;

    hor3d->setBurstAlert( true );

    BinID bid;

    for ( int ids=0; ids<selectedids.size(); ids++ )
    {
	if ( curcs_.nrInl() == 1 )
	{
	    bid.inl = curcs_.hrg.start.inl;
	    bid.crl = 
		mNINT32(getAuxData(selectedids[ids])->poly_[selectedidxs[ids]].x);
	}
	else if ( curcs_.nrCrl() == 1 )
	{
	    bid.inl = 
		mNINT32(getAuxData(selectedids[ids])->poly_[selectedidxs[ids]].x);
	    bid.crl = curcs_.hrg.start.crl;
	}

	EM::PosID posid( emid_, getSectionID(selectedids[ids]), bid.toInt64() );
	emobj->unSetPos( posid, false );
    }

    hor3d->setBurstAlert( false );
}

} // namespace MPE
