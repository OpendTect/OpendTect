#ifndef valseriesinterpol_h
#define valseriesinterpol_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril & Kris Tingdahl
 Date:          Mar 2005
 RCS:           $Id: valseriesinterpol.h,v 1.1 2005-03-08 11:55:44 cvsbert Exp $
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
			, period_(1)		{}

    inline T		value(const ValueSeries<T>&,T pos) const;

    bool		vdamine_;
    int			maxidx_;
    T			snapdist_;
    bool		smooth_;
    bool		extrapol_;
    T			udfval_;
    bool		isperiodic_;
    T			period_;

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

    int curidx = mNINT(pos); mChkVSIRg
    if ( !smooth_ || mIsEqual(pos,curidx,snapdist_) )
	return vda.value( curidx );

    T v[4]; const int lopos = (int)pos;
    curidx = lopos - 1;	mChkVSIRg;	v[0] = vda.value( curidx );
    curidx = lopos;	mChkVSIRg;	v[1] = vda.value( curidx );
    curidx = lopos + 1;	mChkVSIRg;	v[2] = vda.value( curidx );
    curidx = lopos + 2;	mChkVSIRg;	v[3] = vda.value( curidx );

    pos -= lopos; // now 0 < pos < 1
    if ( !isperiodic_ )
	return polyInterpolate( v[0], v[1], v[2], v[3], pos );

    pos += 1; // now 1 < pos < 2
    return interpolateYPeriodicSampled( v, 4, pos, period_, false, udfval_ );
}

#undef mChkVSIRg


#endif
