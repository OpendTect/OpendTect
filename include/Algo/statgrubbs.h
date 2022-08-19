#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "algomod.h"
#include "gendefs.h"
#include <math.h>

namespace Stats
{

/*!
\brief Grubbs' test to detect outliers in a univariate data set assumed to come from a normally distributed population.
*/

mExpClass(Algo) Grubbs
{
public:

    template <class T,class SzTp>
    static T	getMax(T*,SzTp sz,SzTp& index_of_max);

};


template <class T,class SzTp>
inline T Stats::Grubbs::getMax( T* arr, SzTp sz, SzTp& idxof )
{
    if ( sz < 3 )
	{ idxof = 0; return sz > 0 ? 0 : mUdf(T); }
    SzTp startidx = 0;
    for ( ; startidx<sz && mIsUdf(arr[startidx]); startidx++ )
	/* just skip undefs at start */;
    if ( startidx+3 > sz )
	{ idxof = startidx; return 0; }

    T maxval = arr[startidx]; T minval = maxval, sum = maxval;
    SzTp minidx = startidx, maxidx = startidx;
    SzTp nonudfsz = 1;
    for ( SzTp idx=startidx+1; idx<sz; idx++ )
    {
	const T val = arr[idx];
	if ( mIsUdf(val) ) continue;

	if ( maxval < val ) { maxidx = idx; maxval = val; }
	if ( minval > val ) { minidx = idx; minval = val; }
	sum += val;
	nonudfsz++;
    }
    if ( maxval == minval || nonudfsz < 3 )
	{ idxof = startidx; return 0; }

    const T avg = sum / nonudfsz;
    sum = 0;
    for ( SzTp idx=startidx; idx<sz; idx++ )
    {
	const T val = arr[idx];
	if ( mIsUdf(val) ) continue;

	const T delta = avg - arr[idx];
	sum += delta * delta;
    }
    const T stdev = Math::Sqrt( sum / nonudfsz );

    const T diffmin = avg - minval;
    const T diffmax = maxval - avg;
    idxof = diffmin > diffmax ? minidx : maxidx;
    return (diffmin > diffmax ? diffmin : diffmax) / stdev;
}

} // namespace Stats
