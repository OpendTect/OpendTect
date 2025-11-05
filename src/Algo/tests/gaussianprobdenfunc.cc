/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"

#include "gaussianprobdenfunc.h"
#include "linear.h"
#include "statrand.h"
#include "statruncalc.h"



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

    BufferString desc( "2D gaussian PDF cross-correlation" );
    desc.addNewLine().add( "Corr coeff=" ).add( ls2d.corrcoeff )
	.add( ", expected " ).add( mCorrCoeff ).add( " +/- " ).add( cctol );

    Stats::CalcSetup csu;
    csu.require( Stats::Average ).require( Stats::StdDev );
    Stats::RunCalc<float> x0rc( csu ); Stats::RunCalc<float> x1rc( csu );
    x0rc.addValues( mNrPts2Draw, x0 ); x1rc.addValues( mNrPts2Draw, x1 );
    desc.addNewLine().add( "X0: exp=" ).add( pdf.averagePos(0) )
	.add( " std=" ).add( pdf.stddevPos(0) ).add( " => avg=" )
	.add( x0rc.average() ).add( " std=" ).add( x0rc.stdDev() )
	.addNewLine().add( "X1: exp=" ).add( pdf.averagePos(1) )
	.add( " std=" ).add( pdf.stddevPos(1) ).add( " => avg=" )
	.add( x1rc.average() ).add( " std=" ).add( x1rc.stdDev() );

    mRunStandardTest( mIsEqual(ls2d.corrcoeff,mCorrCoeff,cctol), desc );

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    return test2DPDF() ? 0 : 1;
}
