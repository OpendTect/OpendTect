/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2007
-*/


#include "velocitycalc.h"
#include "testprog.h"

#define mErrMsg( fn, baseentity ) \
od_cout() << "Computation failed for " #fn " with:\n v0: " << v0 << "\ndv: " \
	  << dv << "\n v0depth: " << v0depth << "\n at: " << baseentity << "\n"

bool testLinearT2D( double v0, double dv, double v0depth, float time,
		    float expecteddepth )
{
    float res;
    if ( !computeLinearT2D( v0, dv, v0depth, SamplingData<float>(time,0),
			    1, &res ))
    {
	mErrMsg( testLinearT2D, time );
	return false;
    }

    if ( !mIsEqual( res, expecteddepth,  1e-3 ))
    {
	mErrMsg( testLinearT2D, time );
	od_cout() << "Expected depth: " << expecteddepth << "\n";
	od_cout() << "Computed depth: " << res << "\n";
	return false;
    }

    return true;
}


bool testLinearD2T( double v0, double dv, double v0depth, float depth,
		    float expectedtime )
{
    float res;
    if ( !computeLinearD2T( v0, dv, v0depth, SamplingData<float>(depth,0),
			   1, &res ))
    {
	mErrMsg( testLinearD2T, depth );
	return false;
    }

    if ( !mIsEqual( res, expectedtime,  1e-3 ))
    {
	mErrMsg( testLinearD2T, depth );
	od_cout() << "Expected time: " << expectedtime << "\n";
	od_cout() << "Computed time: " << res << "\n";
	return false;
    }

    return true;
}



int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testLinearT2D( 2000, 0, 0, 0, 0) ||
	    !testLinearT2D( 2000, 0, 0, 6, 6000) ||
	    !testLinearT2D( 2000, 0, -200, 0, -200) ||
	    !testLinearT2D( 2000, 0, -200, 6, 5800) ||
	    !testLinearT2D( 2000, 0.1, 0, 0, 0) ||
	    !testLinearT2D( 2000, 0.1, 0, 6, 6997.176) ||
	    !testLinearT2D( 2000, 0.1, -200, 0, -200) ||
	    !testLinearT2D( 2000, 0.1, -200, 6, 6727.204) )
	return 1;

    if ( !testLinearD2T( 2000, 0, 0, 0, 0) ||
	    !testLinearD2T( 2000, 0, 0, 6000, 6) ||
	    !testLinearD2T( 2000, 0, -200, -200, 0) ||
	    !testLinearD2T( 2000, 0, -200, 5800, 6) ||
	    !testLinearD2T( 2000, 0.1, 0, 0, 0) ||
	    !testLinearD2T( 2000, 0.1, 0, 6997.176, 6) ||
	    !testLinearD2T( 2000, 0.1, -200, -200, 0) ||
	    !testLinearD2T( 2000, 0.1, -200, 6727.204, 6) )
	return 1;

    return 0;
}
