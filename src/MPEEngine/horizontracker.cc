/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          March 2005
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "horizontracker.h"

#include "emfault.h"
#include "emhorizon3d.h"
#include "emhorizon2d.h"
#include "survinfo.h"
#include "arrayndimpl.h"
#include "seistrc.h"
#include "varlenarray.h"
#include "ptrman.h"
#include "sorting.h"

using namespace MPE;


#define mNo2D pErrMsg("2D not implemented yet");


class HorizonAutoTrackerTask : public ParallelTask
{
public:
    HorizonAutoTrackerTask( Array2D<HorizonAutoTracker::TrackTarget>& arr )
       : arr_( arr )
       , highestscore_( -1 )
       , bestidx_( -1 )
    {}
    
    float highestScore() const { return highestscore_; }
    int bestIdx() const { return bestidx_; }
    
    od_int64 nrIterations() const
    { return (od_int64) arr_.info().getTotalSz(); }
    
    bool doWork( od_int64, od_int64, int )
    {
	const int nrtargets = (int) nrIterations();
	float highestscore = -1;
	int bestidx = mUdf(int);
	
	HorizonAutoTracker::TrackTarget* targets = arr_.getData();
	
	while ( true )
	{
	    const int targetidx = curtarget_++;
	    if ( targetidx >= nrtargets )
		break;
		
	    HorizonAutoTracker::TrackTarget& target = targets[targetidx];
	    
	    if ( !target.istarget_ )
		continue;
	    
	    if ( target.needsrecalc_ )
		target.computeProposal();
	    
	    if ( target.isvalid_ && target.score_>highestscore )
	    {
		highestscore = target.score_;
		bestidx = targetidx;
	    }
	}
	
	Threads::SpinLockLocker lock( lock_ );
	if ( highestscore>highestscore_ )
	{
	    highestscore_ = highestscore;
	    bestidx_ = bestidx;
	}
	
	return true;
    }
    
protected:
    
    Threads::SpinLock				    lock_;
    float					    highestscore_;
    int						    bestidx_;
    
    Array2D<HorizonAutoTracker::TrackTarget>&	    arr_;
    Threads::Atomic<int>			    curtarget_;
    
};

    
HorizonAutoTracker::HorizonAutoTracker( EM::Horizon& hor )
    : horizon_( hor )
    , history_( 0 )
    , targetarray_( 0 )
    , trcs_( 0 )
    , tracewanted_( 0 )
    , diptrcs_( 0 )
    , hor3d_( 0 )
    , hor2d_( 0 )
    , inlcrlsystem_( 0 )
{
    mDynamicCast( EM::Horizon3D*, hor3d_, &horizon_ );
    mDynamicCast( EM::Horizon2D*, hor2d_, &horizon_ );
    
    if ( hor3d_ )
    {
	inlcrlsystem_ = SI().get3DGeometry( true );
    }    
}

HorizonAutoTracker::~HorizonAutoTracker()
{
    
}


