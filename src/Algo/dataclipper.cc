/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: dataclipper.cc,v 1.3 2002-03-12 16:00:14 bert Exp $";


#include "dataclipper.h"
#include "stats.h"
#include "sorting.h"


DataClipper::DataClipper( float ncr )
    : sampleprob( 1 )
    , subselect( false )
    , cliprate( ncr )
{
    Stat_initRandom( 0 );
} 


void DataClipper::setApproxNrValues( int n, int statsz )
{
    sampleprob = ((float) statsz) / n;

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
	for ( int idx=0; idx<nrvals; idx++ )
	{
	    double rand = Stat_getRandom();
	    if ( rand > sampleprob )
		continue;

	    samples += vals[idx];
	}
    }
    else
    {
	for ( int idx=0; idx<nrvals; idx++ ) samples += vals[idx];
    }
}


void DataClipper::calculateRange()
{
    int nrvals = samples.size();
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
