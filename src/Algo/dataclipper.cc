/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: dataclipper.cc,v 1.6 2002-08-05 08:56:25 nanne Exp $";


#include "dataclipper.h"
#include "stats.h"
#include "sorting.h"


DataClipper::DataClipper( float ncr )
    : sampleprob( 1 )
    , subselect( false )
    , cliprate( ncr )
    , approxstatsize( 2000 )
{
    Stat_initRandom( 0 );
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

    samples += v;
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

    int firstidx = mNINT(cliprate*nrvals);
    int lastidx = nrvals-firstidx-1;

    sortFor( samples.arr(), nrvals, firstidx );
    range.start = samples[firstidx];

    sortFor( samples.arr(), nrvals, lastidx );
    range.stop = samples[lastidx];

    samples.erase();
    subselect = false;
    sampleprob = 1;
}
