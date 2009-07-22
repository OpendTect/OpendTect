/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2008
-*/

static const char* rcsID = "$Id: math2.cc,v 1.8 2009-07-22 16:01:31 cvsbert Exp $";

#include "math2.h"
#include "undefval.h"

#include <float.h>
#include <limits.h>
#include <math.h>

#ifndef __win__
# include <unistd.h>
#endif
#ifdef sun5
# define mFloatLogFn log
# define mFloatExpFn exp
# include <ieeefp.h>
#else
# define mFloatLogFn logf
# define mFloatExpFn expf
#endif

#define mTYPE float
#include "math2_inc.h"
#undef mTYPE
#define mTYPE double
#include "math2_inc.h"
#undef mTYPE

float Math::Exp( float s )
{
    static const float maxval = mFloatLogFn( MAXFLOAT );
    return s < maxval ? mFloatExpFn( s ) : mUdf(float);
}

double Math::Exp( double s )
{
    static const double maxval = log( MAXDOUBLE );
    return s < maxval ? exp( s ) : mUdf(double);
}


int Math::LCMOf( int num1, int num2 )
{
    if ( num1 <= 0 || num2 <= 0 ) return 0;

    for ( int idx=1; idx<=num2/2; idx++ )
    {
	const int prod = num1 * idx;
	if ( !(prod%num2) ) return prod;
    }

    return num1 * num2;
}
