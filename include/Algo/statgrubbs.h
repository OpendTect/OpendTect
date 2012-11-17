#ifndef statgrubbs_h
#define statgrubbs_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert/Bruno
 Date:          Feb 2011
 RCS:           $Id$
________________________________________________________________________

-*/

#include "algomod.h"
#include "gendefs.h"
#include <math.h>

namespace Stats
{

mClass(Algo) Grubbs
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

#endif

