/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2007
-*/


#include "od_iostream.h"
#include "linear.h"
#include "polyfit.h"
#include "trigonometry.h"
#include "testprog.h"


static bool test2DRegresson( const float* xvals, const float* yvals, int sz )
{
#   define mDoCompare( val, target, eps ) \
        mRunStandardTestWithError( mIsEqual(val,target,eps), \
                        #val " value", "outside range" )
    LinStats2D ls2d;
    ls2d.use( xvals, yvals, sz );
    if ( !quiet )
	od_cout() << "a0=" << ls2d.lp.a0_ << " ax=" << ls2d.lp.ax_ << od_endl;
    mDoCompare( ls2d.lp.a0_, 9.3f, 0.3f );
    mDoCompare( ls2d.lp.ax_, 8.5f, 0.7f );

    ls2d.use( yvals, xvals, sz );
    if ( !quiet )
	od_cout() << "a0=" << ls2d.lp.a0_ << " ax=" << ls2d.lp.ax_ << od_endl;
    const float ax = 1.0f / ls2d.lp.ax_;
    const float a0 = -ls2d.lp.a0_ / ls2d.lp.ax_;
    if ( !quiet )
	od_cout() << "inv a0=" << a0 << " ax=" << ax << od_endl;
    mDoCompare( a0, 9.3f, 0.3f );
    mDoCompare( ax, 8.5f, 0.7f );

    return true;
}


static bool testPlaneFit( const TypeSet<Coord3>& coords )
{
    Plane3DFit fitter;
    Plane3 plane;
    fitter.compute( coords, plane );
    if ( !quiet )
	od_cout() << "Plane: " << plane.A_ << "*X + "
	          << plane.B_ << "*Y + "
	          << plane.C_ << "*Z + "
	          << plane.D_ << od_endl;
    return true;
}


static bool testPolyFits( const float* x, const float* y, int sz )
{
    const auto res2 = polyFit( x, y, sz, 2 );
    if ( !quiet )
	od_cout() << "Poly Fit order 2: " << res2[0]
		  << " + " << res2[1] << "*X"
		  << " + " << res2[2] << "*X^2" << od_endl;
    const auto res3 = polyFit( x, y, sz, 3 );
    if ( !quiet )
	od_cout() << "Poly Fit order 3: " << res3[0]
		  << " + " << res3[1] << "*X"
		  << " + " << res3[2] << "*X^2"
		  << " + " << res3[3] << "*X^3" << od_endl;
    // order 2: 8.89870453 + 11.75393295*X + -3.60866618*X^2
    // order 3: 9.05188751 + 10.3282423*X + -0.35720029*X^2 + -2.07160878*X^3
    mRunStandardTest( res2[1] > 11.7539 && res2[1] < 11.754, "Poly Fit" )
    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    TypeSet<float> xvals, yvals, zvals;
    xvals += 0.1f; yvals += 10.f; zvals += 100.f;
    xvals += 0.15f; yvals += 10.4f; zvals += 110.f;
    xvals += 0.2f; yvals += 11.f; zvals += 101.f;
    xvals += 0.25f; yvals += 12.f; zvals += 111.f;
    xvals += 0.35f; yvals += 13.f; zvals += 135.f;
    xvals += 0.55f; yvals += 14.f; zvals += 125.f;
    xvals += 0.6f; yvals += 13.f; zvals += 138.f;
    xvals += 0.7f; yvals += 17.f; zvals += 140.f;
    xvals += 0.9f; yvals += 17.f; zvals += 140.f;
    xvals += 0.92f; yvals += 16.f; zvals += 122.f;

    if ( !test2DRegresson(xvals.arr(),yvals.arr(),xvals.size()) )
	return 1;
    if ( !testPolyFits(xvals.arr(),yvals.arr(),xvals.size()) )
	return 1;

    TypeSet<Coord3> coords;
    for ( int idx=0; idx<xvals.size(); idx++ )
	coords += Coord3( xvals[idx], yvals[idx], zvals[idx] );

    if ( !testPlaneFit(coords) )
	return 1;

    return 0;
}
