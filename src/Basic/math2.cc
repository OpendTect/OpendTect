/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2008
-*/

static const char* rcsID = "$Id: math2.cc,v 1.2 2008-04-10 08:31:48 cvsnanne Exp $";

#include "math2.h"
#include "undefval.h"

#include <math.h>
#ifndef __win__
# include <unistd.h>
# ifdef sun5
#  include <ieeefp.h>
# endif
#else
# include <float.h>
#endif

#define mTYPE float
#include "math2_inc.h"
#undef mTYPE
#define mTYPE double
#include "math2_inc.h"
#undef mTYPE

float Math::Exp( float s )
{
    static const float maxval = log( MAXFLOAT );
    return s < maxval ? exp( s ) : mUdf(float);
}

double Math::Exp( double s )
{
    static const double maxval = log( MAXDOUBLE );
    return s < maxval ? exp( s ) : mUdf(double);
}
