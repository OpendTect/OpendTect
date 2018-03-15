#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert & Kris
 Date:		Mar 2006
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

/*!\brief Linear interpolation as usual. */

template <class PosT,class ValT>
inline ValT linearReg1D( ValT v0, ValT v1, PosT x )
{
    return x * v1 + (1-x) * v0;
}


/*!\brief Linear interpolation as usual with standard undef handling. */

template <class PosT,class ValT>
inline ValT linearReg1DWithUdf( ValT v0, ValT v1, PosT x )
{
    if ( mIsUdf(v0) )
	return x <= (PosT)(0.5) ? mUdf(ValT) : v1;
    if ( mIsUdf(v1) )
	return x > (PosT)(0.5) ? mUdf(ValT) : v0;

    return linearReg1D( v0, v1, x );
}


/*!\brief Interpolate linearly when two points are known.
 Make sure these points are not at the same posistion (crash!).
*/

template <class PosT,class ValT>
inline ValT linear1D( PosT x0, ValT v0, PosT x1, ValT v1, PosT x )
{
    return v0 + (x-x0) * (v1-v0) / (x1-x0);
}

/*!> Same as above, use when iT is from int family */
template <class PosT,class iT>
inline iT linear1Di( PosT x0, iT v0, PosT x1, iT v1, PosT x )
{
    const double tmp = v0 + (x-x0) * (v1-v0) / (x1-x0);
    return mRounded( iT, tmp );
}



/*!<\brief Interpolate 1D regularly sampled, using a 3rd order polynome. */

template <class ValT>
mClass(Algo) PolyReg1D
{
public:

PolyReg1D()
{
    set( (ValT)0, (ValT)0, (ValT)0, (ValT)0 );
}

PolyReg1D( const ValT* v )
{
    set( v[0], v[1], v[2], v[3] );
}

PolyReg1D( ValT vm1, ValT v0, ValT v1, ValT v2 )
{
    set( vm1, v0, v1, v2 );
}

inline void set( ValT vm1, ValT v0, ValT v1, ValT v2 )
{
    a_[0] = v0;
    a_[1] = v1 - (( 2*vm1 + 3*v0 + v2 ) / 6);
    a_[2] = (( v1 + vm1 ) / 2) - v0;
    a_[3] = (( v1 - vm1 ) / 2) - a_[1];
}

template <class PosT>
inline ValT apply( PosT x ) const
{
    const PosT xsq = x * x;
    return xsq * x * a_[3] + xsq * a_[2] + x * a_[1] + a_[0];
}

    ValT    a_[4];
};


template <class PosT,class ValT>
inline ValT polyReg1D( ValT vm1, ValT v0, ValT v1, ValT v2, PosT x )
{
    return PolyReg1D<ValT>(vm1,v0,v1,v2).apply( x );
}


/*!
\brief PolyReg1D which smoothly handles undefined values

  Note that this class _requires_ x to be between 0 and 1 for correct undef
  handling. Correct means: if the nearest sample is undefined, return
  undefined. Otherwise always return a value.
*/

template <class ValT>
mClass(Algo) PolyReg1DWithUdf
{
public:

PolyReg1DWithUdf()
{ set ( (ValT)0, (ValT)0, (ValT)0, (ValT)0 ); }

PolyReg1DWithUdf( const ValT* v )
{
    set( v[0], v[1], v[2], v[3] );
}

PolyReg1DWithUdf( ValT vm1, ValT v0, ValT v1, ValT v2 )
{
    set( vm1, v0, v1, v2 );
}

inline void set( ValT vm1, ValT v0, ValT v1, ValT v2 )
{
    v0udf_ = mIsUdf(v0); v1udf_ = mIsUdf(v1);
    if ( v0udf_ && v1udf_ ) return;

    if ( mIsUdf(vm1) ) vm1 = v0udf_ ? v1 : v0;
    if ( mIsUdf(v2) ) v2 = v1udf_ ? v0 : v1;
    if ( v0udf_ ) v0 = vm1;
    if ( v1udf_ ) v1 = v2;

    intp_.set( vm1, v0, v1, v2 );
}

template <class PosT>
inline ValT apply( PosT x ) const
{
    if ( (v0udf_ && x < 0.5) || (v1udf_ && x >= 0.5) )
	return mUdf(ValT);
    return intp_.apply( x );
}

    PolyReg1D<ValT>	intp_;
    bool		v0udf_;
    bool		v1udf_;

};

template <class PosT,class ValT>
inline ValT polyReg1DWithUdf( ValT vm1, ValT v0, ValT v1, ValT v2, PosT x )
{
    return PolyReg1DWithUdf<ValT>(vm1,v0,v1,v2).apply( x );
}


/*!>
 Interpolate when 3 points are known.
 Make sure none of the positions are the same. Will just crash 'silently'.
 No undefined values allowed.
*/

template <class PosT,class ValT>
inline ValT parabolic1D( PosT x0, ValT v0, PosT x1, ValT v1, PosT x2, ValT v2,
			 PosT x )
{
    PosT xx0 = x - x0, xx1 = x-x1, xx2 = x-x2;
    return	v0 * xx1 * xx2 / ((x0 - x1) * (x0 - x2)) +
		v1 * xx0 * xx2 / ((x1 - x0) * (x1 - x2)) +
		v2 * xx0 * xx1 / ((x2 - x0) * (x2 - x1));
}


/*!>
 Interpolate when 4 points are known.
 Make sure none of the positions are the same. Will just crash 'silently'.
 No undefined values allowed.
*/

template <class PosT,class ValT>
inline ValT poly1D( PosT x0, ValT v0, PosT x1, ValT v1, PosT x2, ValT v2,
		    PosT x3, ValT v3, PosT x )
{
    PosT xx0 = x - x0, xx1 = x-x1, xx2 = x-x2, xx3 = x-x3;
    return	v0 * xx1 * xx2 * xx3 / ((x0 - x1) * (x0 - x2) * (x0 - x3)) +
		v1 * xx0 * xx2 * xx3 / ((x1 - x0) * (x1 - x2) * (x1 - x3)) +
		v2 * xx0 * xx1 * xx3 / ((x2 - x0) * (x2 - x1) * (x2 - x3)) +
		v3 * xx0 * xx1 * xx2 / ((x3 - x0) * (x3 - x1) * (x3 - x2));
}


/*!>
 Predict at sample position 0 when two previous and two next are known.
 Returned is the value of the 3rd order polynome that goes through the points.
*/

template <class ValT>
inline ValT predictAtZero1D( ValT vm2, ValT vm1, ValT v1, ValT v2 )
{
    return (-2 * vm2 + 8 * vm1 + 8 * v1 - 2 * v2) / 12;
}


/*!>
 Predict at sample position 0 when three previous and three next are known.
 Returned is the value of the 5th order polynome that goes through the points.
*/

template <class ValT>
inline ValT predictAtZero1D( ValT vm3, ValT vm2, ValT vm1,
			     ValT v1, ValT v2, ValT v3 )
{
    return (vm3 - 6 * vm2 + 15 * vm1 + 15 * v1 - 6 * v2 + v3) / 20;
}


} // namespace Interpolate
