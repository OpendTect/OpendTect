#ifndef valseriesinterpol_h
#define valseriesinterpol_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril & Kris Tingdahl
 Date:          Mar 2005
 RCS:           $Id: valseriesinterpol.h,v 1.8 2012-07-10 08:05:26 cvskris Exp $
________________________________________________________________________

-*/

#include "valseries.h"
#include "periodicvalue.h"

/*\brief interpolates between values of a ValueSeries object

  Note that we assume that the values are equidistant in 'X'.
 
 */

template <class T>
class ValueSeriesInterpolator
{
public:
			ValueSeriesInterpolator( int mxidx=mUdf(int) )
			: maxidx_(mxidx)
			, snapdist_(0)
			, smooth_(true)
			, extrapol_(false)
			, udfval_(mUdf(T))
			, isperiodic_(false)
			, period_(1)
			, linear_(false)
			, hasudfs_(true)	{}

    inline T		value(const ValueSeries<T>&,T pos) const;

    int			maxidx_;
    T			snapdist_;
    T			udfval_;
    bool		smooth_;
    bool		extrapol_;
    bool		isperiodic_;
    bool		hasudfs_;
    T			period_;
    bool		linear_;

};


#define mChkVSIRg \
	{ if ( curidx < 0 ) curidx = 0; \
	  else if ( curidx > maxidx_ ) curidx = maxidx_; }


template <class T>
inline T ValueSeriesInterpolator<T>::value( const ValueSeries<T>& vda,
					    T pos ) const
{
    if ( !extrapol_ && (pos < -snapdist_ || pos > maxidx_ + snapdist_) )
	return udfval_;
    else if ( maxidx_ < 1 )
	return vda.value( 0 );

    int curidx = mNINT32(pos); mChkVSIRg
    if ( !smooth_ || mIsEqual(pos,curidx,snapdist_) )
	return vda.value( curidx );

    T v[4]; const int lopos = (int)pos;
    curidx = lopos - 1;	mChkVSIRg;	v[0] = vda.value( curidx );
    curidx = lopos;	mChkVSIRg;	v[1] = vda.value( curidx );
    curidx = lopos + 1;	mChkVSIRg;	v[2] = vda.value( curidx );
    curidx = lopos + 2;	mChkVSIRg;	v[3] = vda.value( curidx );

    pos -= lopos; // now 0 < pos < 1
    T rv;
    if ( !isperiodic_ )
    {
	if ( linear_ )
	{
	    if ( hasudfs_ && (mIsUdf(v[1]) || mIsUdf(v[2])) )
		rv = pos < 0.5 ? v[1] : v[2];
	    else
		rv = pos*v[2] + (1-pos)*v[1];
	}
	else
	    rv = hasudfs_
	       ? Interpolate::polyReg1DWithUdf( v[0], v[1], v[2], v[3], pos )
	       : Interpolate::polyReg1D( v[0], v[1], v[2], v[3], pos );
    }
    else
    {
	if ( hasudfs_ )
	{
	    if ( mIsUdf(v[0]) ) v[0] = v[1];
	    if ( mIsUdf(v[3]) ) v[3] = v[2];
	    if ( mIsUdf(v[1]) || mIsUdf(v[2]) )
		return udfval_;
	}
	pos += 1; // now 1 < pos < 2
	rv = IdxAble::interpolateYPeriodicReg( v, 4, pos, period_, false  );
    }
    if ( mIsUdf(rv) )
	rv = udfval_;

    return rv;
}

#undef mChkVSIRg


#endif
