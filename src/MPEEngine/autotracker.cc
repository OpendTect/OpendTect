/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "autotracker.h"

#include "emhorizon3d.h"
#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "emtracker.h"
#include "emundo.h"
#include "horizonadjuster.h"
#include "mpeengine.h"
#include "sectionadjuster.h"
#include "sectionextender.h"
#include "sectiontracker.h"
#include "thread.h"
#include "threadwork.h"
#include "trckeyvalue.h"

namespace MPE
{


class TrackerTask : public Task
{
public:
TrackerTask( HorizonTrackerMgr& mgr, const TrcKeyValue& seed,
		const TrcKeyValue& srcpos, const int seedid )
    : mgr_(mgr)
    , seed_(seed)
    , srcpos_(srcpos)
    , seedid_( seedid )
{
}


~TrackerTask()
{
}


bool execute() override
{
    sectiontracker_ = mgr_.getFreeSectionTracker();
    const bool isok = sectiontracker_ && sectiontracker_->extender() &&
			sectiontracker_->adjuster();
    if ( !isok )
	return false;

    sectiontracker_->reset();
    SectionExtender* ext = sectiontracker_->extender();
    SectionAdjuster* adj = sectiontracker_->adjuster();

    ext->setDirection( TrcKeyValue(BinID::noStepout()) );
    ext->setStartPosition( srcpos_.tk_ );
    int res;
    while ( (res=ext->nextStep())>0 )
	;

    TypeSet<TrcKey> addedpos = ext->getAddedPositions();
    TypeSet<TrcKey> addedpossrc = ext->getAddedPositionsSource();
    adj->setSeedPosition( seed_.tk_ );
    adj->setSeedId( seedid_ );
    adj->setPositions( addedpos, &addedpossrc );
    while ( (res=adj->nextStep())>0 )
	;

    mgr_.freeSectionTracker( sectiontracker_ );
    RefMan<EM::EMObject> emobject = sectiontracker_->emObject();
    mDynamicCastGet(EM::Horizon3D*,hor3d,emobject.ptr())
    for ( const auto& src : addedpos )
    {
	if ( hor3d )
	    hor3d->auxdata.setAuxDataVal( 3, src, (float)mgr_.tasknr_ );

	mgr_.addTask( seed_, src, seedid_ );
    }

    return true;
}

    HorizonTrackerMgr&	mgr_;
    SectionTracker*	sectiontracker_ = nullptr;

