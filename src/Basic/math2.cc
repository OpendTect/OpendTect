/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2008
-*/


#include "math2.h"
#include "undefval.h"
#include "bufstring.h"

#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>

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
#define mTYPE long double
#include "math2_inc.h"
#undef mTYPE

#ifndef OD_NO_QT
# include <QString>
#endif


float_complex Math::Sqrt( const float_complex& s )
{
    return sqrt ( s ); //A bit silly bu the space before the parantesis
		       //makes it avoid the sqrt test.
}



unsigned int Math::SetBits( unsigned int curflags, unsigned int mask, bool yn)
{
    if ( yn )
	return curflags | mask;

    return (~mask) & curflags;
}


bool Math::AreBitsSet( unsigned int curflags, unsigned int mask, bool all )
{
    unsigned int res = curflags & mask;
    if ( all )
	return res == mask;

    return res>0;
}


float Math::Exp( float s )
{
    mDefineStaticLocalObject( const float, maxval, = mFloatLogFn(MAXFLOAT) );
    return s < maxval ? mFloatExpFn( s ) : mUdf(float);
}


double Math::Exp( double s )
{
    mDefineStaticLocalObject( const double, maxval, = log(MAXDOUBLE) );
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


int Math::HCFOf( int num1, int num2 )
{
    if ( num1 <= 0 || num2 <= 0 ) return 1;

    int num = num1 > num2 ? num1 : num2;
    int factor = num1 > num2 ? num2 : num1;
    while ( factor > 1 )
    {
	const int remainder = num % factor;
	if ( !(remainder) )
	    return factor;

	num = factor;
	factor = remainder;
    }

    return 1;
}


unsigned int Math::Abs( int val )	{ return abs(val); }
od_uint64 Math::Abs( od_int64 val )	{ return llabs(val); }
long double Math::Abs( long double val ) { return fabs(val); }
double Math::Abs( double val )		{ return fabs(val); }
float Math::Abs( float val )		{ return fabsf(val); }
float Math::Abs( float_complex val )	{ return abs(val); }
double Math::Floor( double val )	{ return floor(val); }
float Math::Floor( float val )		{ return floorf(val); }
double Math::Ceil( double val )		{ return ceil(val); }
float Math::Ceil( float val )		{ return ceilf(val); }


float Math::Atan2( float y, float x )
{
    if ( mIsUdf(x) || mIsUdf(y) || (mIsZero(x,mDefEpsF) && mIsZero(y,mDefEpsF)))
	return mUdf(float);

    return atan2 ( y, x );
}


float Math::Atan2( float_complex val )
{
    return Atan2( val.imag(), val.real() );
}


double Math::Atan2( double y, double x )
{
    if ( mIsUdf(x) || mIsUdf(y) || (mIsZero(x,mDefEps) && mIsZero(y,mDefEps)) )
	return mUdf(double);

    return atan2 ( y, x );
}


int Math::NrSignificantDecimals( double val )
{
    int digits = 10; //double precision
    const int magnitude = mCast(int,Floor( Log10( Abs(val) ) ) ) + 1;
    digits -= magnitude;
    if ( digits < 0 )
	digits = 0;

    BufferString resstr;
#ifdef OD_NO_QT
    char buf[80]; OD::memZero( buf, 80 );
    sprintf( buf, "%.*f", digits, val );
    resstr.set( buf );
#else
    const QString qstr = QString::number( val, 'f', digits );
    resstr.set( qstr );
#endif
    const char* str = resstr.buf();
    const char* ptrdot = firstOcc( str, '.' );
    if ( !ptrdot ) return 0;
    int ret = FixedString(str).size();
    const char* ptrend = str + ret;
    while ( ptrend-- > ptrdot )
    {
	if ( *ptrend != '0' )
	    break;
	ret--;
    }

    return ret-2;
}


static int niceFraction( bool round, double fraction )
{
    int nice_fraction = 0;
    if ( round )
    {
	if ( fraction < 1.5 )
	    nice_fraction = 1;
	else if (fraction < 3)
	    nice_fraction = 2;
	else if (fraction < 7)
	    nice_fraction = 5;
	else
	    nice_fraction = 10;
    }
    else
    {	if (fraction <= 1)
	    nice_fraction = 1;
	else if (fraction <= 2)
	    nice_fraction = 2;
	else if (fraction <= 5)
	    nice_fraction = 5;
	else
	    nice_fraction = 10;
    }
    return nice_fraction;
}


float Math::NiceNumber( float val, bool round )
{
    if ( mIsZero( val, mDefEpsF) )
	return 0.f;

    float signfactor = 1.f;
    if ( val<0.f )
    {
	signfactor = -1.f;
	val = -val;
    }

    const float exponent = Floor( log10( val) );
    const float fraction = val / pow(10.f, exponent);

    return niceFraction(round, fraction) * pow(10.f, exponent) * signfactor;
}


double Math::NiceNumber( double val, bool round )
{
    if ( mIsZero( val, mDefEpsD) )
	return 0.0;

    double signfactor = 1.0;
    if ( val<0.0 )
    {
	signfactor = -1.0;
	val = -val;
    }

    const double exponent = Floor( log10( val) );
    const double fraction = val / pow(10.0, exponent);

    return niceFraction(round, fraction) * pow(10.0, exponent) * signfactor;
}
