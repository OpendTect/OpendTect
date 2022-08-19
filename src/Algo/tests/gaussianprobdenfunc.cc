/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

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
    pdf.set( 0, 50.f, 10.f )
       .set( 1, 1000.f, 100.f )
       .setCorrelation( mCorrCoeff );

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
	od_cout() << "X0: exp=" << pdf.averagePos(0)
		  << " std=" << pdf.stddevPos(0)
		  << " => avg=" << x0rc.average() << " std=" << x0rc.stdDev();
	od_cout() << "\nX1: exp=" << pdf.averagePos(1)
		  << " std=" << pdf.stddevPos(1)
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
