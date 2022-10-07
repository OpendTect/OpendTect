#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "algomod.h"
#include "math2.h"


template<class VT,class IT>
mClass(Algo) DeSpiker
{
public:
			DeSpiker( VT maxgrubbs=VT(5) )
			    : maxgrubbs_(maxgrubbs)		{}
    virtual		~DeSpiker()				{}

    inline bool		hasSpike(const VT*,IT sz) const;
			//!< returns whether there is at least one spike
    inline bool		deSpike(VT*,IT sz) const;
			//!< fixes all spikes. returns whether any change made


    inline IT		getSpikeIdx(const VT*,IT sz,VT* newvalptr=0) const;
			//!< returns index of first spike. if provided, will
			//!< fill newvalptr with suggested replacement value

    const VT		maxgrubbs_;

};


template <class VT,class IT>
inline IT DeSpiker<VT,IT>::getSpikeIdx( const VT* vals, IT sz,
					VT* newvalptr ) const
{
    if ( sz < 5 )
	return -1;

    VT maxval = vals[0]; VT minval = maxval, sum = maxval;
    IT minidx = 0, maxidx = 0;
    for ( IT isamp=1; isamp<sz; isamp++ )
    {
	const VT val = vals[isamp];
	if ( maxval < val )
	    { maxidx = isamp; maxval = val; }
	if ( minval > val )
	    { minidx = isamp; minval = val; }
	sum += val;
    }
    if ( maxval == minval )
	return -1;

    const VT avg = sum / sz;
    sum = 0;
    for ( IT isamp=0; isamp<sz; isamp++ )
    {
	const VT delta = avg - vals[isamp];
	sum += delta * delta;
    }
    const VT stdev = Math::Sqrt( sum / sz );

    const VT diffmin = avg - minval;
    const VT diffmax = maxval - avg;
    const bool testingmin = diffmin > diffmax;
    const VT maxdiff = testingmin ? diffmin : diffmax;
    const VT grubbsval = maxdiff / stdev;
    if ( grubbsval < maxgrubbs_ )
	return -1;

    const IT targetidx = testingmin ? minidx : maxidx;
    if ( newvalptr )
    {
	IT previdx = targetidx - 1; IT nextidx = targetidx + 1;
	if ( previdx < 0 )
	    previdx = 1;
	if ( nextidx >= sz )
	    nextidx = sz-2;
	*newvalptr = avg;
    }

    return targetidx;
}


template <class VT,class IT>
inline bool DeSpiker<VT,IT>::hasSpike( const VT* vals, IT sz ) const
{
    return getSpikeIdx( vals, sz ) >= 0;
}


template <class VT,class IT>
inline bool DeSpiker<VT,IT>::deSpike( VT* vals, IT sz ) const
{
    float newval;
    bool hadspikes = false;
    for ( IT idx=0; idx<500 /* Max nr to ensure no infinite loops*/; idx++ )
    {
	const IT spikeidx = getSpikeIdx( vals, sz, &newval );
	if ( spikeidx < 0 )
	    break;

	hadspikes = true;
	vals[spikeidx] = newval;
    }

    return hadspikes;
}
