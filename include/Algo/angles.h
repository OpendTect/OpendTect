#ifndef angles_h
#define angles_h

/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Mar 2009
 Contents:	Angle functions
 RCS:		$Id: angles.h,v 1.1 2009-03-30 12:38:51 cvsbert Exp $
________________________________________________________________________

*/

#include "gendefs.h"
#include <math.h>

#ifndef M_PI
# define M_PI           3.14159265358979323846
#endif

#ifndef M_PIl
# define M_PIl          3.1415926535897932384626433832795029L
#endif


inline double deg2rad( int deg )
{
    static double deg2radconst = M_PI / 180;
    return deg * deg2radconst;
}


inline float deg2rad( float deg )
{
    static float deg2radconst = M_PI / 180;
    return deg * deg2radconst;
}


inline double deg2rad( double deg )
{
    static double deg2radconst = M_PI / 180;
    return deg * deg2radconst;
}


inline long double deg2rad( long double deg )
{
    static long double deg2radconst = M_PIl / 180;
    return deg * deg2radconst;
}


inline float rad2deg( float rad )
{
    static float rad2degconst = 180 / M_PI;
    return rad * rad2degconst;
}


inline double rad2deg( double rad )
{
    static double rad2degconst = 180 / M_PI;
    return rad * rad2degconst;
}


inline long double rad2deg( long double rad )
{
    static long double rad2degconst = 180 / M_PIl;
    return rad * rad2degconst;
}


template <class T>
inline T deg2usrdeg( T deg )
{
    T usrdeg = 90 - deg;
    while ( usrdeg >= 360 ) usrdeg -= 360;
    while ( usrdeg < 0 ) usrdeg += 360;
    return usrdeg;
}


template <class T>
inline T rad2usrdeg( T rad )
{
    return deg2usrdeg( rad2deg(rad) );
}


#endif