    TrcKeyValue		seed_;
    TrcKeyValue		srcpos_;
    const int		seedid_;

};



HorizonTrackerMgr::HorizonTrackerMgr( EMTracker& emt )
    : twm_(Threads::WorkManager::twm())
    , tracker_(&emt)
    , finished(this)
{
    queueid_ = twm_.addQueue(
		    Threads::WorkManager::MultiThread, "Horizon Tracker" );
}


HorizonTrackerMgr::~HorizonTrackerMgr()
{
    twm_.removeQueue( queueid_, false );
    if ( flatcubes_ )
	deepErase( *flatcubes_ );

    delete flatcubes_;
    deepErase( sectiontrackers_ );
}


ConstRefMan<EMTracker> HorizonTrackerMgr::getTracker() const
{
    return tracker_.get();
}


RefMan<EMTracker> HorizonTrackerMgr::getTracker()
{
    return tracker_.get();
}


void HorizonTrackerMgr::init()
{
    if ( flatcubes_ )
	deepErase( *flatcubes_ );

    delete flatcubes_;
    flatcubes_ = new ObjectSet<FlatCubeInfo>;
}


void HorizonTrackerMgr::stop()
{
    twm_.removeQueue( queueid_, false );
    nrtodo_ = 0;
    addUndoEvent();
    queueid_ = twm_.addQueue(
	Threads::WorkManager::MultiThread, "Horizon Tracker" );
}


bool HorizonTrackerMgr::hasTasks() const
{
    return nrtodo_ > 0;
}


void HorizonTrackerMgr::addTask( const TrcKeyValue& seed,
				 const TrcKeyValue& source, int seedid )
{
    RefMan<EMTracker> tracker = getTracker();
    RefMan<EM::EMObject> emobject = tracker ? tracker->emObject() : nullptr;
    mDynamicCastGet(EM::Horizon*,hor,emobject.ptr())
    if ( !hor || !hor->hasZ(source.tk_) )
	return;

    Threads::Locker locker( addlock_ );
    nrtodo_++;
    tasknr_++;
    locker.unlockNow();

    CallBack cb( mCB(this,HorizonTrackerMgr,taskFinished) );
    Task* task = new TrackerTask( *this, seed, source, seedid );
    twm_.addWork( Threads::Work(*task,true), &cb, queueid_,
		  false, false, true );
}


void HorizonTrackerMgr::taskFinished( CallBacker* )
{
    Threads::Locker locker( addlock_ );
    nrtodo_--;
    nrdone_++;

    RefMan<EMTracker> tracker = getTracker();
    RefMan<EM::EMObject> emobject = tracker ? tracker->emObject() : nullptr;
    mDynamicCastGet(EM::Horizon3D*,hor3d,emobject.ptr())
    if ( nrdone_%500 == 0 || nrtodo_==0 )
    {
	if ( hor3d )
	    hor3d->geometryElement()->blockCallBacks(true);
    }

    if ( nrtodo_ == 0 )
    {
	if ( hor3d )
	    hor3d->setBurstAlert( false );

	addUndoEvent();
	deepErase( sectiontrackers_ );
	finished.trigger();
    }
}


SectionTracker* HorizonTrackerMgr::getFreeSectionTracker()
{
    Threads::Locker locker( getfreestlock_  );
    for ( int idx=0; idx<sectiontrackers_.size(); idx++ )
    {
	if ( trackerinuse_[idx] )
	    continue;

	trackerinuse_[idx] = true;
	return sectiontrackers_[idx];
    }

    return 0;
}


void HorizonTrackerMgr::freeSectionTracker( const SectionTracker* st )
{
    Threads::Locker locker( getfreestlock_  );
    const int stidx = sectiontrackers_.indexOf( st );
    trackerinuse_[stidx] = false;
}


void HorizonTrackerMgr::startFromSeeds( const TypeSet<TrcKey>& seeds )
{
    seeds_ = seeds;
    RefMan<EMTracker> tracker = getTracker();
    RefMan<EM::EMObject> emobject = tracker ? tracker->emObject() : nullptr;
    if ( !emobject )
	return;

    SectionTracker* st = tracker->getSectionTracker();
    if ( !st || !st->extender() )
	return;

    st->extender()->preallocExtArea();
    mDynamicCastGet(EM::Horizon3D*,hor3d,emobject.ptr())
    if ( hor3d )
    {
	if ( nrdone_==0 )
	{
	    hor3d->initTrackingAuxData();
	    hor3d->initTrackingArrays();
	}

	hor3d->updateTrackingSampling();
	horizon3dundoinfo_ = hor3d->createArray2D();
	horizon3dundoorigin_ = hor3d->range().start_;
    }

    deepErase( sectiontrackers_ );
    trackerinuse_.erase();
    for ( int idx=0; idx<twm_.nrThreads(); idx++ )
    {
	SectionTracker* cst = tracker->cloneSectionTracker();
	cst->extender()->setUndo( false );
	cst->adjuster()->setUndo( false );
	sectiontrackers_ += cst;
	trackerinuse_ += false;
    }

    nrtodo_ = 0;
    hor3d->setBurstAlert( true );
    for ( int idx=0; idx<seeds_.size(); idx++ )
	addTask( seeds_[idx], seeds_[idx], idx+1 );
}


void HorizonTrackerMgr::addUndoEvent()
{
    if ( !horizon3dundoinfo_ )
	return;

    RefMan<EMTracker> tracker = getTracker();
    RefMan<EM::EMObject> emobject = tracker ? tracker->emObject() : nullptr;
    mDynamicCastGet(EM::Horizon3D*,hor3d,emobject.ptr())
    UndoEvent* undo = new EM::SetAllHor3DPosUndoEvent(
			    hor3d, horizon3dundoinfo_, horizon3dundoorigin_ );
    if ( emobject )
	EM::EMM().undo(emobject->id()).addEvent( undo, "Auto tracking" );

    horizon3dundoinfo_ = nullptr;
}


void HorizonTrackerMgr::updateFlatCubesContainer( const TrcKeyZSampling& cs,
						  bool addremove )
{
    if ( (cs.nrInl() != 1 && cs.nrCrl() !=1) || !flatcubes_ )
	return;

    ObjectSet<FlatCubeInfo>& flatcubes = *flatcubes_;
    int idxinquestion = -1;
    for ( int flatcsidx=0; flatcsidx<flatcubes.size(); flatcsidx++ )
    {
	if ( flatcubes[flatcsidx]->flatcs_.defaultDir() == cs.defaultDir() )
	{
	    if ( flatcubes[flatcsidx]->flatcs_.nrInl() == 1 )
	    {
		if ( flatcubes[flatcsidx]->flatcs_.hsamp_.start_.inl() ==
			cs.hsamp_.start_.inl() )
		{
		    idxinquestion = flatcsidx;
		    break;
		}
	    }
	    else if ( flatcubes[flatcsidx]->flatcs_.nrCrl() == 1 )
	    {
		if ( flatcubes[flatcsidx]->flatcs_.hsamp_.start_.crl() ==
		     cs.hsamp_.start_.crl() )
		{
		    idxinquestion = flatcsidx;
		    break;
		}
	    }
	}
    }

    if ( addremove )
    {
	if ( idxinquestion == -1 )
	{
	    auto* flatcsinfo = new FlatCubeInfo();
	    flatcsinfo->flatcs_.include( cs );
	    flatcubes.add( flatcsinfo );
	}
	else
	{
	    flatcubes[idxinquestion]->flatcs_.include( cs );
	    flatcubes[idxinquestion]->nrseeds_++;
	}
    }
    else
    {
	if ( idxinquestion == -1 )
	    return;

	flatcubes[idxinquestion]->nrseeds_--;
	if ( flatcubes[idxinquestion]->nrseeds_ == 0 )
	    flatcubes.removeSingle( idxinquestion );
    }
}

} // namespace MPE
