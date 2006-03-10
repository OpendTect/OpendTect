/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Sep 2005
___________________________________________________________________

-*/

static const char* rcsID = "$Id: horizon3dseedpicker.cc,v 1.5 2006-03-10 16:07:05 cvsjaap Exp $";

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

//    emobj->setPos( pid, seedcrd3, true );
//    emobj->setPosAttrib( pid, EM::EMObject::sSeedNode, true);

    return reTrack();
}


bool HorizonSeedPicker::removeSeed( const EM::PosID& pid)
{
    int idx = seedlist_.indexOf(pid);
    if ( idx == -1 ) return false;
   
    // TO DO: Removal of seeds outside the active volume. Temporarily disabled
    // because there is no possibility to retrack. seedClick() gets the volume
    // of the controlclicked seed, not the volume of the underlying in/crline.
    BinID seedbid = SI().transform( seedpos_[idx] );
    const HorSampling hrg = engine().activeVolume().hrg;
    const StepInterval<float> zrg = engine().activeVolume().zrg;

    if ( !zrg.includes( seedpos_[idx].z ) || !hrg.includes(seedbid) )
	return false;
    // End of shield.

    seedlist_.remove(idx);
    seedpos_.remove(idx);

    return reTrack();
}


bool HorizonSeedPicker::reTrack()
{
   EM::EMObject* emobj = EM::EMM().getObject( tracker_.objectID() );  

   /* for ( int idx=seedlist_.size()-1; idx>=0; idx-- )
    {
	if ( !emobj->isDefined( seedlist_[idx] ) )
	{
	    seedlist_.remove(idx);
	    seedpos_.remove(idx);
	}
    }
    */

    if ( !clearActiveLine() )
    	return false;
    
    for ( int idx=0; idx<seedlist_.size(); idx++ )
    {
	if ( !emobj->isDefined( seedlist_[idx] ) )
	{
	    emobj->setPos( seedlist_[idx], seedpos_[idx], true );
	    emobj->setPosAttrib( seedlist_[idx], EM::EMObject::sSeedNode, true);
	}
    }
   
    if ( !retrackActiveLine() );
	return false;

    repairDisconnections();
    return true;
}


bool HorizonSeedPicker::retrackActiveLine()
{
    if ( !seedlist_.size() )
	return true;
    if ( frozen_ ) 
	return true;
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


bool HorizonSeedPicker::isInVolumeMode() const
{ return getSeedMode()==TrackFromSeeds; }


/*
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
*/


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


void HorizonSeedPicker::repairDisconnections()
{
    EM::EMObject* emobj = EM::EMM().getObject( tracker_.objectID() );  

    for ( int idx=0; idx<crosspid_.size(); idx++ )
    {
	if ( !emobj->isDefined( crosspid_[idx] ) )
	    emobj->setPos( crosspid_[idx], crosspos_[idx], true ); 
    }
}


bool HorizonSeedPicker::clearActiveLine()
{
    crosspid_.erase(); crosspos_.erase();

    bool inlineactive = false; 
    if ( engine().activeVolume().nrInl() == 1 ) 
	inlineactive = true; 
    else if ( engine().activeVolume().nrCrl() != 1 ) 
	return false; 

    EM::EMObject* emobj = EM::EMM().getObject( tracker_.objectID() );  

    const StepInterval<int> inlrng = engine().activeVolume().hrg.inlRange();
    const StepInterval<int> crlrng = engine().activeVolume().hrg.crlRange();
    
    for ( int inl=inlrng.start; inl<=inlrng.stop; inl+=inlrng.step )
    {
	for ( int crl=crlrng.start; crl<=crlrng.stop; crl+=crlrng.step ) 
	{
	    const BinID curbid( inl, crl );
	    BinID leftbid( inl-inlrng.step, crl );
	    BinID rightbid( inl+inlrng.step, crl );
	    if ( inlineactive )
	    {
		leftbid = BinID( inl, crl-crlrng.step );
		rightbid = BinID( inl, crl+crlrng.step );
	    }

	    const EM::PosID curpid( emobj->id(), sectionid_, 
						     curbid.getSerialized() ); 
	    const EM::PosID leftpid( emobj->id(), sectionid_, 
						    leftbid.getSerialized() ); 
	    const EM::PosID rightpid( emobj->id(), sectionid_, 
						   rightbid.getSerialized() ); 

	    const bool crossconnected = emobj->isDefined(curpid) &&
		  ( emobj->isDefined(leftpid) || emobj->isDefined(rightpid) );
	    if ( crossconnected ) 
	    {
		crosspid_ += curpid;
		crosspos_ += emobj->getPos( curpid );
	    }

	    emobj->unSetPos( curpid, true );
	}
    }
    return true;
}


bool HorizonSeedPicker::interpolateSeeds()
{
    bool inlineactive = false; 
    if ( engine().activeVolume().nrInl() == 1 ) 
	inlineactive = true; 
    else if ( engine().activeVolume().nrCrl() != 1 ) 
	return false; 
    const int step = inlineactive ? 
		     engine().activeVolume().hrg.crlRange().step : 
		     engine().activeVolume().hrg.inlRange().step ; 

    const int nrseeds = nrSeeds();
    int sortval[nrseeds], sortidx[nrseeds]; 
    int nractives = 0;

    for ( int idx=0; idx<nrseeds; idx++ )
    {
	const BinID seedbid = SI().transform( seedpos_[idx] );
	if ( engine().activeVolume().hrg.includes(seedbid) )
	{
	    sortval[ nractives ] = inlineactive ? seedbid.crl : seedbid.inl;
	    sortidx[ nractives++ ] = idx;
	}
    }	
    if ( nractives<2 ) return true;

    sort_coupled( sortval, sortidx, nractives ); 
    
    EM::EMObject* emobj = EM::EMM().getObject( tracker_.objectID() );  

    for ( int vtx=0; vtx<nractives-1; vtx++ ) 
    { 
	const int diff = sortval[ vtx+1 ] - sortval[ vtx ]; 
	for ( int idx=step; idx<diff; idx+=step ) 
	{ 
	    const double frac = (double) idx / diff; 
	    const Coord3 interpos = (1-frac) * seedpos_[ sortidx[vtx] ] + 
				       frac  * seedpos_[ sortidx[vtx+1] ];
	    const EM::PosID interpid( emobj->id(), sectionid_, 
				SI().transform(interpos).getSerialized() ); 
	    emobj->setPos( interpid, interpos, true ); 
	} 
    } 
    return true;
}


CubeSampling HorizonSeedPicker::getSeedBox() const
{
    CubeSampling seedbox( true );
    seedbox.hrg.init( false );
    for ( int idx=0; idx<nrSeeds(); idx++ )
    {
	const BinID seedbid = SI().transform( seedpos_[idx] );
	if ( engine().activeVolume().hrg.includes(seedbid) )
	    seedbox.hrg.include( seedbid );
    }
    return seedbox;
}


}; // namespace MPE

