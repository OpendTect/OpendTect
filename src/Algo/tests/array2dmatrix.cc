/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "array2dmatrix.h"
#include "arraynddumper.h"

#include "od_iostream.h"
#include "testprog.h"


static bool testMultiply()
{
    // example from:
    // http://www.mathsisfun.com/algebra/matrix-multiplying.html
    Array2DMatrix<float> mat( 1, 3 );
    Array2DMatrix<float> mat2( 3, 4 );
    mat.get(0,0) = 3; mat.get(0,1) = 4; mat.get(0,2) = 2;
    mat2.get(0,0) = 13; mat2.get(0,1) = 9; mat2.get(0,2) = 7; mat2.get(0,3)= 15;
    mat2.get(1,0) = 8; mat2.get(1,1) = 7; mat2.get(1,2) = 4; mat2.get(1,3)= 6;
    mat2.get(2,0) = 6; mat2.get(2,1) = 4; mat2.get(2,2) = 0; mat2.get(2,3)= 3;

    mat.multiply( mat2 );
    Array2DMatrix<float> exp( 1, 4 );
    exp.get(0,0) = 83; exp.get(0,1) = 63; exp.get(0,2) = 37; exp.get(0,3) = 75;
    if ( !mat.isEq(exp.a2d_,0.0001) )
	{ od_cout() << "Failed: Matrix multiply" << od_endl; return false; }

    return true;
}

static bool testCholesky()
{
    Array2DMatrix<float> mat( 3 );
    // example from wikipedia
    mat.get(0,0) = 4;
    mat.get(1,1) = 37;
    mat.get(2,2) = 98;
    mat.get(0,1) = mat.get(1,0) = 12;
    mat.get(0,2) = mat.get(2,0) = -16;
    mat.get(1,2) = mat.get(2,1) = -43;

    Array2DMatrix<float> out( 3 );
    mat.getCholesky( out );

    Array2DMatrix<float> exp( 3 );
    exp.get(0,0) = 2;
    exp.get(1,1) = 1;
    exp.get(2,2) = 3;
    exp.get(1,0) = 6;
    exp.get(2,0) = -8;
    exp.get(2,1) = 5;

    if ( !out.isEq(exp.a2d_,0.0001) )
	{ od_cout() << "Failed: Cholesky decomp (1)" << od_endl; return false; }

    // example cov matrix from:
    // http://www.sitmo.com/article/generating-correlated-random-numbers
    mat.get(0,0) = mat.get(1,1) = mat.get(2,2) = 1;
    mat.get(0,1) = mat.get(1,0) = 0.6f;
    mat.get(0,2) = mat.get(2,0) = 0.3f;
    mat.get(1,2) = mat.get(2,1) = 0.5f;

    mat.getCholesky( out );

    if ( !quiet_ )
    {
	ArrayNDDumper<float> matdmpr( mat.a2d_ );
	od_cout() << "Inp matrix" << od_endl;
	matdmpr.dump( od_cout() );

	od_cout() << "Cholesky matrix" << od_endl;
	ArrayNDDumper<float> outdmpr( out.a2d_ );
	outdmpr.dump( od_cout() );
    }

    exp.get(0,0) = 1;
    exp.get(1,1) = 0.8f;
    exp.get(2,2) = 0.8660254f;
    exp.get(1,0) = 0.6f;
    exp.get(2,0) = 0.3f;
    exp.get(2,1) = 0.4f;
    if ( !out.isEq(exp.a2d_,0.0001) )
	{ od_cout() << "Failed: Cholesky decomp (2)" << od_endl; return false; }

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testMultiply() )
	return 1;
    if ( !testCholesky() )
	return 1;

    return 0;
}
