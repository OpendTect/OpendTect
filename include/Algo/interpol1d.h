#ifndef interpol1d_h
#define interpol1d_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert & Kris
 Date:		Mar 2006
 RCS:		$Id$
________________________________________________________________________

*/

#include "undefval.h"

/*\brief Interpolation for regular and irregular sampling.

  You have to supply the values 'just around' the position.
  In regular sampling, the sample values are spaced in an equidistant manner.

  When you have undefs, there may be a utility which behaves as follows:
  * When the nearest sample value is undefined, return undefined
  * When another value is undef, use a value from a sample near that sample
    and continue the interpolation.

  The position where the interpolation is done is needed.
  A 'float x' must be provided which will be 0 <= x <= 1 in almost all
  cases.

  When things become a bit difficult, the parameters for the interpolation can
  be calculated up front. Then, you'll find a class with an 'apply' method.
  The position for apply is usually between v0 and v1, but doesn't
  _need_ to be. In that case, 0 < pos < 1. For the undef handlign classes and
  functions, this is _required_.

  */

namespace Interpolate
{

/*!>
 Linear interpolation as usual.
*/

template <class T>
inline T linearReg1D( T v0, T v1, float x )
{
    return x * v1 + (1-x) * v0;
}


/*!>
 Linear interpolation as usual with standard undef handling.
*/

template <class T>
inline T linearReg1DWithUdf( T v0, T v1, float x )
{
    if ( mIsUdf(v0) )
	return x < 0.5 ? mUdf(T) : v1;
    if ( mIsUdf(v1) )
	return x >= 0.5 ? mUdf(T) : v0;

    return linearReg1D( v0, v1, x );
}


/*!>
 Interpolate linearly when two points are known.
 Make sure these points are not at the same posistion (crash!).
*/

template <class T>
inline T linear1D( float x0, T v0, float x1, T v1, float x )
{
    return v0 + (x-x0) * (v1-v0) / (x1-x0);
} 

/*!> Same as above, use when iT is from int family */
template <class iT>
inline iT linear1Di( float x0, iT v0, float x1, iT v1, float x )
{
    const float tmp = v0 + (x-x0) * (v1-v0) / (x1-x0);
    return mRounded( iT, tmp );
} 



/*!<\brief Interpolate 1D regularly sampled, using a 3rd order polynome. */

template <class T>
class PolyReg1D
{
public:

PolyReg1D() {}

PolyReg1D( const T* v )
{
    set( v[0], v[1], v[2], v[3] );
}

PolyReg1D( T vm1, T v0, T v1, T v2 )
{
    set( vm1, v0, v1, v2 );
}

inline void set( T vm1, T v0, T v1, T v2 )
{
    a_[0] = v0;
    a_[1] = v1 - (( 2*vm1 + 3*v0 + v2 ) / 6);
    a_[2] = (( v1 + vm1 ) / 2) - v0;
    a_[3] = (( v1 - vm1 ) / 2) - a_[1];
}

inline T apply( float x ) const
{
    const float xsq = x * x;
    return xsq * x * a_[3] + xsq * a_[2] + x * a_[1] + a_[0];
}

    T	a_[4];
};


template <class T>
inline T polyReg1D( T vm1, T v0, T v1, T v2, float x )
{
    return PolyReg1D<T>(vm1,v0,v1,v2).apply( x );
}


/*!
  \ingroup Algo
  \brief PolyReg1D which smoothly handles undefined values

  Note that this class _requires_ x to be between 0 and 1 for correct undef
  handling. Correct means: if the nearest sample is undefined, return
  undefined. Otherwise always return a value.
*/

template <class T>
class PolyReg1DWithUdf
{
public:

PolyReg1DWithUdf()	{}

PolyReg1DWithUdf( const T* v )
{
    set( v[0], v[1], v[2], v[3] );
}

PolyReg1DWithUdf( T vm1, T v0, T v1, T v2 )
{
    set( vm1, v0, v1, v2 );
}

inline void set( T vm1, T v0, T v1, T v2 )
{
    v0udf_ = mIsUdf(v0); v1udf_ = mIsUdf(v1);
    if ( v0udf_ && v1udf_ ) return;

    if ( mIsUdf(vm1) ) vm1 = v0udf_ ? v1 : v0;
    if ( mIsUdf(v2) ) v2 = v1udf_ ? v0 : v1;
    if ( v0udf_ ) v0 = vm1;
    if ( v1udf_ ) v1 = v2;

    intp_.set( vm1, v0, v1, v2 );
}

inline T apply( float x ) const
{
    if ( (v0udf_ && x < 0.5) || (v1udf_ && x >= 0.5) )
	return mUdf(T);
    return intp_.apply( x );
}

    PolyReg1D<T>	intp_;
    bool		v0udf_;
    bool		v1udf_;

};

template <class T>
inline T polyReg1DWithUdf( T vm1, T v0, T v1, T v2, float x )
{
    return PolyReg1DWithUdf<T>(vm1,v0,v1,v2).apply( x );
}


/*!>
 Interpolate when 3 points are known.
 Make sure none of the positions are the same. Will just crash 'silently'.
 No undefined values allowed.
*/

template <class T>
inline T parabolic1D( float x0, T v0, float x1, T v1, float x2, T v2, float x )
{
    float xx0 = x - x0, xx1 = x-x1, xx2 = x-x2;
    return 	v0 * xx1 * xx2 / ((x0 - x1) * (x0 - x2)) +
		v1 * xx0 * xx2 / ((x1 - x0) * (x1 - x2)) +
		v2 * xx0 * xx1 / ((x2 - x0) * (x2 - x1));
}


/*!>
 Interpolate when 4 points are known.
 Make sure none of the positions are the same. Will just crash 'silently'.
 No undefined values allowed.
*/

template <class T>
inline T poly1D( float x0, T v0, float x1, T v1, float x2, T v2, 
		 float x3, T v3, float x )
{
    float xx0 = x - x0, xx1 = x-x1, xx2 = x-x2, xx3 = x-x3;
    return 	v0 * xx1 * xx2 * xx3 / ((x0 - x1) * (x0 - x2) * (x0 - x3)) +
		v1 * xx0 * xx2 * xx3 / ((x1 - x0) * (x1 - x2) * (x1 - x3)) +
		v2 * xx0 * xx1 * xx3 / ((x2 - x0) * (x2 - x1) * (x2 - x3)) +
		v3 * xx0 * xx1 * xx2 / ((x3 - x0) * (x3 - x1) * (x3 - x2));
}


/*!>
 Predict at sample position 0 when two previous and two next are known.
 Returned is the value of the 3rd order polynome that goes through the points.
*/

template <class T>
inline T predictAtZero1D( T vm2, T vm1, T v1, T v2 )
{
    return (-2 * vm2 + 8 * vm1 + 8 * v1 - 2 * v2) / 12;
}


/*!>
 Predict at sample position 0 when three previous and three next are known.
 Returned is the value of the 5th order polynome that goes through the points.
*/

template <class T>
inline T predictAtZero1D( T vm3, T vm2, T vm1, T v1, T v2, T v3 )
{
    return (vm3 - 6 * vm2 + 15 * vm1 + 15 * v1 - 6 * v2 + v3) / 20;
}


} // namespace Interpolate

#endif
