/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : Sep 2006
-*/

static const char* rcsID = "$Id: stats.cc,v 1.7 2009-07-22 16:01:29 cvsbert Exp $";

#include "statruncalc.h"
#include "statrand.h"
#include "errh.h"
#include "timefun.h"
#include "envvars.h"
#include "settings.h"

DefineNameSpaceEnumNames(Stats,Type,0,"Statistics types")
{ "Count", "Average", "Median", "StdDev", "Variance", "Min", "Max",
  "MostFrequent", "Sum", "SquareSum", "RMS", "NormVariance", 0 };

int Stats::RandGen::seed_ = 0;

#include <math.h>
#include <stdlib.h>


Stats::RunCalcSetup& Stats::RunCalcSetup::require( Stats::Type t )
{
    if ( t == Stats::Median )
	{ needmed_ = true; return *this; }
    else if ( t == Stats::MostFreq )
	{ needmostfreq_ = true; return *this; }
    else if ( t == Stats::Min || t == Stats::Max )
	{ needextreme_ = true; return *this; }

    needsums_ = true;
    return *this;
}


bool Stats::RunCalcSetup::medianEvenAverage()
{
    static int ret = -1;

    if ( ret < 0 )
    {
	ret = GetEnvVarYN( "OD_EVEN_MEDIAN_AVERAGE" ) ? 1 : 0;
	if ( ret == 0 )
	{
	    bool yn = false;
	    Settings::common().getYN( "dTect.Average even median", yn );
	    ret = yn ? 1 : 0;
	}
    }

    return (bool)ret;
}


double Stats::RandGen::get()
{
#ifdef __win__
    int ran_ = rand();
    int rmax_ = RAND_MAX;

    double ret = ran_;
    ret /= rmax_;
    return ret;
#else
    return drand48();
#endif
}


void Stats::RandGen::init( int seed )
{
    if ( seed == 0 )
    {
	if ( seed_ != 0 )
	    return;
	seed = (int)Time_getMilliSeconds();
    }
    seed_ = seed;

#ifdef __win__
    srand( (unsigned)seed_ );
#else
    srand48( (long)seed_ );
#endif
}


double Stats::RandGen::getNormal( double ex, double sd )
{
    static int iset = 0;
    static double gset = 0;
    double fac, r, v1, v2;
    double arg;

    double retval = gset;
    if ( !iset )
    {
        do 
	{
            v1 = 2.0*get() - 1.0;
            v2 = 2.0*get() - 1.0;
            r=v1*v1+v2*v2;
        } while (r >= 1.0 || r <= 0.0);

	arg = (double)(-2.0*log(r)/r);
        fac = Math::Sqrt( arg );
        gset = v1 * fac;
        retval = v2 * fac;
    }

    iset = !iset;
    return retval*sd + ex;
}


int Stats::RandGen::getIndex( int sz )
{
    if ( sz < 2 ) return 0;

    int idx = (int)(sz * get());
    if ( idx < 0 ) idx = 0;
    if ( idx >= sz ) idx = sz-1;

    return idx;
}
