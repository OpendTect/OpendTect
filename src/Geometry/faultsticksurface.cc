/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "faultsticksurface.h"
#include "binidvalue.h"
#include <math.h>

namespace Geometry
{

    
FaultStickSurface::FaultStickSurface()
    : sticksvertical_(true)
{}


bool FaultStickSurface::insertStick( const Coord3& firstpos, 
				     const Coord3& editnormal, int sticknr,
				     int firstcol )
{
    if ( !editnormal.isDefined() || mIsZero(editnormal.sqAbs(),mDefEps) )
	return false;

    const Coord3 normvec = editnormal.normalize();
    const bool newstickvert = fabs(normvec.z) < 0.5;

    if ( sticks_.isEmpty() )
	sticksvertical_ = newstickvert;

    if ( newstickvert != sticksvertical_ )
	return false;

    return FaultStickSet::insertStick(firstpos, editnormal, sticknr, firstcol);
}


bool FaultStickSurface::areSticksVertical() const
{
    return sticksvertical_;
}


} // namespace Geometry
