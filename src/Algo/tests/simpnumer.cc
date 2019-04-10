/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2007
-*/


#include "simpnumer.h"
#include "terrain3x3.h"
#include "testprog.h"

#undef mTest
#define mTest( testname, test ) \
if ( (test)==true ) \
{ \
    handleTestResult( true, testname ); \
} \
else \
{ \
    handleTestResult( false, testname ); \
    return false; \
} \


#define mTestCommonSI( si1, si2, expected ) \
testname = "getCommonStepInterval<"; \
testname.add( tp ).add( ">(" #si1 "," #si2 ")" ); \
mTest( testname.buf(),  \
       getCommonStepInterval( si1, si2 )==expected ) \
testname = "getCommonStepInterval<"; \
testname.add( tp ).add( ">(" #si2 "," #si1 ")" ); \
mTest( testname.buf(),  \
      getCommonStepInterval( si2, si1 )==expected )

template <class T>
bool testComputeCommonStepInterval( const char* tp )
{
    BufferString testname;
    mTestCommonSI( StepInterval<T>( 2, 7, 2 ), StepInterval<T>( 3, 7, 2 ),
		   StepInterval<T>( 2, 7, 1 ) );

    mTestCommonSI( StepInterval<T>( 2, 10, 2 ), StepInterval<T>( 4, 6, 2 ),
		   StepInterval<T>( 2, 10, 2 ) );
    return true;
}


bool testTerrain3x3()
{
    /*
    Terrain3x3<float> t3x3( 25.f );
    float vals[9];
    vals[0] = vals[3] = vals[6] = 50.f;
    vals[1] = vals[4] = vals[7] = 25.f;
    vals[2] = vals[5] = vals[8] = 0.f;
    vals[3] = 25.0f; vals[4] = 0.f;
    t3x3.set( vals );
    od_cout() << "val at -12.5=" << t3x3.valueAt( -12.5f, 0.f ) << od_endl;
    od_cout() << "slope=" << t3x3.slope() << od_endl;
    od_cout() << "direction=" << t3x3.direction() << od_endl;
    od_cout() << "profileCurvature=" << t3x3.profileCurvature() << od_endl;
    od_cout() << "planformCurvature=" << t3x3.planformCurvature() << od_endl;
    */
    return true;
}

#define mTestGCD( si1, si2, expected ) \
mTest( "GCD( " #si1 ", " #si2 ")" , \
    greatestCommonDivisor( si1, si2 )==expected ) \
mTest( "GCD( " #si2 ", " #si1 ")" , \
    greatestCommonDivisor( si2, si1 )==expected )


template <class T>
bool testGCD()
{
    mTestGCD( 10, 4, 2 );
    mTestGCD( 5, 3, 1 );
    mTestGCD( 345, 3492, 3 );
    mTestGCD( 10, 0, 10 );

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();


    if ( !testGCD<int>() )
	return 1;

    if ( !testComputeCommonStepInterval<short>("short" )
      || !testComputeCommonStepInterval<int>( "int" )
      || !testComputeCommonStepInterval<od_int64>( "od_int64" ) )
	return 1;

    if ( !testTerrain3x3() )
	return 1;

    return 0;
}
