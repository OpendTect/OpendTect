/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "atomic.h"
#include "envvars.h"
#include "hiddenparam.h"
#include "statruncalc.h"
#include "statrand.h"
#include "settings.h"


static HiddenParam<Stats::NormalRandGen,std::normal_distribution<double>*>
		hp_normalrandgendis( nullptr );


Threads::Atomic<int> partsortglobalseed( 0 );


mDefineNameSpaceEnumUtils(Stats,Type,"Statistic type")
{
	"Count",
	"Average", "Median", "RMS",
	"StdDev", "Variance", "NormVariance",
	"Min", "Max", "Extreme",
	"Sum", "SquareSum",
	"MostFrequent",
	nullptr
};

mDefineNameSpaceEnumUtils(Stats,UpscaleType,"Upscale type")
{
	"Take Nearest Sample",
	"Use Average", "Use Median", "Use RMS", "Use Most Frequent",
	nullptr
};


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


void Stats::RandGen::seedRandom()
{
    std::seed_seq seed{rd_(), rd_(), rd_(), rd_(), rd_(), rd_(), rd_(), rd_()};
    gen_ = std::mt19937( seed );
    gen64_ = std::mt19937_64( seed );
}


void Stats::RandGen::seedFromInt( int seed )
{
    gen_ = std::mt19937( seed );
    gen64_ = std::mt19937_64( seed );
}


Stats::RandGen::RandGen()
{
    seedRandom();
}


double Stats::RandGen::get() const
{
    static std::uniform_real_distribution<double> dis( 0., 1. );
    return dis( gen64_ );
}


int Stats::RandGen::getInt() const
{
    return gen_();
}


int Stats::RandGen::getInt( int min, int max ) const
{
    if ( min == max )
	return min;
    else if ( min > max )
	std::swap( min, max );

    std::uniform_int_distribution<int> dis( min, max );
    return dis( gen_ );
}


int Stats::RandGen::getIndex( int sz ) const
{
    if ( sz < 2 )
	return 0;

    std::uniform_int_distribution<int> dis( 0, sz-1 );
    return dis( gen_ );
}


od_int64 Stats::RandGen::getIndex( od_int64 sz ) const
{
    if ( sz < 2 )
	return 0;

    std::uniform_int_distribution<od_int64> dis( 0, sz-1 );
    return dis( gen64_ );
}


bool Stats::RandGen::setSeed( int seed )
{
    if ( seed == 0 || mIsUdf(seed) )
    {
	pErrMsg( "The seed should be a non-zero, defined value" );
	return false;
    }

    seedval_ = seed;
    seedFromInt( seed );
    return true;
}


void Stats::RandGen::clearSeed()
{
    seedval_ = mUdf(int);
    seedRandom();
}


bool Stats::RandGen::init( int seed )
{
    return setSeed( seed );
}


Stats::RandGen& Stats::randGen()
{
    mDefineStaticLocalObject( PtrMan<Stats::RandGen>, rgptr,
			      = new Stats::RandGen() );
    return *rgptr.ptr();
}



void Stats::NormalRandGen::seedRandom()
{
    std::seed_seq seed{rd_(), rd_(), rd_(), rd_(), rd_(), rd_(), rd_(), rd_()};
    gen64_ = std::mt19937_64( seed );
    *hp_normalrandgendis.getParam( this )
				= std::normal_distribution<double>( 0., 1. );
}


void Stats::NormalRandGen::seedFromInt( int seed )
{
    gen64_ = std::mt19937_64( seed );
    *hp_normalrandgendis.getParam( this )
				= std::normal_distribution<double>( 0., 1. );
}


Stats::NormalRandGen::NormalRandGen()
{
    hp_normalrandgendis.setParam( this,
			new std::normal_distribution<double>( 0., 1. ) );
    seedRandom();
}


Stats::NormalRandGen::~NormalRandGen()
{
    hp_normalrandgendis.removeAndDeleteParam( this );
}


double Stats::NormalRandGen::get() const
{
    return (*hp_normalrandgendis.getParam( this ))( gen64_ );
}


float Stats::NormalRandGen::get( float exp, float s ) const
{
    return (float)(exp + get() * s);
}


double Stats::NormalRandGen::get( double exp, double s ) const
{
    return exp + get() * s;
}


bool Stats::NormalRandGen::setSeed( int seed )
{
    if ( seed == 0 || mIsUdf(seed) )
    {
	pErrMsg( "The seed should be a non-zero, defined value" );
	return false;
    }

    seedval_ = seed;
    seedFromInt( seed );
    return true;
}


void Stats::NormalRandGen::clearSeed()
{
    seedval_ = mUdf(int);
    seedRandom();
}


bool Stats::NormalRandGen::init( int seed )
{
    return setSeed( seed );
}
