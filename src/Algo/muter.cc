/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id$";

#include "muter.h"

#include "valseries.h"
#include "errh.h"
#include <math.h>


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
	arr.setValue( idx, arr[idx] * 0.5 * ( 1 - cos(M_PI * relpos) ) );
    }
}


void Muter::tailMute( ValueSeries<float>& arr, int sz, float pos ) const
{
    int endidx = pos < 0 ? (int)pos - 1 : (int)pos;
    if ( endidx > sz-1 ) endidx = sz - 1;

    float endpos = pos + taperlen_;
    if ( endpos <= 0 ) return;

    int startidx = endidx + 1;
    if ( startidx<0 ) startidx = 0;
    endidx = (int)endpos; if ( endidx > sz-1 ) endidx = sz - 1;
    for ( int idx=startidx; idx<=endidx; idx++ )
    {
	float relpos = 1-((idx-pos) / taperlen_);
	arr.setValue( idx, arr[idx] * 0.5 * ( 1 - cos(M_PI * relpos) ) );
    }

    for ( int idx=endidx+1; idx<sz; idx++ )
	arr.setValue( idx, 0 );
}