bool HorizonAutoTracker::init()
{
    if ( !horizon_.isFullyLoaded() )
	return false;
    
    RowCol arraysz;
    
    if ( hor3d_ )
    {
	if ( !horizon_.nrSections() )
	{
	    return false;
	}
	
	const EM::SectionID sid = horizon_.sectionID( 0 );
	const StepInterval<int> inlrg = hor3d_->geometry().rowRange( sid );
	const StepInterval<int> crlrg = hor3d_->geometry().colRange( sid );
	
	
	const BinID step = hor3d_->geometry().step();
	
	arrayorigin_ = BinID ( inlrg.snap( inlcrlsystem_->inlRange().start ),
		     crlrg.snap( inlcrlsystem_->crlRange().start ) );
	
	
	if ( !inlcrlsystem_->inlRange().includes(arrayorigin_.row, false ) )
	    arrayorigin_.row += step.inl;
	
	if ( !inlcrlsystem_->crlRange().includes( arrayorigin_.col, false ) )
	    arrayorigin_.col += step.crl;
	
	BinID stop( inlrg.snap( inlcrlsystem_->inlRange().stop ),
		    crlrg.snap( inlcrlsystem_->crlRange().stop ) );
	
	if ( !inlcrlsystem_->inlRange().includes( stop.inl, false ) )
	    stop.inl -= step.inl;
	
	if ( !inlcrlsystem_->crlRange().includes( stop.crl, false ) )
	    stop.crl -= step.crl;

	arraysz = RowCol( ((stop.inl-arrayorigin_.row)/step.inl)+1,
			  ((stop.crl-arrayorigin_.col)/step.crl)+1 );
    }
    else
    {
	mNo2D;
	return false;
	
	arrayorigin_ = RowCol( 0, 0 );
	//Set arraysz
    }
    
#define mAllocArr( arr, clss, extra ) \
    arr = new Array2DImpl<clss>( arraysz.row, arraysz.col ); \
    if ( !arr.ptr() || !arr->isOK() ) \
    { \
	errmsg_ = "Cannot allocate " #arr; \
	return false; \
    } \
    extra;

    
    mAllocArr( targetarray_, TrackTarget,
	       targetarray_->setAll( TrackTarget(this) ) );
    mAllocArr( history_, unsigned char, history_->setAll( 0 ) );
    mAllocArr( trcs_, PtrMan<SeisTrc>, trcs_->setAll( 0 ) );
    mAllocArr( diptrcs_, PtrMan<SeisTrc>, diptrcs_->setAll( 0 ) );
    mAllocArr( tracewanted_, bool, tracewanted_->setAll( false ) );
    
    return true;
}
    
    

void HorizonAutoTracker::addFault( EM::Fault& fault )
{
    fault.ref();
    faults_ += &fault;
}
    
    
void HorizonAutoTracker::removeFault( EM::Fault& fault )
{
    const int idx = faults_.indexOf( &fault );
    
    if ( !faults_.validIdx( idx ) )
        return;
    
    faults_.remove( idx )->unRef();
}
    
    
    
int HorizonAutoTracker::doStep()
{
    HorizonAutoTrackerTask computetask( *targetarray_ );
    if ( !computetask.execute(true) )
	return ErrorOccurred();
    
    int targetidx = computetask.bestIdx();
    
    if ( targetidx==-1 )
	return Finished();
    
    TrackTarget& target = targetarray_->getData()[targetidx];
    
    horizon_.setPos(target.pid_, Coord3(0,0,target.proposedz_), true );
    history_->getData()[targetidx] = target.sources_;
    target.istarget_ = false; //we won't track this one again
    
    addSeed( target.pid_.sectionID(), target.pid_.subID() );
    
    return MoreToDo();
}
    
    
RowCol HorizonAutoTracker::getArrayPos( const EM::PosID& pos ) const
{
    if ( hor3d_ )
    {
	const BinID bid = pos.getRowCol();
	return (bid-arrayorigin_)/hor3d_->geometry().step();
    }
    
    pErrMsg("2D not implemented yet");
    return RowCol( mUdf(int), mUdf(int) );
}


#define mAddTarget( newrc ) \
{ \
   const int newtarget = addPossibleTarget( sectionid, newrc.toInt64() ); \
    if ( !mIsUdf(newtarget) ) \
	newtargets += newtarget; \
}

    
    
    
void HorizonAutoTracker::addSeed( EM::SectionID sectionid, EM::SubID subid )
{
    
    RowCol rc;
    rc.fromInt64( subid );
    TypeSet<int> newtargets;
    if ( hor2d_ )
    {
	mAddTarget( RowCol(rc.row, rc.col-step_.col ) );
	mAddTarget( RowCol(rc.row, rc.col+step_.col ) );
    }
    else
    {
	mAddTarget( RowCol(rc.row-step_.row, rc.col-step_.col ) );
	mAddTarget( RowCol(rc.row-step_.row, rc.col           ) );
	mAddTarget( RowCol(rc.row-step_.row, rc.col+step_.col ) );
	
	mAddTarget( RowCol(rc.row,rc.col-step_.col) );
	mAddTarget( RowCol(rc.row,rc.col+step_.col) );
	
	mAddTarget( RowCol(rc.row+step_.row, rc.col-step_.col ) );
	mAddTarget( RowCol(rc.row+step_.row, rc.col           ) );
	mAddTarget( RowCol(rc.row+step_.row, rc.col+step_.col ) );
	
    }
    
    if ( newtargets.isEmpty() )
	return;
}


