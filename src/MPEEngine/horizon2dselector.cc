/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2006
___________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

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


int Horizon2DSelector::nextStep()
{
    pErrMsg( "Fix inlrg, crlrg and zrg" );
    StepInterval<int> inlrg;
    StepInterval<int> crlrg;
    const StepInterval<float > zrg;

//    inlrg.include( inlrg.start+trackplane_.motion().inl() );
//    crlrg.include( crlrg.start+trackplane_.motion().crl() );

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
	if ( !inlrg.includes( bid.inl(),true ) ||
	     !crlrg.includes( bid.crl(),true ) ||
	     !zrg.includes( pos.z,true ) )
	    continue;

	if ( horizon_.isAtEdge(pid) )
	    selpos_ += subid;
    }

    return 0;
}

} // namespace MPE
