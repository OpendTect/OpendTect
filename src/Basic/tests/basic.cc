/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION : 
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "commandlineparser.h"
#include "commandlineparser.h"
#include "keystrs.h"

#include <iostream>

#define mTest( testname, test ) \
if ( (test)==true ) \
{ \
    if ( !quiet ) \
    { \
        std::cout << testname << ": OK\n"; \
    } \
} \
else \
{ \
    std::cout << testname << ": Failed\n"; \
    return false; \
}


bool testPointerCast( bool quiet )
{
    float val;
    float* ptr = &val;

    void* voidptr = ptr;

    float* newptr = mCastPtr(float,voidptr);

    mTest( "Pointer cast", newptr==ptr );

    return true;
}


int main( int narg, char** argv )
{
    od_init_test_program( narg, argv );
    const bool quiet = CommandLineParser().hasKey( sKey::Quiet() );

    // Main idea is to test things that are so basic they don't
    // really fit anywhere else.

    if ( !testPointerCast(quiet) )
	ExitProgram(1);

    ExitProgram( 0 );
}
