/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Sep 2005
___________________________________________________________________

-*/

static const char* rcsID = "$Id: horizon3dseedpicker.cc,v 1.2 2006-01-25 14:49:53 cvsjaap Exp $";

#include "horizonseedpicker.h"

#include "autotracker.h"
#include "emhistory.h"
#include "emhorizon.h"
//#include "emfault.h"
#include "emmanager.h"
#include "executor.h"
#include "mpeengine.h"
#include "survinfo.h"
#include "sorting.h"

#define mInitialEventNrNotSet -2

namespace MPE 
{

HorizonSeedPicker::HorizonSeedPicker( MPE::EMTracker& t )
    : tracker( t )
    , firsthistorynr( mInitialEventNrNotSet )
    , interpolmode( false )  
{}


bool HorizonSeedPicker::setSectionID( const EM::SectionID& sid )
{ sectionid = sid; return true; }


#define mGetHorizon(hor) \
    const EM::ObjectID emobjid = tracker.objectID(); \
    mDynamicCastGet( EM::Horizon*, hor, EM::EMM().getObject(emobjid) ); \
    if ( !hor ) \
	return false;\

bool HorizonSeedPicker::startSeedPick()
{
    firsthistorynr =  EM::EMM().history().currentEventNr();
    seedlist.erase();
    seedpos.erase();

    mGetHorizon(hor);
    didchecksupport = hor->geometry.checkSupport(false);

    return true;
}


	
bool HorizonSeedPicker::addSeed(const Coord3& seedcrd3 )
{
    BinID seedbid = SI().transform(seedcrd3);
    const HorSampling hrg = engine().activeVolume().hrg;
    const StepInterval<float> zrg = engine().activeVolume().zrg;
    if ( !zrg.includes(seedcrd3.z) || !hrg.includes(seedbid) )
	return false;

    // Set seed positions
    const EM::ObjectID emobjid = tracker.objectID();
    EM::EMObject* emobj = EM::EMM().getObject(emobjid);
    const EM::PosID pid( emobj->id(), sectionid, seedbid.getSerialized() );

    const int idx = seedlist.indexOf(pid);
    if ( idx==-1 )
    {
	seedlist += pid;
	seedpos += seedcrd3;
    }
    else
    {
	seedpos[idx] = seedcrd3;
    }

    return reTrack();
}


bool HorizonSeedPicker::removeSeed( const EM::PosID& pid)
{
    int idx = seedlist.indexOf(pid);
    if ( idx == -1 )	return false;
    
    seedlist.remove(idx);
    seedpos.remove(idx);

    return reTrack();
}


bool HorizonSeedPicker::reTrack()
{
    removeEverythingButSeeds();
    if ( !seedlist.size() )
	return true;

    if ( interpolmode )
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

	    autotrk->setNewSeeds( seedlist );
	    autotrk->execute();
	}
    }

    delete trkersgrp;
    
    engine().setTrackMode( tm );

    return true;
}


bool HorizonSeedPicker::removeEverythingButSeeds()
{
    if ( firsthistorynr != mInitialEventNrNotSet )
	EM::EMM().history().unDo(
	    EM::EMM().history().currentEventNr() - firsthistorynr );

    const EM::ObjectID emobjid = tracker.objectID();
    EM::EMObject* emobj = EM::EMM().getObject(emobjid);

    for ( int idx=0; idx<seedlist.size(); idx++ )
    {
	emobj->setPos( seedlist[idx], seedpos[idx], true );
	emobj->setPosAttrib( seedlist[idx], EM::EMObject::sSeedNode, true );
    }

    return true;
}


bool HorizonSeedPicker::stopSeedPick( bool iscancel )
{
    if ( iscancel )
    {
	if ( firsthistorynr != mInitialEventNrNotSet )
	    EM::EMM().history().unDo(
		EM::EMM().history().currentEventNr() - firsthistorynr );
	EM::EMObject* emobj = EM::EMM().getObject(tracker.objectID());
	for ( int idx=0; idx<seedlist.size(); idx++ )
	    emobj->setPosAttrib(seedlist[idx], EM::EMObject::sSeedNode, false);
    }
    mGetHorizon(hor);
    hor->geometry.checkSupport( didchecksupport );
    seedlist.erase();
    seedpos.erase();
    firsthistorynr = mInitialEventNrNotSet;
    return true;
}


#define mSortSeeds(rorc) \
{ \
    for ( int idx=0; idx<nrseeds; idx++) \
    { \
	rorc##value[idx] = SI().transform( seedpos[idx] ).rorc(); \
	rorc##index[idx] = idx; \
    } \
    sort_coupled( rorc##value, rorc##index, nrseeds ); \
}


#define mInterpolSeeds(rorc) \
{ \
    mGetHorizon(horptr); \
    EM::EMObject* emobj = EM::EMM().getObject(emobjid); \
    const RowCol step = horptr->geometry.step(); \
    \
    for ( int vtx=0; vtx<nrseeds-1; vtx++ ) \
    { \
	const int diff = rorc##value[vtx+1] - rorc##value[vtx]; \
	for ( int idx=step.rorc(); idx<diff; idx+=step.rorc() ) \
	{ \
	    const double frac = (double) idx / diff; \
	    const Coord3 interpos = (1-frac) * seedpos[ rorc##index[vtx] ]  \
				    + frac * seedpos[ rorc##index[vtx+1] ]; \
	    const EM::PosID interpid( emobj->id(), sectionid, \
				SI().transform(interpos).getSerialized() ); \
	    emobj->setPos( interpid, interpos, true ); \
	    emobj->setPosAttrib( interpid, EM::EMObject::sSeedNode, true ); \
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

}; // namespace MPE

