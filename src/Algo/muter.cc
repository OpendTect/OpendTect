/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "muter.h"

#include "valseries.h"
#include <math.h>


Muter::Muter( float taperlen, bool tail )
    : taperlen_(taperlen)
    , tail_(tail)
{
}


Muter::~Muter()
{
}


void Muter::mute( ValueSeries<float>& arr, int sz, float pos ) const
{
    if ( tail_ )
	tailMute( arr, sz, pos );
    else
	topMute( arr, sz, pos );
}


void Muter::topMute( ValueSeries<float>& arr, int sz, float pos ) const
{
    int endidx = pos < 0 ? (int)pos - 1 : (int)pos;
    if ( endidx > sz-1 ) endidx = sz - 1;
    for ( int idx=0; idx<=endidx; idx++ )
	arr.setValue( idx, 0 );

    float endpos = pos + taperlen_;
    if ( endpos <= 0 ) return;

    int startidx = endidx + 1;
    if ( startidx<0 ) startidx = 0;
    endidx = (int)endpos; if ( endidx > sz-1 ) endidx = sz - 1;
    for ( int idx=startidx; idx<=endidx; idx++ )
    {
	float relpos = (idx-pos) / taperlen_;
	arr.setValue( idx,
		    (float) ( arr[idx] * 0.5 * ( 1 - cos(M_PI * relpos) ) ) );
    }
}


void Muter::tailMute( ValueSeries<float>& arr, int sz, float pos ) const
{
    if ( pos < 0 )
    {
	for ( int idx=0; idx<sz; idx++ )
	    arr.setValue( idx, 0.f );
	return;
    }

    int endidx = (int)pos;
    if ( endidx > sz-1 )
	endidx = sz - 1;

    float endpos = pos + taperlen_;
    if ( endpos <= 0 )
	return;

    int startidx = endidx + 1;
    if ( startidx<0 ) startidx = 0;
    endidx = (int)endpos; if ( endidx > sz-1 ) endidx = sz - 1;
    for ( int idx=startidx; idx<=endidx; idx++ )
    {
	float relpos = 1-((idx-pos) / taperlen_);
	arr.setValue( idx,
		    (float) ( arr[idx] * 0.5 * ( 1 - cos(M_PI * relpos) ) ) );
    }

    for ( int idx=endidx+1; idx<sz; idx++ )
	arr.setValue( idx, 0.f );
}


void Muter::muteIntervalsPos( const TypeSet< Interval<float> >& itvs,
				TypeSet< Interval<float> >& muteitvs,
				const SamplingData<double>& sd )
{
    for ( int idx=0; idx<itvs.size(); idx++ )
    {
	muteitvs += itvs[idx];
	muteitvs[idx].start_ = mutePos( itvs[idx].start_, sd );
	muteitvs[idx].stop_ = mutePos( itvs[idx].stop_, sd );
    }
}


void Muter::muteIntervals( ValueSeries<float>& arr, int sz,
			   const TypeSet< Interval<float> >& muteitvs ) const
{
    //TODO assumes intervals sorted. What if not ??

    for ( int idx=0; idx<muteitvs.size(); idx++ )
    {
	const Interval<float> itv = muteitvs[idx];

	float pos0 = itv.start_;
	float pos1 = itv.stop_;

	if ( mIsUdf( pos0 ) &&  mIsUdf( pos1 ) )
	    continue;

	if ( mIsUdf( pos1 ) || mIsUdf( pos0 ) )
	{
	    if ( !tail_ && idx == 0 )
		topMute( arr, sz, pos0 );
	    else if ( tail_ && idx == muteitvs.size()-1 )
		tailMute( arr, sz, pos0 );
	}
	else
	    itvMute( arr, sz, muteitvs[idx] );
    }
}


void Muter::itvMute(ValueSeries<float>& arr, int sz, Interval<float> itv ) const
{
    itv.sort( true );

    const float pos0 = itv.start_;
    const float pos1 = itv.stop_;

    if ( mIsUdf( pos0 ) ||  mIsUdf( pos1 ) )
	return;

    int startidx = pos0 < 0 ? (int)pos0 - 1 : (int)pos0;
    if ( startidx < 0 ) startidx = 0;
    int endidx = pos1 < 0 ? (int)pos1 - 1 : (int)pos1;
    if ( endidx > sz-1 ) endidx = sz - 1;
    for ( int idx=startidx; idx<=endidx; idx++ )
	arr.setValue( idx, 0 );

    float endpos = pos1 + taperlen_;
    if ( endpos <= 0 ) return;

    startidx = endidx + 1;
    if ( startidx<0 ) startidx = 0;
    endidx = (int)endpos; if ( endidx > sz-1 ) endidx = sz - 1;
    for ( int idx=startidx; idx<=endidx; idx++ )
    {
	float relpos = (idx-pos1) / taperlen_;
	arr.setValue( idx,
		    (float) ( arr[idx] * 0.5 * ( 1 - cos(M_PI * relpos) ) ) );
    }
}
