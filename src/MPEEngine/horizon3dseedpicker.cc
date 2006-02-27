/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Sep 2005
___________________________________________________________________

-*/

static const char* rcsID = "$Id: horizon3dseedpicker.cc,v 1.3 2006-02-27 11:07:15 cvsjaap Exp $";

#include "horizonseedpicker.h"

#include "autotracker.h"
#include "emhistory.h"
#include "emhorizon.h"
#include "emmanager.h"
#include "sectiontracker.h"
#include "executor.h"
#include "mpeengine.h"
#include "survinfo.h"
#include "sorting.h"

#define mInitialEventNrNotSet -2

namespace MPE 
{

HorizonSeedPicker::HorizonSeedPicker( MPE::EMTracker& t )
    : tracker_( t )
    , firsthistorynr_( mInitialEventNrNotSet )
    , seedmode_( TrackFromSeeds )
    , frozen_( false )
{}


bool HorizonSeedPicker::setSectionID( const EM::SectionID& sid )
{ sectionid_ = sid; return true; }


#define mGetHorizon(hor) \
    const EM::ObjectID emobjid = tracker_.objectID(); \
    mDynamicCastGet( EM::Horizon*, hor, EM::EMM().getObject(emobjid) ); \
    if ( !hor ) \
	return false;\

bool HorizonSeedPicker::startSeedPick()
{
    firsthistorynr_ =  EM::EMM().history().currentEventNr();
    seedlist_.erase();
    seedpos_.erase();

    mGetHorizon(hor);
    didchecksupport_ = hor->geometry.checkSupport(false);

    return true;
}


	
bool HorizonSeedPicker::addSeed(const Coord3& seedcrd3 )
{
    if ( frozen_ ) return true;

    BinID seedbid = SI().transform(seedcrd3);
    const HorSampling hrg = engine().activeVolume().hrg;
    const StepInterval<float> zrg = engine().activeVolume().zrg;

    if ( !zrg.includes(seedcrd3.z) || !hrg.includes(seedbid) )
	return false;

    // Set seed positions
    const EM::ObjectID emobjid = tracker_.objectID();
    EM::EMObject* emobj = EM::EMM().getObject(emobjid);
    const EM::PosID pid( emobj->id(), sectionid_, seedbid.getSerialized() );

    const int idx = seedlist_.indexOf(pid);
    if ( idx==-1 )
    {
	seedlist_ += pid;
	seedpos_ += seedcrd3;
    }
    else
    {
	seedpos_[idx] = seedcrd3;
    }

    return reTrack();
}


bool HorizonSeedPicker::removeSeed( const EM::PosID& pid)
{
    int idx = seedlist_.indexOf(pid);
    if ( idx == -1 ) return false;
    
    seedlist_.remove(idx);
    seedpos_.remove(idx);

    return reTrack();
}


bool HorizonSeedPicker::reTrack()
{
    removeEverythingButSeeds();
    if ( !seedlist_.size() )
	return true;

    if ( frozen_ ) return true;
   
    if ( seedmode_ == DrawBetweenSeeds )
	return interpolateSeeds();
   
    const TrackPlane::TrackMode tm = engine().trackPlane().getTrackMode();
    engine().setTrackMode( TrackPlane::Extend );

    Executor* execfromeng = engine().trackInVolume();

    mDynamicCastGet( ExecutorGroup*, trkersgrp, execfromeng );
    if ( !trkersgrp )	return false;

    for( int trker=0; trker<trkersgrp->nrExecutors(); ++trker)
    {
	Executor* exectrk = trkersgrp->getExecutor(trker);
	mDynamicCastGet( ExecutorGroup*, sectiongrp, exectrk );
	if ( !sectiongrp )	break;
	
	for( int section=0; section<sectiongrp->nrExecutors(); ++section )
	{
	    Executor* exec = sectiongrp->getExecutor(section);
	    if ( !exec )	break;

	    mDynamicCastGet( AutoTracker*, autotrk, exec );
	    if ( !autotrk )	break;

	    if ( seedmode_ == TrackBetweenSeeds )
		autotrk->setTrackBoundary( getSeedBox() );
	    autotrk->setNewSeeds( seedlist_ );
	    autotrk->execute();
	    autotrk->unsetTrackBoundary();
	}
    }

    delete trkersgrp;
    
    engine().setTrackMode( tm );

    return true;
}


int HorizonSeedPicker::nrSeeds() const
{ return seedpos_.size(); }


int HorizonSeedPicker::isMinimumNrOfSeeds() const
{ return getSeedMode()==TrackFromSeeds ? 1 : 0 ; }



bool HorizonSeedPicker::removeEverythingButSeeds()
{
    if ( firsthistorynr_ != mInitialEventNrNotSet )
	EM::EMM().history().unDo(
	    EM::EMM().history().currentEventNr() - firsthistorynr_ );

    const EM::ObjectID emobjid = tracker_.objectID();
    EM::EMObject* emobj = EM::EMM().getObject(emobjid);

    for ( int idx=0; idx<seedlist_.size(); idx++ )
    {
	emobj->setPos( seedlist_[idx], seedpos_[idx], true );
	emobj->setPosAttrib( seedlist_[idx], EM::EMObject::sSeedNode, true );
    }

    return true;
}


bool HorizonSeedPicker::stopSeedPick( bool iscancel )
{
    if ( iscancel )
    {
	if ( firsthistorynr_ != mInitialEventNrNotSet )
	    EM::EMM().history().unDo(
		EM::EMM().history().currentEventNr() - firsthistorynr_ );
	EM::EMObject* emobj = EM::EMM().getObject(tracker_.objectID());
	for ( int idx=0; idx<seedlist_.size(); idx++ )
	    emobj->setPosAttrib(seedlist_[idx], EM::EMObject::sSeedNode, false);
    }
    mGetHorizon(hor);
    hor->geometry.checkSupport( didchecksupport_ );
    seedlist_.erase();
    seedpos_.erase();
    firsthistorynr_ = mInitialEventNrNotSet;
    return true;
}

#define mSortSeeds(rorc) \
{ \
    for ( int idx=0; idx<nrseeds; idx++) \
    { \
	rorc##value[idx] = SI().transform( seedpos_[idx] ).rorc(); \
	rorc##index[idx] = idx; \
    } \
    sort_coupled( rorc##value, rorc##index, nrseeds ); \
}


#define mInterpolSeeds(rorc) \
{ \
    mGetHorizon(horptr); \
    EM::EMObject* emobj = EM::EMM().getObject(emobjid); \
    const int step = horptr->geometry.step().rorc(); \
    \
    for ( int vtx=0; vtx<nrseeds-1; vtx++ ) \
    { \
	const int diff = rorc##value[vtx+1] - rorc##value[vtx]; \
	for ( int idx=step; idx<diff; idx+=step ) \
	{ \
	    const double frac = (double) idx / diff; \
	    const Coord3 interpos = (1-frac) * seedpos_[ rorc##index[vtx] ]  \
				    + frac * seedpos_[ rorc##index[vtx+1] ]; \
	    const EM::PosID interpid( emobj->id(), sectionid_, \
				SI().transform(interpos).getSerialized() ); \
	    emobj->setPos( interpid, interpos, true ); \
	} \
    } \
}

bool HorizonSeedPicker::interpolateSeeds()
{
    const int nrseeds = nrSeeds();
    if ( nrseeds<2 ) return false;

    int rvalue[ nrseeds ], rindex[ nrseeds ]; 
    int cvalue[ nrseeds ], cindex[ nrseeds ]; 

    mSortSeeds(r); mSortSeeds(c);

    if ( rvalue[nrseeds-1] - rvalue[0] > cvalue[nrseeds-1] - cvalue[0] ) 
    {
	mInterpolSeeds(r);
    }
    else
	mInterpolSeeds(c);

    return true;
}


CubeSampling HorizonSeedPicker::getSeedBox() const
{
    CubeSampling seedbox( true );
    seedbox.hrg.init( false );
    for ( int idx=0; idx<nrSeeds(); idx++ )
    {
	BinID seedbid = SI().transform( seedpos_[idx] );
	seedbox.hrg.include( seedbid );
    }
    return seedbox;
}


}; // namespace MPE

