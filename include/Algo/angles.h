#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "gendefs.h"
#include <math.h>

/*
  Converting degrees, radians and user degrees to one another.
  Users (or rather: geologists) have North=0, and then clockwise to E=90,
  S=180s and W=270.
*/

namespace Angle
{

enum Type { Rad, Deg, UsrDeg };
// Generic conversion function see bottom: Angle::convert

template <class T>
T cPI() { return (T)M_PIl; }

template <class T>
inline void getFullCircle( Type typ, T& t )
{
    t = typ == Rad ? 2 * cPI<T>() : 360;
}


//! User degrees are from North, clockwise
template <class T>
inline T deg2usrdeg( T deg ) //Make sure deg is defined, otherwise, dead loop
{
    T usrdeg = 90 - deg;
    while ( usrdeg >= 360 ) usrdeg -= 360;
    while ( usrdeg < 0 ) usrdeg += 360;
    return usrdeg;
}


//! User degrees are from North, clockwise
template <class T>
inline T usrdeg2deg( T udeg )
{
    T deg = 90 - udeg;
    if ( deg < 0 ) deg += 360;
    return deg;
}


template <class T>
inline T rad2usrdeg( T rad )
{
    return deg2usrdeg( Math::toDegrees(rad) );
}


template <class T>
inline T usrdeg2rad( T udeg )
{
    return Math::toRadians( usrdeg2deg(udeg) );
}


template <class T>
inline T convert( Type inptyp, T val, Type outtyp )
{
    if ( inptyp == outtyp || mIsUdf(val) )
	return val;

    switch ( inptyp )
    {
        case Rad:
            val = outtyp == Deg ? Math::toDegrees(val) : rad2usrdeg(val);
            break;
        case Deg:
            val = outtyp == Rad ? Math::toRadians(val) : deg2usrdeg(val);
            break;
        case UsrDeg:
            val = outtyp == Deg ? usrdeg2deg(val) : usrdeg2rad(val);
            break;
    }

    return val;
}

} // namespace Angle
