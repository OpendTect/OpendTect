/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : J.C. Glas
 * DATE     : September 2007
-*/

static const char* rcsID = "$Id$";

#include "faultsticksurface.h"
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
