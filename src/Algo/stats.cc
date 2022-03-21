/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : Sep 2006
-*/


#include "atomic.h"
#include "envvars.h"
#include "statruncalc.h"
#include "statrand.h"
#include "settings.h"
#include "timefun.h"


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


namespace Stats
{

bool initSeed( int seed, Threads::Atomic<int>& seedval )
{
    if ( seed == 0 )
    {
	if ( seedval != 0 && !mIsUdf(seedval.load()) )
	    return false;

	seed = Time::getMilliSeconds();
    }

    seedval = seed;
    return true;
}

} // namespace Stats


Stats::RandGen::RandGen()
{
    std::seed_seq seed{rd_(), rd_(), rd_(), rd_(), rd_(), rd_(), rd_(), rd_()};
    gen_ = std::mt19937( seed );
    gen64_ = std::mt19937_64( seed );
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
    std::uniform_int_distribution<int> dis( min, max );
    return dis( gen_ );
}


int Stats::RandGen::getIndex( int sz ) const
{
    if ( sz < 2 )
	return 0;

    std::uniform_int_distribution<int> dis( 0, sz );
    return dis( gen_ );
}


od_int64 Stats::RandGen::getIndex( od_int64 sz ) const
{
    if ( sz < 2 )
	return 0;

    std::uniform_int_distribution<od_int64> dis( 0, sz );
    return dis( gen64_ );
}


bool Stats::RandGen::init( int seed )
{
    if ( seed != 0 && seedval_ != 0 && !mIsUdf(seedval_.load()) )
    {
	pErrMsg("The seed should only be set once per generator");
	return false;
    }
    else if ( !initSeed(seed,seedval_) )
	return false;

    gen_ = std::mt19937( seedval_.load() );
    gen64_ = std::mt19937_64( seedval_.load() );
    return true;
}


Stats::RandGen& Stats::randGen()
{
    mDefineStaticLocalObject( PtrMan<Stats::RandGen>, rgptr,
			      = new Stats::RandGen() );
    return *rgptr.ptr();
}



Stats::NormalRandGen::NormalRandGen()
{
    std::seed_seq seed{rd_(), rd_(), rd_(), rd_(), rd_(), rd_(), rd_(), rd_()};
    gen64_ = std::mt19937_64( seed );
}


double Stats::NormalRandGen::get() const
{
    static std::normal_distribution<double> dis( 0., 1. );
    return dis( gen64_ );
}


float Stats::NormalRandGen::get( float exp, float s ) const
{
    return (float)(exp + get() * s);
}


double Stats::NormalRandGen::get( double exp, double s ) const
{
    return exp + get() * s;
}


bool Stats::NormalRandGen::init( int seed )
{
    if ( !initSeed(seed,seedval_) )
	return false;

    gen64_ = std::mt19937_64( seedval_.load() );
    return true;
}
