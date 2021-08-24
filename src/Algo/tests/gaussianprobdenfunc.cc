/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2007
-*/


#include "gaussianprobdenfunc.h"

#include "od_iostream.h"
#include "linear.h"
#include "statrand.h"
#include "statruncalc.h"
#include "testprog.h"


#define mNrPts2Draw 100000
#define mCorrCoeff 0.6f

static bool test2DPDF()
{
    Gaussian2DProbDenFunc pdf;
    pdf.exp0_ = 50; pdf.std0_ = 10;
    pdf.exp1_ = 1000; pdf.std1_ = 100;
    pdf.cc_ = mCorrCoeff;

    pdf.prepareRandDrawing();

    float x0[mNrPts2Draw]; float x1[mNrPts2Draw];
    for ( int idx=0; idx<mNrPts2Draw; idx++ )
	pdf.drawRandomPos( x0[idx], x1[idx] );

    LinStats2D ls2d;
    ls2d.use( x0, x1, mNrPts2Draw );
    const float cctol = 1e3f / mNrPts2Draw;

    const bool failed = ls2d.corrcoeff < mCorrCoeff-cctol
		     || ls2d.corrcoeff > mCorrCoeff+cctol;

    if ( failed )
	od_cout() << "Failed: ";

    if ( !quiet_ || failed )
    {
	od_cout() << "Corr coeff=" << ls2d.corrcoeff
		  << ", expected " << mCorrCoeff << " +/- " << cctol << od_endl;

	Stats::CalcSetup csu;
	csu.require( Stats::Average ).require( Stats::StdDev );
	Stats::RunCalc<float> x0rc( csu ); Stats::RunCalc<float> x1rc( csu );
	x0rc.addValues( mNrPts2Draw, x0 ); x1rc.addValues( mNrPts2Draw, x1 );
	od_cout() << "X0: exp=" << pdf.exp0_ << " std=" << pdf.std0_
		  << " => avg=" << x0rc.average() << " std=" << x0rc.stdDev();
	od_cout() << "\nX1: exp=" << pdf.exp1_ << " std=" << pdf.std1_
		  << " => avg=" << x1rc.average() << " std=" << x1rc.stdDev();
	od_cout() << od_endl;
    }

    return !failed;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    return test2DPDF() ? 0 : 1;
}
