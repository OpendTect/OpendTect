/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: dataclipper.cc,v 1.9 2003-11-07 12:21:57 bert Exp $";


#include "dataclipper.h"
#include "stats.h"
#include "sorting.h"


DataClipper::DataClipper( float cr0, float cr1 )
    : sampleprob( 1 )
    , subselect( false )
    , cliprate0( cr0 )
    , cliprate1( cr1 )
    , approxstatsize( 2000 )
{
    if ( cliprate1 < 0 )
	cliprate1 = cliprate0;

    Stat_initRandom( 0 );
} 


void DataClipper::setClipRate( float cr0, float cr1 )
{
    cliprate0 = cr0;
    cliprate1 = cr1 < 0 ? cr0 : cr1;
}


void DataClipper::setApproxNrValues( int n, int statsz )
{
    sampleprob = ((float) statsz) / n;
    approxstatsize = statsz;

    sampleprob = mMIN( sampleprob, 1 );
    subselect = true;
}


void DataClipper::putData( float v )
{
    if ( subselect )
    {
	double rand = Stat_getRandom();

	if ( rand > sampleprob )
	    return;
    }

    if ( !mIsUndefined( v ) ) samples += v;
}


void DataClipper::putData( const float* vals, int nrvals )
{
    if ( subselect )
    {
	int nrsamples = approxstatsize-samples.size();
	if ( nrsamples>nrvals )
	{
	    for ( int idx=0; idx<nrvals; idx++ )
	    {
		double rand = Stat_getRandom();
		if ( rand > sampleprob )
		    continue;

		float val =  vals[idx];
		if ( !mIsUndefined( val ) ) samples += val;
	    }
	}
	else
	{
	    for ( int idx=0; idx<nrsamples; idx++ )
	    {
		double rand = Stat_getRandom();
		rand *= (nrvals-1);
		float val =  vals[mNINT(rand)];
		if ( !mIsUndefined( val ) )
		    samples += val;
	    }
	}
    }
    else
    {
	for ( int idx=0; idx<nrvals; idx++ )
	{
	    float val = vals[idx];
	    if ( !mIsUndefined( val ) ) samples += val;
	}
    }
}


void DataClipper::calculateRange()
{
    int nrvals = samples.size();
    if ( !nrvals ) return;

    int firstidx = mNINT(cliprate0*nrvals);
    int topnr = mNINT(cliprate1*nrvals);
    int lastidx = nrvals-topnr-1;

    sortFor( samples.arr(), nrvals, firstidx );
    range.start = samples[firstidx];

    sortFor( samples.arr(), nrvals, lastidx );
    range.stop = samples[lastidx];

    samples.erase();
    subselect = false;
    sampleprob = 1;
}
