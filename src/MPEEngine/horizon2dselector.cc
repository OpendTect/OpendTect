/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2006
___________________________________________________________________

-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "horizon2dselector.h"

#include "ptrman.h"
#include "survinfo.h"
#include "emhorizon2d.h"

namespace MPE 
{

Horizon2DSelector::Horizon2DSelector( const EM::Horizon2D& hor,
				      const EM::SectionID& sid )
    : SectionSourceSelector( sid )
    , horizon_( hor )
{}


void Horizon2DSelector::setTrackPlane( const TrackPlane& trackplane )
{
    trackplane_ = trackplane;
}


int Horizon2DSelector::nextStep()
{
    if ( !trackplane_.isVertical() )
	return 0;

    const TrackPlane::TrackMode trackmode = trackplane_.getTrackMode();
    if ( trackmode==TrackPlane::None || trackmode==TrackPlane::Move )
	return 0;

    const bool selectall = trackmode==TrackPlane::ReTrack ||
			   trackmode==TrackPlane::Erase;

    StepInterval<int> inlrg = trackplane_.boundingBox().hrg.inlRange();
    StepInterval<int> crlrg = trackplane_.boundingBox().hrg.crlRange();
    const StepInterval<float >& zrg = trackplane_.boundingBox().zrg;

    inlrg.include( inlrg.start+trackplane_.motion().binid.inl );
    crlrg.include( crlrg.start+trackplane_.motion().binid.crl );

    PtrMan<EM::EMObjectIterator> iterator =
				horizon_.createIterator( sectionid_ );
    while ( iterator )
    {
	const EM::PosID pid = iterator->next();
	if ( pid.objectID()==-1 )
	    break;

	const EM::SubID subid = pid.subID();

	const Coord3 pos = horizon_.getPos( sectionid_, subid );
	const BinID bid = SI().transform( pos );
	if ( !inlrg.includes( bid.inl,true ) ||
	     !crlrg.includes( bid.crl,true ) ||
	     !zrg.includes( pos.z,true ) )
	    continue;

	if ( selectall || horizon_.isAtEdge(pid) )
	    selpos_ += subid;
    }

    return 0;
}


};
