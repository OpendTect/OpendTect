/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2007
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "simpnumer.h"
#include "commandlineparser.h"
#include "keystrs.h"

#include <iostream>

#define mTest( testname, test ) \
if ( (test)==true ) \
{ \
    if ( !quiet ) \
	std::cout << testname << ": OK\n"; \
} \
else \
{ \
    std::cout << testname << ": Failed\n"; \
    return false; \
} \


#define mTestCommonSI( si1, si2, expected ) \
testname = "computeCommonStepInterval<"; \
testname.add( tp ).add( ">(" #si1 "," #si2 ")" ); \
mTest( testname.buf(),  \
       computeCommonStepInterval( si1, si2 )==expected ) \
testname = "computeCommonStepInterval<"; \
testname.add( tp ).add( ">(" #si2 "," #si1 ")" ); \
mTest( testname.buf(),  \
      computeCommonStepInterval( si2, si1 )==expected )
    
template <class T>
bool testComputeCommonStepInterval( const char* tp, bool quiet )
{
    BufferString testname;
    mTestCommonSI( StepInterval<T>( 2, 7, 2 ), StepInterval<T>( 3, 7, 2 ),
	    	   StepInterval<T>( 2, 7, 1 ) );

    mTestCommonSI( StepInterval<T>( 2, 10, 2 ), StepInterval<T>( 4, 6, 2 ),
	    	   StepInterval<T>( 2, 10, 2 ) );
    return true;
}

#define mTestGCD( si1, si2, expected ) \
mTest( "GCD( " #si1 ", " #si2 ")" , \
    greatestCommonDivisor( si1, si2 )==expected ) \
mTest( "GCD( " #si2 ", " #si1 ")" , \
    greatestCommonDivisor( si2, si1 )==expected )


template <class T>
bool testGCD( bool quiet )
{
    mTestGCD( 10, 4, 2 );
    mTestGCD( 5, 3, 1 );
    mTestGCD( 345, 3492, 3 );
    mTestGCD( 10, 0, 10 );
    
    return true;
}


int main( int argc, char** argv )
{
    od_init_test_program( argc, argv );

    const bool quiet = CommandLineParser().hasKey( sKey::Quiet() );

    if ( !testGCD<int>( quiet ) )
	ExitProgram( 1 );

    if ( !testComputeCommonStepInterval<short>("short", quiet ) ||
         !testComputeCommonStepInterval<int>( "int", quiet ) ||
         !testComputeCommonStepInterval<od_int64>( "od_int64", quiet ) )
	ExitProgram( 1 );

    return ExitProgram( 0 );
}

