/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2007
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "gaussianprobdenfunc.h"

#include "od_iostream.h"
#include "linear.h"
#include "statrand.h"
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
    const float tol = 500.0f / mNrPts2Draw;

    const bool failed = ls2d.corrcoeff < mCorrCoeff-tol
		     || ls2d.corrcoeff > mCorrCoeff+tol;

    if ( failed )
	od_cout() << "Failed. ";

    if ( !quiet || failed )
	od_cout() << "Corr coeff=" << ls2d.corrcoeff
	    	  << ", expected " << mCorrCoeff << " +/- " << tol << od_endl;

    return !failed;
}


int main( int argc, char** argv )
{
    mInitTestProg();

    Stats::randGen().init();

    if ( !test2DPDF() )
	ExitProgram( 1 );

    ExitProgram( 0 );
}

