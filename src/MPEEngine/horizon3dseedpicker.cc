/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Sep 2005
___________________________________________________________________

-*/

static const char* rcsID = "$Id: horizon3dseedpicker.cc,v 1.22 2006-12-01 16:37:52 cvsjaap Exp $";

#include "horizonseedpicker.h"

#include "autotracker.h"
#include "emhorizon.h"
#include "emmanager.h"
#include "sectiontracker.h"
#include "executor.h"
#include "mpeengine.h"
#include "survinfo.h"
#include "sorting.h"


namespace MPE 
{

HorizonSeedPicker::HorizonSeedPicker( MPE::EMTracker& t )
    : tracker_( t )
    , addrmseed_( this )
    , surfchange_( this )
    , seedconmode_( defaultSeedConMode() )
    , blockpicking_( false )
{ }


bool HorizonSeedPicker::setSectionID( const EM::SectionID& sid )
{ 
    sectionid_ = sid; 
    return true; 
}


#define mGetHorizon(hor) \
    const EM::ObjectID emobjid = tracker_.objectID(); \
    mDynamicCastGet( EM::Horizon*, hor, EM::EMM().getObject(emobjid) ); \
    if ( !hor ) \
	return false;\

bool HorizonSeedPicker::startSeedPick()
{
    mGetHorizon(hor);
    didchecksupport_ = hor->enableGeometryChecks( false );
    return true;
}

	
bool HorizonSeedPicker::addSeed(const Coord3& seedcrd )
{
    addrmseed_.trigger();

    if ( blockpicking_ ) 
	return true;

    BinID seedbid = SI().transform( seedcrd );
    const HorSampling hrg = engine().activeVolume().hrg;
    const StepInterval<float> zrg = engine().activeVolume().zrg;
    if ( !zrg.includes(seedcrd.z) || !hrg.includes(seedbid) )
	return false;

    EM::EMObject* emobj = EM::EMM().getObject( tracker_.objectID() );
    const EM::PosID pid( emobj->id(), sectionid_, seedbid.getSerialized() );

    const bool startwasdefined = emobj->isDefined( pid );

    emobj->setPos( pid, seedcrd, true );
    if ( !emobj->isPosAttrib( pid, EM::EMObject::sSeedNode ) )
	emobj->setPosAttrib( pid, EM::EMObject::sSeedNode, true );

    seedlist_.erase(); seedpos_.erase(); 
    seedlist_ += pid;  seedpos_ += seedcrd;

    if ( seedconmode_ != DrawBetweenSeeds )
	tracker_.snapPositions( seedlist_ );

    return retrackOnActiveLine( seedbid, startwasdefined );
}


bool HorizonSeedPicker::removeSeed( const EM::PosID& pid, bool retrack )
{
    addrmseed_.trigger();
    
    if ( blockpicking_ ) 
	return true;
    
    EM::EMObject* emobj = EM::EMM().getObject( tracker_.objectID() );  
    const Coord3 oldpos = emobj->getPos(pid);
    BinID seedbid = SI().transform( oldpos );
    const bool attribwasnotremovedalready = 
			    emobj->isPosAttrib( pid, EM::EMObject::sSeedNode );

    emobj->unSetPos( pid, true );

    if ( !retrack && !nrLineNeighbors(pid) ) return true;
    
    seedlist_.erase(); seedpos_.erase(); 

    const int res = retrackOnActiveLine( seedbid, true, !retrack );

    if ( nrLateralNeighbors(pid) )
    {
	if ( !emobj->isDefined( pid ) )
	    emobj->setPos( pid, oldpos, true );

	if  ( attribwasnotremovedalready )
	    emobj->setPosAttrib( pid, EM::EMObject::sSeedNode, true );
    }

    return res;
}


bool HorizonSeedPicker::reTrack()
{
    seedlist_.erase(); seedpos_.erase(); 
    
    return retrackOnActiveLine( BinID(-1,-1), false );
}


bool HorizonSeedPicker::retrackOnActiveLine( const BinID& startbid, 
					     bool startwasdefined,
					     bool eraseonly ) 
{
    BinID dir;
    if ( !lineTrackDirection(dir) ) return false;

    trackbounds_.erase();
    TypeSet<EM::PosID> candidatejunctionpairs;

    if ( engine().activeVolume().hrg.includes(startbid) )
    {
	extendSeedListEraseInBetween( false, startbid, startwasdefined, 
				      -dir, candidatejunctionpairs );
	extendSeedListEraseInBetween( false, startbid, startwasdefined, 
				      dir, candidatejunctionpairs );
    }
    else
    {
	// traverse whole active line
	const BinID dummystartbid = engine().activeVolume().hrg.start;
	extendSeedListEraseInBetween( true, dummystartbid, false, 
				      -dir, candidatejunctionpairs );
	extendSeedListEraseInBetween( true, dummystartbid-dir, false, 
				      dir, candidatejunctionpairs );
	
	if ( seedconmode_ != DrawBetweenSeeds )
	    tracker_.snapPositions( seedlist_ );
    }
    
    bool res = true;

    if ( !eraseonly )
    {
	res = retrackFromSeedList();

	EM::EMObject* emobj = EM::EMM().getObject( tracker_.objectID() );  

	for ( int idx=0; idx<candidatejunctionpairs.size(); idx+=2 )
	{
	    if ( emobj->isDefined(candidatejunctionpairs[idx]) )
	    {
		emobj->setPosAttrib( candidatejunctionpairs[idx+1], 
				     EM::EMObject::sSeedNode, true );
	    }
	}
    }

    surfchange_.trigger();
    return res;
}


void HorizonSeedPicker::extendSeedListEraseInBetween( 
		bool wholeline, const BinID& startbid, bool startwasdefined,
		const BinID& dir, TypeSet<EM::PosID>& candidatejunctionpairs )
{
    EM::EMObject* emobj = EM::EMM().getObject( tracker_.objectID() );  
    EM::PosID curpid = EM::PosID( emobj->id(), sectionid_, 
				  startbid.getSerialized() ); 
    BinID curbid = startbid;
    bool curdefined = startwasdefined;

    while ( true )
    {
	const BinID prevbid = curbid;
	const EM::PosID prevpid = curpid;
	const bool prevdefined = curdefined;
	
    	curbid += dir;
	
	// reaching end of survey
	if ( !engine().activeVolume().hrg.includes(curbid) )
	{
	    if  ( seedconmode_==TrackFromSeeds )
		trackbounds_ += prevbid;
	    return;
	}
	
	curpid = EM::PosID( emobj->id(), sectionid_, curbid.getSerialized() ); 
	curdefined = emobj->isDefined(curpid);

	// running into a seed point
	if ( emobj->isPosAttrib( curpid, EM::EMObject::sSeedNode ) )
	{
	    seedlist_ += curpid;
	    seedpos_ += emobj->getPos( curpid );
	    
	    if ( !wholeline )
		return;
	    
	    continue;
	}
	
	// running into a loose end
	if ( !wholeline && !prevdefined && curdefined )
	{
	    if  ( seedconmode_==DrawBetweenSeeds )
	    {
		seedlist_ += curpid;
		seedpos_ += emobj->getPos( curpid );
	    }
	    else
		trackbounds_ += curbid;

	    candidatejunctionpairs += prevpid;
	    candidatejunctionpairs += curpid;
	    return;
	}

	// erasing points attached to start
	if ( curdefined ) 
	    emobj->unSetPos( curpid, true );
    }
}


bool HorizonSeedPicker::retrackFromSeedList()
{
    if ( seedlist_.isEmpty() )
	return true;
    if ( blockpicking_ ) 
	return true;
    if ( seedconmode_ == DrawBetweenSeeds )
	return interpolateSeeds(); 
    
    const TrackPlane::TrackMode tm = engine().trackPlane().getTrackMode();
    engine().setTrackMode( TrackPlane::Extend );

    PtrMan<Executor> execfromeng = engine().trackInVolume();

    mDynamicCastGet( ExecutorGroup*, trkersgrp, execfromeng.ptr() );
    if ( !trkersgrp )
	return false;

    for( int trker=0; trker<trkersgrp->nrExecutors(); ++trker)
    {
	Executor* exectrk = trkersgrp->getExecutor(trker);
	mDynamicCastGet( ExecutorGroup*, sectiongrp, exectrk );
	if ( !sectiongrp )
	    break;
	
	for( int section=0; section<sectiongrp->nrExecutors(); ++section )
	{
	    Executor* exec = sectiongrp->getExecutor(section);
	    if ( !exec )
		break;

	    mDynamicCastGet( AutoTracker*, autotrk, exec );
	    if ( !autotrk )
		break;
	    autotrk->setTrackBoundary( getTrackBox() );
	    autotrk->setNewSeeds( seedlist_ );
	    autotrk->execute();
	    autotrk->unsetTrackBoundary();
	}
    }

    engine().setTrackMode( tm );

    return true;
}


int HorizonSeedPicker::nrSeeds() const
{
    EM::EMObject* emobj = EM::EMM().getObject( tracker_.objectID() );  
    const TypeSet<EM::PosID>* seednodelist = 
			emobj->getPosAttribList( EM::EMObject::sSeedNode );
    return seednodelist ? seednodelist->size() : 0;
}

const char* HorizonSeedPicker::seedConModeText( int mode, bool abbrev )
{
    if ( mode==TrackFromSeeds && !abbrev )
	return "Tracking in volume";
    else if ( mode==TrackFromSeeds && abbrev )
	return "Volume track";
    else if ( mode==TrackBetweenSeeds )
	return "Line tracking";
    else if ( mode==DrawBetweenSeeds )
	return "Line manual";
    else
	return "Unknown mode";
}

int HorizonSeedPicker::minSeedsToLeaveInitStage() const
{ 
    if ( seedconmode_==TrackFromSeeds ) 
	return 1;
    else if ( seedconmode_==TrackBetweenSeeds ) 
	return 2;
    else 
	return 0 ; 
}


bool HorizonSeedPicker::doesModeUseVolume() const
{ return seedconmode_==TrackFromSeeds; }


bool HorizonSeedPicker::doesModeUseSetup() const
{ return seedconmode_!=DrawBetweenSeeds; }


int HorizonSeedPicker::defaultSeedConMode( bool gotsetup ) const 
{ return gotsetup ? defaultSeedConMode() : DrawBetweenSeeds; }


bool HorizonSeedPicker::stopSeedPick( bool iscancel )
{
    mGetHorizon(hor);
    hor->enableGeometryChecks( didchecksupport_ );
    return true;
}


bool HorizonSeedPicker::lineTrackDirection( BinID& dir, 
					    bool perptotrackdir ) const
{
    const CubeSampling& activevol = engine().activeVolume();
    dir = activevol.hrg.step;
    
    const bool inltracking = activevol.nrInl()==1 && activevol.nrCrl()>1;
    const bool crltracking = activevol.nrCrl()==1 && activevol.nrInl()>1;

    if ( !inltracking && !crltracking )
	return false;

    if ( !perptotrackdir && inltracking  ||  perptotrackdir && crltracking )
    {
	dir.inl = 0; 
    }
    else
    {
	dir.crl = 0;
    }

    return true;
}


int HorizonSeedPicker::nrLateralNeighbors( const EM::PosID& pid ) const
{
    return nrLineNeighbors( pid, true );
}


int HorizonSeedPicker::nrLineNeighbors( const EM::PosID& pid, 
				        bool perptotrackdir ) const
{
    EM::EMObject* emobj = EM::EMM().getObject( tracker_.objectID() );  
    BinID bid = SI().transform( emobj->getPos(pid) );

    TypeSet<EM::PosID> neighpid;
    mGetHorizon(hor);
    hor->geometry().getConnectedPos( pid, &neighpid );
   
    BinID dir;
    if ( !lineTrackDirection(dir,perptotrackdir) )
	return -1;

    int total = 0;
    for ( int idx=0; idx<neighpid.size(); idx++ )
    {
	BinID neighbid = SI().transform( emobj->getPos(neighpid[idx]) ); 
	if ( bid.isNeighborTo(neighbid,dir) )
	    total++;
    }
    return total;
}


bool HorizonSeedPicker::interpolateSeeds()
{
    BinID dir;
    if ( !lineTrackDirection(dir) ) return false;

    const int step = dir.inl ? dir.inl : dir.crl;

    const int nrseeds = seedlist_.size();
    if ( nrseeds<2 ) 
	return true;

    int sortval[nrseeds], sortidx[nrseeds]; 

    for ( int idx=0; idx<nrseeds; idx++ )
    {
	const BinID seedbid = SI().transform( seedpos_[idx] );
	sortval[idx] = dir.inl ? seedbid.inl : seedbid.crl;
	sortidx[idx] = idx;
    }	
    sort_coupled( sortval, sortidx, nrseeds ); 
    
    EM::EMObject* emobj = EM::EMM().getObject( tracker_.objectID() );  

    for ( int vtx=0; vtx<nrseeds-1; vtx++ ) 
    { 
	const int diff = sortval[vtx+1] - sortval[vtx]; 
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


CubeSampling HorizonSeedPicker::getTrackBox() const
{
    CubeSampling trackbox( true );
    trackbox.hrg.init( false );
    for ( int idx=0; idx<seedpos_.size(); idx++ )
    {
	const BinID seedbid = SI().transform( seedpos_[idx] );
	if ( engine().activeVolume().hrg.includes(seedbid) )
	    trackbox.hrg.include( seedbid );
    }

    for ( int idx=0; idx<trackbounds_.size(); idx++ )
	trackbox.hrg.include( trackbounds_[idx] );
    
    return trackbox;
}


}; // namespace MPE

