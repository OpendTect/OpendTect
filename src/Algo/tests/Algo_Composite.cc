/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION :
-*/


#include "testprog.h"
#undef mInitTestProg
#define mInitTestProg()


#undef mTestMainFnName
#define mTestMainFnName test_main_array2dmatrix
#include "array2dmatrix.cc"

#undef mTestMainFnName
#define mTestMainFnName test_main_arraymath
#include "arraymath.cc"

#undef mTestMainFnName
#define mTestMainFnName test_main_checksum
#include "checksum.cc"

#undef mTestMainFnName
#define mTestMainFnName test_main_contcurvinterpol
#include "contcurvinterpol.cc"

#undef mTestMainFnName
#define mTestMainFnName test_main_datadistributionextracter
#include "datadistributionextracter.cc"

#undef mTestMainFnName
#define mTestMainFnName test_main_fourier
#include "fourier.cc"

#undef mTestMainFnName
#define mTestMainFnName test_main_gaussianprobdenfunc
#include "gaussianprobdenfunc.cc"

#undef mTestMainFnName
#define mTestMainFnName test_main_regression
#include "regression.cc"

#undef mTestMainFnName
#define mTestMainFnName test_main_simpnumer
#include "simpnumer.cc"

#undef mTestMainFnName
#define mTestMainFnName test_main_sorting
#include "sorting.cc"

#undef mTestMainFnName
#define mTestMainFnName test_main_trigonometry
#include "trigonometry.cc"

#undef mTestMainFnName
#define mTestMainFnName test_main_velocitycalc
#include "velocitycalc.cc"


int testMain( int argc, char** argv )
{
    mInitCompositeTestProg( Algo );

    mRunSubTest( array2dmatrix );
    mRunSubTest( arraymath );
    mRunSubTest( checksum );
    mRunSubTest( contcurvinterpol );
    mRunSubTest( datadistributionextracter );
    mRunSubTest( fourier );
    mRunSubTest( gaussianprobdenfunc );
    mRunSubTest( regression );
    mRunSubTest( simpnumer );
    mRunSubTest( sorting );
    mRunSubTest( trigonometry );
    mRunSubTest( velocitycalc );

    return 0;
}