HorizonAutoTracker::TrackTarget::TrackTarget( HorizonAutoTracker* tracker )
    : tracker_( tracker )
{
    reset();
}


unsigned char HorizonAutoTracker::TrackTarget::getSourceMask(EM::SubID from,
							     EM::SubID to)
{
    const RowCol diff = RowCol::fromInt64(to)-RowCol::fromInt64(from);
    
    if ( diff.row<0 )
    {
	if ( diff.col<0 ) return cSourcePP();
	if ( !diff.col ) return cSourcePS();
	return cSourcePN();
    }

    if ( !diff.row )
    {
	if ( diff.col<0 ) return cSourceSP();
	if ( !diff.col )
	    pErrMsg("To and from are the same!");
	return cSourceSN();
    }

    if ( diff.col<0 ) return cSourceNP();
    if ( !diff.col ) return cSourceNS();
    return cSourceNN();
}
    

void HorizonAutoTracker::TrackTarget::reset()
{
    pid_ = EM::PosID::udf();
    score_ = mUdf(float);
    proposedz_ = mUdf(float);
    isvalid_ = false;
    sources_ = 0;
}

#define mBreak \
{ \
    reset(); \
    return; \
}

void HorizonAutoTracker::TrackTarget::computeProposal()
{
    needsrecalc_ = false;
    
    TypeSet<EM::PosID> sourcepids;
    tracker_->getSources( pid_, sourcepids );
	
    if ( !sourcepids.size() )
	mBreak;
		
    TypeSet<float> targets;
    TypeSet<float> scores;
    TypeSet<unsigned char> sources;
	    
    const SeisTrc* targettrc = tracker_->getTrace( pid_.subID() );
    if ( !targettrc )
	mBreak;
	    
    const float eps = targettrc->info().sampling.step * 0.1f;
	
    const float interpolz = tracker_->getInterpolZ( pid_ );

    int besttarget = -1;
	
    for ( int idx=sourcepids.size()-1; idx>=0; idx-- )
    {
	const SeisTrc* sourcetrc = tracker_->getTrace( sourcepids[idx].subID() );
	if ( !sourcetrc )
	    continue;
	    
	//Track from source to target
	float newz = interpolz;
	//TODO: Do the real tracking
	float newscore = 1;
	
	bool found = false;
	for ( int idy=targets.size()-1; idy>=0; idy-- )
	{
	    if ( mIsEqual(targets[idy], newz, eps ) )
	    {
		found = true;
		scores[idy] += newscore;
		sources[idy] +=
			getSourceMask( sourcepids[idx].subID(), pid_.subID() );
		break;
	    }
	}
	    
	if ( found )
	    continue;
	    
	if ( besttarget==-1 || scores[besttarget]<newscore )
	    besttarget = scores.size();
	    
	targets += newz;
	scores += newscore;
	sources += getSourceMask( sourcepids[idx].subID(), pid_.subID() );
    }
	
    if ( besttarget==-1 )
    {
	proposedz_ = mUdf( float );
	score_ = mUdf( float );
    }
    else
    {
	proposedz_ = targets[besttarget];
	score_ = scores[besttarget];
	sources_ = sources[besttarget];
	isvalid_ = true;
    }
}


void HorizonAutoTracker::fillPar( IOPar& par ) const
{
}


bool HorizonAutoTracker::usePar( const IOPar& par )
{
    return true;
}
    
