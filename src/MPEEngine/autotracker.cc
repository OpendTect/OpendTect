/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/


#include "autotracker.h"

#include "arraynd.h"
#include "binidvalue.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emtracker.h"
#include "emundo.h"
#include "emsurfaceauxdata.h"
#include "horizonadjuster.h"
#include "mpeengine.h"
#include "progressmeter.h"
#include "sectionadjuster.h"
#include "sectionextender.h"
#include "sectiontracker.h"
#include "survinfo.h"
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
    , sectiontracker_(0)
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

    ext->setDirection( TrcKeyValue(TrcKey(0,0)) );
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

    for ( int idx=0; idx<addedpos.size(); idx++ )
    {
	const TrcKey src = addedpos[idx];
	mDynamicCastGet(EM::Horizon3D*,hor3d,&sectiontracker_->emObject())
	if ( hor3d )
	    hor3d->auxdata.setAuxDataVal( 3, src, (float)mgr_.tasknr_ );
	mgr_.addTask( seed_, src, seedid_ );
    }

    return true;
}

    HorizonTrackerMgr&	mgr_;
    SectionTracker*	sectiontracker_;

    TrcKeyValue		seed_;
    TrcKeyValue		srcpos_;
    const int		seedid_;

};



HorizonTrackerMgr::HorizonTrackerMgr( EMTracker& emt )
    : twm_(Threads::WorkManager::twm())
    , tracker_(emt)
    , finished(this)
    , nrdone_(0)
    , nrtodo_(0)
    , tasknr_(0)
    , horizon3dundoinfo_(0)
{
    queueid_ = twm_.addQueue(
	Threads::WorkManager::MultiThread, "Horizon Tracker" );
}


HorizonTrackerMgr::~HorizonTrackerMgr()
{
    twm_.removeQueue( queueid_, false );
    deepErase( sectiontrackers_ );
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
{ return nrtodo_ > 0; }


void HorizonTrackerMgr::setSeeds( const TypeSet<TrcKey>& seeds )
{ seeds_ = seeds; }


void HorizonTrackerMgr::addTask( const TrcKeyValue& seed,
				 const TrcKeyValue& source, int seedid )
{
    mDynamicCastGet(EM::Horizon*,hor,tracker_.emObject())
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

    mDynamicCastGet(EM::Horizon3D*,hor3d,tracker_.emObject())
    if ( nrdone_%500 == 0 || nrtodo_==0 )
    {
	if ( hor3d )
	    hor3d->sectionGeometry( hor3d->sectionID(0) )->blockCallBacks(true);
    }

    if ( nrtodo_ == 0 )
    {
	if ( hor3d ) hor3d->setBurstAlert( false );
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


void HorizonTrackerMgr::startFromSeeds()
{
    EM::EMObject* emobj = tracker_.emObject();
    if ( !emobj ) return;

    SectionTracker* st = tracker_.getSectionTracker( emobj->sectionID(0) );
    if ( !st || !st->extender() )
	return;

    st->extender()->preallocExtArea();
    mDynamicCastGet(EM::Horizon3D*,hor3d,emobj)
    if ( hor3d )
    {
	if ( nrdone_==0 )
	{
	    hor3d->initTrackingAuxData();
	    hor3d->initTrackingArrays();
	}

	hor3d->updateTrackingSampling();
	horizon3dundoinfo_ = hor3d->createArray2D( hor3d->sectionID(0) );
	horizon3dundoorigin_ = hor3d->range().start_;
    }

    deepErase( sectiontrackers_ );
    trackerinuse_.erase();
    for ( int idx=0; idx<twm_.nrThreads(); idx++ )
    {
	SectionTracker* cst = tracker_.cloneSectionTracker();
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
    if ( horizon3dundoinfo_ )
    {
	EM::EMObject* emobj = tracker_.emObject();
	mDynamicCastGet(EM::Horizon3D*,hor3d,emobj)
	UndoEvent* undo = new EM::SetAllHor3DPosUndoEvent(
		hor3d, hor3d->sectionID(0),
		horizon3dundoinfo_, horizon3dundoorigin_ );
	EM::EMM().undo(emobj->id()).addEvent( undo, "Auto tracking" );
	horizon3dundoinfo_ = 0;
    }
}

} // namespace MPE
