/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: dataclipper.cc,v 1.14 2007-01-04 22:36:16 cvskris Exp $";


#include "dataclipper.h"
#include "statrand.h"
#include "sorting.h"
#include "valseries.h"
#include "undefval.h"


DataClipper::DataClipper( float cr0, float cr1 )
    : sampleprob( 1 )
    , subselect( false )
    , cliprate0( cr0 )
    , cliprate1( cr1 )
    , approxstatsize( 2000 )
{
    if ( cliprate1 < 0 )
	cliprate1 = cliprate0;

    Stats::RandGen::init();
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
	double rand = Stats::RandGen::get();

	if ( rand > sampleprob )
	    return;
    }

    if ( !mIsUdf( v ) ) samples += v;
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
		double rand = Stats::RandGen::get();
		if ( rand > sampleprob )
		    continue;

		float val =  vals[idx];
		if ( !mIsUdf( val ) ) samples += val;
	    }
	}
	else
	{
	    for ( int idx=0; idx<nrsamples; idx++ )
	    {
		double rand = Stats::RandGen::get();
		rand *= (nrvals-1);
		float val =  vals[mNINT(rand)];
		if ( !mIsUdf( val ) )
		    samples += val;
	    }
	}
    }
    else
    {
	for ( int idx=0; idx<nrvals; idx++ )
	{
	    float val = vals[idx];
	    if ( !mIsUdf( val ) ) samples += val;
	}
    }
}


void DataClipper::putData( const ValueSeries<float>& vals, int nrvals )
{
    if ( vals.arr() )
    {
	putData( vals.arr(), nrvals );
	return;
    }
	   
    if ( subselect )
    {
	int nrsamples = approxstatsize-samples.size();
	if ( nrsamples>nrvals )
	{
	    for ( int idx=0; idx<nrvals; idx++ )
	    {
		double rand = Stats::RandGen::get();
		if ( rand > sampleprob )
		    continue;

		float val =  vals.value(idx);
		if ( !mIsUdf( val ) ) samples += val;
	    }
	}
	else
	{
	    for ( int idx=0; idx<nrsamples; idx++ )
	    {
		double rand = Stats::RandGen::get();
		rand *= (nrvals-1);
		float val =  vals.value(mNINT(rand));
		if ( !mIsUdf( val ) )
		    samples += val;
	    }
	}
    }
    else
    {
	for ( int idx=0; idx<nrvals; idx++ )
	{
	    float val = vals.value(idx);
	    if ( !mIsUdf( val ) ) samples += val;
	}
    }
}


bool DataClipper::calculateRange()
{
    int nrvals = samples.size();
    if ( !nrvals ) return false;

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
    return true;
}
