/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID = "$Id: autotracker.cc,v 1.21 2009-08-21 12:47:26 cvsumesh Exp $";

#include "autotracker.h"

#include "emmanager.h"
#include "emobject.h"
#include "emtracker.h"
#include "horizonadjuster.h"
#include "mpeengine.h"
#include "progressmeter.h"
#include "sectionextender.h"
#include "sectionadjuster.h"
#include "sectiontracker.h"
#include "survinfo.h"
#include "timefun.h"

namespace MPE 
{

AutoTracker::AutoTracker( EMTracker& et, const EM::SectionID& sid )
    : Executor("Autotracker")
    , emobject_( *EM::EMM().getObject(et.objectID()) )
    , sectionid_( sid )
    , sectiontracker_( et.getSectionTracker(sid,true) )
    , nrdone_( 0 )
    , totalnr_( 0 )
    , nrflushes_( 0 )
    , flushcntr_( 0 )
    , stepcntallowedvar_(-1)
    , stepcntapmtthesld_(-1)
    , trackingextriffail_(false)
    , burstalertactive_(false)
{
    geomelem_ = emobject_.sectionGeometry(sectionid_);
    extender_ = sectiontracker_->extender();
    adjuster_ = sectiontracker_->adjuster();

    trackingextriffail_ = adjuster_->removesOnFailure();

    reCalculateTtalNr();

    mDynamicCastGet(HorizonAdjuster*,horadj,sectiontracker_->adjuster());
    if ( horadj )
    {
	if ( horadj->useAbsThreshold() )
	{
	    if ( horadj->getAmplitudeThresholds().size()>0 ) 
	    {
		stepcntapmtthesld_ = 0;
		stepcntallowedvar_ = -1;
		if ( horadj->getAmplitudeThresholds().size() > 1 )
		    adjuster_->removeOnFailure( true );
		horadj->setAmplitudeThreshold(
			horadj->getAmplitudeThresholds()[stepcntapmtthesld_] );
	    }
	}
	else if ( horadj->getAllowedVariances().size()>0 ) 
	{ 
	    stepcntallowedvar_ = 0;
	    stepcntapmtthesld_ = -1;
	    if ( horadj->getAllowedVariances().size()>1 )
		adjuster_->removeOnFailure( true );
	    horadj->setAllowedVariance(
		    horadj->getAllowedVariances()[stepcntallowedvar_] );
	    
	}
    }
}


AutoTracker::~AutoTracker()
{
    manageCBbuffer( false );
    geomelem_->trimUndefParts();
    emobject_.setBurstAlert( false );
    burstalertactive_ = false;
}


void AutoTracker::reCalculateTtalNr()
{
    PtrMan<EM::EMObjectIterator>iterator =
	emobject_.createIterator( sectionid_, &engine().activeVolume() );

    totalnr_ = extender_->maxNrPosInExtArea();

    while( true )
    {
	const EM::PosID pid = iterator->next();
	if ( pid.objectID()==-1 )
	    break;

	totalnr_--;
	if ( !sectiontracker_->propagatingFromSeedOnly() ||
	     emobject_.isPosAttrib(pid,EM::EMObject::sSeedNode()) )
	    addSeed(pid);
    }

    if ( currentseeds_.isEmpty() )
	totalnr_ = 0;
}


void AutoTracker::manageCBbuffer( bool block )
{
    if ( !block )
    {
	geomelem_->blockCallBacks( false, true );
	nrflushes_ = 0; flushcntr_ = 0; 
	return;
    }
    
    // progressive flushing (i.e. wait one extra cycle for every next flush)
    flushcntr_++;
    if ( flushcntr_ >= nrflushes_ )
    {
	flushcntr_ = 0;
	geomelem_->blockCallBacks( true, true );
	nrflushes_++;
    }
}


void AutoTracker::setNewSeeds( const TypeSet<EM::PosID>& seeds )
{
    currentseeds_.erase();

    for( int idx=0; idx<seeds.size(); ++idx )
	addSeed(seeds[idx]);
}


int AutoTracker::nextStep()
{
    manageCBbuffer( true );

    if ( !nrdone_ )
    {
	if ( !burstalertactive_ )
	{
	    emobject_.setBurstAlert( true );
	    burstalertactive_ = true;
	}

	extender_->preallocExtArea();
    }

    extender_->reset();
    extender_->setDirection( BinIDValue(BinID(0,0), mUdf(float)) );
    extender_->setStartPositions(currentseeds_);
    int res;
    while ( (res=extender_->nextStep())>0 )
	;

    TypeSet<EM::SubID> addedpos = extender_->getAddedPositions();
    TypeSet<EM::SubID> addedpossrc = extender_->getAddedPositionsSource();

    //Remove nodes that have failed 8 times before
    for ( int idx=0; idx<addedpos.size(); idx++ )
    {
	const int blacklistidx = blacklist_.indexOf(addedpos[idx]);
	if ( blacklistidx<0 ) continue;
	if ( blacklistscore_[blacklistidx]>7 )
	{
	    const EM::PosID pid( emobject_.id(), sectionid_, addedpos[idx] );
	    emobject_.unSetPos(pid,false);
	    addedpos.remove(idx);
	    addedpossrc.remove(idx);
	    idx--;
	}
    }

    adjuster_->reset();    
    adjuster_->setPositions(addedpos, &addedpossrc);
    while ( (res=adjuster_->nextStep())>0 )
	;

    //Not needed anymore, so we avoid hazzles below if we simply empty it
    addedpossrc.erase();

    //Add positions that have failed to blacklist
    for ( int idx=0; idx<addedpos.size(); idx++ )
    {
	const EM::PosID pid( emobject_.id(), sectionid_, addedpos[idx] );
	if ( !emobject_.isDefined(pid) )
	{
	    const int blacklistidx = blacklist_.indexOf(addedpos[idx]);
	    if ( blacklistidx!=-1 ) blacklistscore_[blacklistidx]++;
	    else
	    {
		blacklist_ += addedpos[idx];
		blacklistscore_ += 1;
	    }

	    addedpos.remove(idx);
	    idx--;
	}
    }

    //Some positions failed in the optimization, wich may lead to that
    //others are unsupported. Remove all unsupported nodes.
    sectiontracker_->removeUnSupported( addedpos );

    // Make all new nodes seeds
    currentseeds_ = addedpos;
    nrdone_ += currentseeds_.size();
    
    int status =  currentseeds_.size() ? MoreToDo() : Finished();
    if ( status == Finished() )
    {
	mDynamicCastGet(HorizonAdjuster*,horadj,sectiontracker_->adjuster());
	if ( !horadj )
	{
	    stepcntallowedvar_ = -1;
	    stepcntapmtthesld_ = -1;
	    return status;
	}

	if ( horadj->useAbsThreshold() )
	{
	    if ( stepcntapmtthesld_ == -1 )
		return status;

	    if ( horadj->getAmplitudeThresholds().size() <=
		 (stepcntapmtthesld_+1) )
	    {
		stepcntapmtthesld_ = -1;
		return status;
	    }

	    stepcntapmtthesld_++;

	    if( horadj->getAmplitudeThresholds().size() ==
		(stepcntapmtthesld_+1) )
		adjuster_->removeOnFailure( trackingextriffail_ );

	    reCalculateTtalNr();
	    horadj->setAmplitudeThreshold(
		    horadj->getAmplitudeThresholds()[stepcntapmtthesld_] );

	    return MoreToDo();
	}
	else
	{
	    if ( stepcntallowedvar_ == -1 )
		return status;

	    if ( horadj->getAllowedVariances().size() <= (stepcntallowedvar_+1))
	    {
		stepcntallowedvar_ = -1;
		return status;
	    }

	    stepcntallowedvar_++;

	    if ( horadj->getAllowedVariances().size() == (stepcntallowedvar_+1))
		adjuster_->removeOnFailure( trackingextriffail_ );

	    reCalculateTtalNr();
	    horadj->setAllowedVariance(
		    horadj->getAllowedVariances()[stepcntallowedvar_] );
	    
	    return MoreToDo();
	}
    }

    return status;
}


void AutoTracker::setTrackBoundary( const CubeSampling& cs )
{ extender_->setExtBoundary( cs ); }


void AutoTracker::unsetTrackBoundary()
{ extender_->unsetExtBoundary(); }


bool AutoTracker::addSeed( const EM::PosID& pid )
{
    if ( pid.sectionID()!=sectionid_ )	return false;
    if ( !emobject_.isAtEdge(pid) )	return false;

    const Coord3& pos = emobject_.getPos(pid);
    if ( !engine().activeVolume().zrg.includes(pos.z) )	return false;
    const BinID bid = SI().transform(pos);
    if ( !engine().activeVolume().hrg.includes(bid) )	return false;

    currentseeds_ += pid.subID();
    return true;
}


}; // namespace MPE
