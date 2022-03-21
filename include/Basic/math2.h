#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Aug 2005
 RCS:		$Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "gendefs.h"
#include "odcomplex.h"

/* Functions with some extra facilities added to math.h

   Each function has a float and a double version. This may be very
   old-fashioned, but who cares.

   At the end, there is also an all-integer x to the power y.

 */


namespace Math
{

/*! Takes curflags, sets/clears the bits in flag, and returns
    the composite value.*/
mGlobal(Basic) unsigned int SetBits( unsigned int curflags,
				     unsigned int mask, bool yn );

/*! Returns wether the bits in the flag are set. If mask has multiple
    bits, the all boolean specifies if all bits are required. */
mGlobal(Basic) bool AreBitsSet( unsigned int curflags,
			        unsigned int mask, bool all=true );

/*!\returns 0 for for infinite, NaN, and that sort of crap */
mGlobal(Basic) bool IsNormalNumber(float);

/*! PowerOf(-2,2) returns -4. This may be mathematically
  incorrect, it delivers continuity with negative numbers */
mGlobal(Basic) float PowerOf(float,float);

/*!Checks the input range before calling asin, and does thus
   avoid nan's due to roundoff errors. */
mGlobal(Basic) float ASin(float);

/*!Checks the input range before calling acos, and does thus
   avoid nan's due to roundoff errors. */
mGlobal(Basic) float ACos(float);

/*!Checks the input range before calling log, returns
   undefined if negative or zero value is given. */
mGlobal(Basic) float Log(float);

/*!Checks the input range before calling log10, returns
   mUdf(float) if negative or zero value is given. */
mGlobal(Basic) float Log10(float);

/*!Checks the input range before calling sqrt, if negative
   value is given, zero is returned. */
mGlobal(Basic) float Sqrt(float);

/*! Zeroth order modified Bessel function of the first kind */
mGlobal(Basic) float BesselI0(float);

mGlobal(Basic) float_complex Sqrt(const float_complex&);

/*!Checks the input range before calling exp, if too large
    value is given, mUdf(float) is returned. */
mGlobal(Basic) float Exp(float);

mGlobal(Basic) float toDB(float);

mGlobal(Basic) inline unsigned int Abs( unsigned int i )    { return i; }
mGlobal(Basic) inline od_uint64 Abs( od_uint64 i )	    { return i; }
mGlobal(Basic) unsigned int Abs(int i);
mGlobal(Basic) od_uint64 Abs(od_int64 i);
mGlobal(Basic) float Abs(float);
mGlobal(Basic) double Abs(double);
mGlobal(Basic) long double Abs(long double);
mGlobal(Basic) float Abs(float_complex);

mGlobal(Basic) float Floor(float);
mGlobal(Basic) double Floor(double);
mGlobal(Basic) float Ceil(float);
mGlobal(Basic) double Ceil(double);

mGlobal(Basic)	float Atan2(float y, float x);
mGlobal(Basic)	double Atan2(double y, double x);
mGlobal(Basic)	float Atan2(float_complex timeval);

inline float	toDegrees(float r)	{ return (mIsUdf(r) ? r : r*mRad2DegF);}
inline double	toDegrees(double r)	{ return (mIsUdf(r) ? r : r*mRad2DegD);}
inline float	toRadians(float d)	{ return (mIsUdf(d) ? d : d*mDeg2RadF);}
inline double	toRadians(double d)	{ return (mIsUdf(d) ? d : d*mDeg2RadD);}

inline float	degFromNorth( float azimuth )
		{
		    const float deg = 90 - mRad2DegF * azimuth;
		    return deg < 0 ? deg + 360 : deg;
		}

mGlobal(Basic) int LCMOf(int,int); /*! <Lowest Common Multiple. */
mGlobal(Basic) int HCFOf(int,int); /*! <Highest Common Factor. */
mGlobal(Basic) bool IsNormalNumber(double);
mGlobal(Basic) double PowerOf(double,double);
mGlobal(Basic) double ASin(double);
mGlobal(Basic) double ACos(double);
mGlobal(Basic) double Log(double);
mGlobal(Basic) double Log10(double);
mGlobal(Basic) double Exp(double);
mGlobal(Basic) double Sqrt(double);
mGlobal(Basic) double toDB(double);
mGlobal(Basic) double BesselI0(double);

mGlobal(Basic) bool IsNormalNumber(long double);
mGlobal(Basic) long double PowerOf(long double,long double);
mGlobal(Basic) long double ASin(long double);
mGlobal(Basic) long double ACos(long double);
mGlobal(Basic) long double Log(long double);
mGlobal(Basic) long double Log10(long double);
mGlobal(Basic) long double Sqrt(long double);
mGlobal(Basic) long double toDB(long double);
mGlobal(Basic) long double BesselI0(long double);

template <class iT,class iPOW> inline
iT IntPowerOf( iT i, iPOW p )
{
    iT ret = 1;
    while ( p )
    {
	if ( p > 0 )
	    { ret *= i; p--; }
	else
	    { ret /= i; p++; }
    }
    return ret;
}

mGlobal(Basic) long double IntPowerOf(long double,int);
mGlobal(Basic) double IntPowerOf(double,int);
mGlobal(Basic) float IntPowerOf(float,int);
mGlobal(Basic) int NrSignificantDecimals(double);


mGlobal(Basic) float NiceNumber(float, bool round=true);
mGlobal(Basic) double NiceNumber(double, bool round=true);

} // namespace Math


