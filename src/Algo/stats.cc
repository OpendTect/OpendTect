/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : Sep 2006
-*/



#define _CRT_RAND_S

#include "atomic.h"
#include "statruncalc.h"
#include "statrand.h"
#include "timefun.h"
#include "envvars.h"
#include "settings.h"


Threads::Atomic<int> partsortglobalseed( 0 );
Threads::Atomic<int> globalseed( 0 );


mDefineNameSpaceEnumUtils(Stats,Type,"Statistic type")
{
	"Count",
	"Average", "Median", "RMS",
	"StdDev", "Variance", "NormVariance",
	"Min", "Max", "Extreme",
	"Sum", "SquareSum",
	"MostFrequent",
	0
};

mDefineNameSpaceEnumUtils(Stats,UpscaleType,"Upscale type")
{
	"Take Nearest Sample",
	"Use Average", "Use Median", "Use RMS", "Use Most Frequent",
	0
};


#include <math.h>
#include <stdlib.h>


void initSeed( int seed )
{
    // rand_s function on Windows does not use a seed.
#ifndef __win__
    if ( seed == 0 )
    {
	if ( globalseed != 0 )
	    return;

	globalseed = int( Time::getMilliSeconds() );
    }

    globalseed = seed;

    srand48( globalseed );
#endif
}


void Stats::RandomGenerator::init( int seed )
{
    initSeed( seed );
}


Stats::CalcSetup& Stats::CalcSetup::require( Stats::Type t )
{
    if ( t == Stats::Median )
	{ needmed_ = true; needsorted_ = true; return *this; }
    else if ( t == Stats::MostFreq )
	{ needmostfreq_ = true; return *this; }
    else if ( t >= Stats::Min && t <= Stats::Extreme )
	{ needextreme_ = true; return *this; }
    else if ( t == Stats::Variance || t == Stats::StdDev
	    || t == Stats::NormVariance )
	{ needvariance_ = true; needsums_ = true; return *this; }

    needsums_ = true;
    return *this;
}


static int medianhandling = -2;
static Threads::Lock medianhandlinglock;


int Stats::CalcSetup::medianEvenHandling()
{
    Threads::Locker locker( medianhandlinglock );
    if ( medianhandling != -2 ) return medianhandling;

    if ( GetEnvVarYN("OD_EVEN_MEDIAN_AVERAGE") )
	medianhandling = 0;
    else if ( GetEnvVarYN("OD_EVEN_MEDIAN_LOWMID") )
	medianhandling = -1;
    else
    {
	bool yn = false;
	Settings::common().getYN( "dTect.Even Median.Average", yn );
	if ( yn )
	    medianhandling = 0;
	else
	{
	    Settings::common().getYN( "dTect.Even Median.LowMid", yn );
	    medianhandling = yn ? -1 : 1;
	}
    }

    return medianhandling;
}


Stats::RandGen::RandGen()
{
    initSeed( 0 );
}


Stats::RandGen Stats::randGen()
{
    mDefineStaticLocalObject( PtrMan<Stats::RandGen>, rgptr,
			      = new Stats::RandGen() );
    return *rgptr;
}


double Stats::RandGen::get() const
{
#ifdef __win__
    unsigned int rand = 0;
    rand_s( &rand );
    double ret = rand;
    ret /= UINT_MAX;
    return ret;
#else
    return drand48();
#endif
}


int Stats::RandGen::getInt() const
{
#ifdef __win__
    unsigned int rand = 0;
    rand_s( &rand );
    return *((int*)(&rand));
#else
    return (int) lrand48();
#endif
}

int Stats::RandGen::getInt( int min, int max ) const
{
    return getIndex( max-min+1 ) + min;
}

int Stats::RandGen::getIndex( int sz ) const
{
    if ( sz < 2 ) return 0;

    int idx = (int)(sz * get());
    if ( idx < 0 ) idx = 0;
    if ( idx >= sz ) idx = sz-1;

    return idx;
}


int Stats::RandGen::getIndexFast( int sz, int seed ) const
{
    if ( sz < 2 ) return 0;

    int randidx = 1664525u * seed + 1013904223u;
    if ( randidx < 0 ) randidx = -randidx;
    return randidx % sz;
}


od_int64 Stats::RandGen::getIndex( od_int64 sz ) const
{
    if ( sz < 2 ) return 0;

    od_int64 idx = (od_int64)(sz * get());
    if ( idx < 0 ) idx = 0;
    if ( idx >= sz ) idx = sz-1;

    return idx;
}


od_int64 Stats::RandGen::getIndexFast( od_int64 sz, od_int64 seed ) const
{
    if ( sz < 2 ) return 0;

    const int randidx1 = mCast( int, 1664525u * seed + 1013904223u );
    const int randidx2 = mCast(int, 1664525u * (seed+0x12341234) + 1013904223u);
    od_int64 randidx = (((od_int64)randidx1)<<32)|((od_int64)randidx2);
    if ( randidx < 0 ) randidx = -randidx;
    return randidx % sz;
}


Stats::NormalRandGen::NormalRandGen()
    : useothdrawn_(false)
    , othdrawn_(0)
{
    initSeed( 0 );
}


double Stats::NormalRandGen::get() const
{
    double fac, r, v1, v2;
    double arg;

    double retval = othdrawn_;

    if ( !useothdrawn_ )
    {
	do
	{
	    v1 = 2.0 * randGen().get() - 1.0;
	    v2 = 2.0 * randGen().get() - 1.0;
	    r= v1*v1 + v2*v2;
	} while (r >= 1.0 || r <= 0.0);

	arg = (double)(-2.0*log(r) / r);
	fac = Math::Sqrt( arg );
	othdrawn_ = v1 * fac;
	retval = v2 * fac;
    }

    useothdrawn_ = !useothdrawn_;
    return retval;
}


float Stats::NormalRandGen::get( float e, float s ) const
{
    return (float)(e + get() * s);
}


double Stats::NormalRandGen::get( double e, double s ) const
{
    return e + get() * s;
}
