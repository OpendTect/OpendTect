/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2008
-*/

static const char* rcsID = "$Id: math2.cc,v 1.6 2008-10-22 11:32:39 cvsnanne Exp $";

#include "math2.h"
#include "undefval.h"

#include <float.h>
#include <limits.h>
#include <math.h>

#ifndef __win__
# include <unistd.h>
# ifdef sun5
#  include <ieeefp.h>
# endif
#endif

#define mTYPE float
#include "math2_inc.h"
#undef mTYPE
#define mTYPE double
#include "math2_inc.h"
#undef mTYPE

float Math::Exp( float s )
{
    static const float maxval = logf( MAXFLOAT );
    return s < maxval ? expf( s ) : mUdf(float);
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
