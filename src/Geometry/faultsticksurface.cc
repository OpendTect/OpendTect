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
{}


FaultStickSurface::~FaultStickSurface()
{}


FaultStickSurface& FaultStickSurface::operator =( const FaultStickSurface& oth )
{
    if ( &oth == this )
	return *this;

    FaultStickSet::operator =( oth );
    sticksvertical_ = oth.sticksvertical_;
    return *this;
}


bool FaultStickSurface::insertStick( const Coord3& firstpos, 
				     const Coord3& editnormal, int sticknr,
				     int firstcol, const Pos::GeomID& geomid )
{
    if ( !editnormal.isDefined() || mIsZero(editnormal.sqAbs(),mDefEps) )
	return false;

    const Coord3 normvec = editnormal.normalize();
    const bool newstickvert = fabs(normvec.z_) < 0.5;

    if ( sticks_.isEmpty() )
	sticksvertical_ = newstickvert;

    if ( newstickvert != sticksvertical_ )
	return false;

    return FaultStickSet::insertStick(firstpos, editnormal, sticknr, firstcol,
									geomid);
}


bool FaultStickSurface::areSticksVertical() const
{
    return sticksvertical_;
}

} // namespace Geometry
