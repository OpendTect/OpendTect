/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Sep 2005
___________________________________________________________________

-*/

static const char* rcsID = "$Id: horizon3dseedpicker.cc,v 1.10 2006-04-12 12:50:42 cvsjaap Exp $";

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
    didchecksupport_ = hor->geometry.checkSupport(false);
    return true;
}

	
bool HorizonSeedPicker::addSeed(const Coord3& seedcrd )
{
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

    return retrackOnActiveLine( seedbid, startwasdefined );
}


bool HorizonSeedPicker::removeSeed( const EM::PosID& pid, bool retrack )
{
    if ( blockpicking_ ) 
	return true;
    
    EM::EMObject* emobj = EM::EMM().getObject( tracker_.objectID() );  
    BinID seedbid = SI().transform( emobj->getPos(pid) );

    if ( nrLateralConnections(seedbid) == 0 )
	emobj->unSetPos( pid, true );

    seedlist_.erase(); seedpos_.erase(); 

    return retrackOnActiveLine( seedbid, true, !retrack );
}


bool HorizonSeedPicker::reTrack()
{
    seedlist_.erase(); seedpos_.erase(); 
    
    return retrackOnActiveLine( BinID(-1,-1), false );
}


bool HorizonSeedPicker::retrackOnActiveLine( BinID startbid, 
					     bool startwasdefined,
					     bool eraseonly ) 
{
    BinID dir = engine().activeVolume().hrg.step;
    if ( engine().activeVolume().nrCrl() == 1 )
	dir.crl = 0;
    else
	dir.inl = 0;

    bool wholeline = false;
    if ( !engine().activeVolume().hrg.includes(startbid) )
    {
	wholeline = true;
	startbid = engine().activeVolume().hrg.start - dir;
	startwasdefined = false;
    }
    
    TypeSet<EM::PosID> candidatejunctionpairs;

    extendSeedListEraseInBetween( wholeline, startbid, startwasdefined, -dir, 
				  candidatejunctionpairs );
    extendSeedListEraseInBetween( wholeline, startbid, startwasdefined,  dir, 
				  candidatejunctionpairs );
    if ( eraseonly )
       return true;

    if ( !retrackFromSeedList() )
	return false;

    EM::EMObject* emobj = EM::EMM().getObject( tracker_.objectID() );  

    for ( int idx=0; idx<candidatejunctionpairs.size(); idx+=2 )
    {
	if ( emobj->isDefined(candidatejunctionpairs[idx]) )
	{
	    emobj->setPosAttrib( candidatejunctionpairs[idx+1], 
				 EM::EMObject::sSeedNode, true );
	}
    }
    return true;
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
	const EM::PosID prevpid = curpid;
	const bool prevdefined = curdefined;
	
    	curbid += dir ;
	if ( !engine().activeVolume().hrg.includes(curbid) )
	    return;
	curpid = EM::PosID( emobj->id(), sectionid_, curbid.getSerialized() ); 
	curdefined = emobj->isDefined(curpid);

	if ( emobj->isPosAttrib( curpid, EM::EMObject::sSeedNode ) )
	{
	    if  ( wholeline || seedconmode_!=TrackFromSeeds )
	    {
		seedlist_ += curpid;
		seedpos_ += emobj->getPos( curpid );
	    }
	    if ( !wholeline )
		return;
	    
	    continue;
	}
	
	if ( !wholeline && !prevdefined && curdefined )
	{
	    if  ( seedconmode_!=TrackFromSeeds )
	    {
		seedlist_ += curpid;
		seedpos_ += emobj->getPos( curpid );
	    }
	    candidatejunctionpairs += prevpid;
	    candidatejunctionpairs += curpid;
	    return;
	}

	if ( curdefined ) 
	    emobj->unSetPos( curpid, true );
    }
}


bool HorizonSeedPicker::retrackFromSeedList()
{
    if ( !seedlist_.size() )
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

	    if ( seedconmode_ == TrackBetweenSeeds )
		autotrk->setTrackBoundary( getSeedBox() );
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

int HorizonSeedPicker::isMinimumNrOfSeeds() const
{ return seedconmode_==TrackFromSeeds ? 1 : 0 ; }


bool HorizonSeedPicker::doesModeUseVolume() const
{ return seedconmode_==TrackFromSeeds; }


bool HorizonSeedPicker::doesModeUseSetup() const
{ return seedconmode_!=DrawBetweenSeeds; }


bool HorizonSeedPicker::stopSeedPick( bool iscancel )
{
    mGetHorizon(hor);
    hor->geometry.checkSupport( didchecksupport_ );
    return true;
}


int HorizonSeedPicker::nrLateralConnections( const BinID& seedbid ) const
{
    EM::EMObject* emobj = EM::EMM().getObject( tracker_.objectID() );  

    BinID dir = engine().activeVolume().hrg.step;
    if ( engine().activeVolume().nrInl() == 1 )
	dir.crl = 0;
    else
	dir.inl = 0;
    
    int nrlatcon(0);
    BinID neighbid = seedbid - dir;
    
    for ( int cnt=0; cnt<2; cnt++ );
    {
	EM::PosID neighpid = EM::PosID( emobj->id(), sectionid_, 
					neighbid.getSerialized() ); 
	if ( emobj->isDefined(neighpid) )
	    nrlatcon++;
	
    	neighbid = seedbid + dir;
    }

    return nrlatcon;
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

    const int nrseeds = seedlist_.size();
    if ( nrseeds<2 ) 
	return true;

    int sortval[nrseeds], sortidx[nrseeds]; 

    for ( int idx=0; idx<nrseeds; idx++ )
    {
	const BinID seedbid = SI().transform( seedpos_[idx] );
	sortval[idx] = inlineactive ? seedbid.crl : seedbid.inl;
	sortidx[idx] = idx;
    }	
    sort_coupled( sortval, sortidx, nrseeds ); 
    
    EM::EMObject* emobj = EM::EMM().getObject( tracker_.objectID() );  

    for ( int vtx=0; vtx<nrseeds-1; vtx++ ) 
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
    for ( int idx=0; idx<seedpos_.size(); idx++ )
    {
	const BinID seedbid = SI().transform( seedpos_[idx] );
	if ( engine().activeVolume().hrg.includes(seedbid) )
	    seedbox.hrg.include( seedbid );
    }
    return seedbox;
}


}; // namespace MPE

