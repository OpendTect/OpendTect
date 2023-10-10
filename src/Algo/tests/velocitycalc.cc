/*+
________________________________________________________________________

 Copyright:	(C) 1995-2023 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"

#include "velocitycalc.h"
#include "zdomain.h"
#include "zvalseriesimpl.h"

static BufferString msg_;

static const double t0[] = { 0., 6., 0., 6., 0., 6., 0., 6. };
static const double z0[] = { 0., 6000., -200., 5800.,
			     0., 6997.176152, -200., 6797.176152 };


static bool setMsg( const char* fn, double v0, double dv, double srd,
		    float zval, bool zistime, double calculated =mUdf(double),
		    double expected = mUdf(double) )
{
    msg_.set( "Computation failed for " ).add( fn )
	.add( " with:\n v0: " ).add( v0 ).add( " m/s" ).addNewLine()
	.add( " dv: " ).add( dv ).add( " /s" ).addNewLine()
	.add( " srd: " ).add( srd ).add( zistime ? " s\n" : " m\n" )
	.add( " at: " ).add( zval ).add( zistime ? " s" : " m" );
    if ( !mIsUdf(calculated) )
    {
	msg_.addNewLine().add( "Expected depth: " )
	    .add( toStringPrecise( float(expected)) ) \
	    .add( zistime ? " m" : " s" ).addNewLine()
	    .add( "Calculated depth: " ) \
	    .add( toStringPrecise( float(calculated) ) ) \
	    .add( zistime ? " m" : " s" );
    }

    return false;
}


static bool testLinearT2D( double v0, double dv, double srd,
			   const double* times, const double* depths, int& idx )
{
    const double time = times[idx];
    const double expecteddepth = depths[idx] + srd;
    idx++;

    double depth;
    const ArrayZValues<double> timesarr( (double*)&time, 1, ZDomain::TWT() );
    ArrayValueSeries<double,double> depthsarr( &depth, false, 1 );

    mRunStandardTestWithError( Vel::calcDepthsFromLinearV0k(v0,dv,timesarr,
							    depthsarr)
	    ? true : setMsg( "testLinearT2D", v0, dv, srd, time, true ),
	    "Time-to-depth with linear transformation", msg_ );

    mRunStandardTestWithError( float(depth) == float(expecteddepth) ? true
	    : setMsg( "testLinearT2D", v0, dv, srd, time, true, depth,
		      expecteddepth ),
	    "Time-to-depth with linear value", msg_ );

    return true;
}


static bool testLinearD2T( double v0, double dv, double srd,
			   const double* depths, const double* times, int& idx )
{
    const double depth = depths[idx] + srd;
    const double expectedtime = times[idx];
    idx++;

    double time;
    const ArrayZValues depthsarr( (double*)&depth, 1, ZDomain::DepthMeter() );
    ArrayValueSeries<double,double> timesarr( &time, false, 1 );

    mRunStandardTestWithError( Vel::calcTimesFromLinearV0k(v0,dv,depthsarr,
							   timesarr)
	    ? true : setMsg( "testLinearD2T", v0, dv, srd, depth, false ),
	    "Depth-to-time with linear transformation", msg_ );

    mRunStandardTestWithError( mIsEqual(time,expectedtime,1e-6) ? true
	    : setMsg( "testLinearD2T", v0, dv, srd, depth, false, time,
			expectedtime ),
	    "Depth-to-time with linear value", msg_ );

    return true;
}


static bool testLinearT2D( double v0 )
{
    const double dv0 = 0;
    const double dv1 = 0.1;
    const double srd0 = 0.;
    const double srd1 = 200.f;
    int idx = 0;
    if ( !testLinearT2D(v0,dv0,srd0,t0,z0,idx) ||
	 !testLinearT2D(v0,dv0,srd0,t0,z0,idx) ||
	 !testLinearT2D(v0,dv0,srd1,t0,z0,idx) ||
	 !testLinearT2D(v0,dv0,srd1,t0,z0,idx) ||
	 !testLinearT2D(v0,dv1,srd0,t0,z0,idx) ||
	 !testLinearT2D(v0,dv1,srd0,t0,z0,idx) ||
	 !testLinearT2D(v0,dv1,srd1,t0,z0,idx) ||
	 !testLinearT2D(v0,dv1,srd1,t0,z0,idx) )
	return false;

    return true;
}


static bool testLinearD2T( double v0 )
{
    const double dv0 = 0;
    const double dv1 = 0.1;
    const double srd0 = 0.;
    const double srd1 = 200.f;
    int idx = 0;
    if ( !testLinearD2T(v0,dv0,srd0,z0,t0,idx) ||
	 !testLinearD2T(v0,dv0,srd0,z0,t0,idx) ||
	 !testLinearD2T(v0,dv0,srd1,z0,t0,idx) ||
	 !testLinearD2T(v0,dv0,srd1,z0,t0,idx) ||
	 !testLinearD2T(v0,dv1,srd0,z0,t0,idx) ||
	 !testLinearD2T(v0,dv1,srd0,z0,t0,idx) ||
	 !testLinearD2T(v0,dv1,srd1,z0,t0,idx) ||
	 !testLinearD2T(v0,dv1,srd1,z0,t0,idx) )
	return false;

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    const double v0 = 2000.f;
    if ( !testLinearT2D(v0) ||
	 !testLinearD2T(v0) )
	return 1;

    return 0;
}
