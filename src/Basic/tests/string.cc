/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION : 
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "string2.h"
#include "commandlineparser.h"
#include "keystrs.h"
#include "debug.h"

#include <iostream>

#define mRunTest( desc, test ) \
{ \
    if ( (test) ) \
    { \
	if ( !quiet ) \
	{ \
	    std::cout << desc << ":"; \
	    std::cout << " OK\n"; \
	} \
    } \
    else \
    { \
	std::cout << desc << ":"; \
	std::cout << " Fail\n"; \
	return false; \
    } \
}


bool testBytes2String( bool quiet )
{
    NrBytesToStringCreator b2s;
    b2s.setUnitFrom( 100000, true ); //hundred thoughand
    mRunTest( "kB unit (1)",
	     b2s.getUnitString()== b2s.toString(NrBytesToStringCreator::KB) )
    
    b2s.setUnitFrom( 1000000, true ); //One million
    mRunTest( "kB unit (2)",
	     b2s.getUnitString()== b2s.toString(NrBytesToStringCreator::KB) )

    b2s.setUnitFrom( 2000000, true ); //Two millions
    mRunTest( "MB unit",
	     b2s.getUnitString()== b2s.toString(NrBytesToStringCreator::MB) )
    
    b2s.setUnitFrom( 1000000, true ); //One million
    mRunTest( "Maximum flag turned on",
	     b2s.getUnitString()== b2s.toString(NrBytesToStringCreator::MB) )

    b2s.setUnitFrom( 1000000, false ); //One million
    mRunTest( "Maximum flag turned off",
	     b2s.getUnitString()== b2s.toString(NrBytesToStringCreator::KB) )
    
    mRunTest( "Conversion test", b2s.getString( 100000 )=="97.66 kB" );

    return true;
}


int main( int narg, char** argv )
{
    od_init_test_program( narg, argv );
    
    const bool quiet = CommandLineParser().hasKey( sKey::Quiet() );


    if ( !testBytes2String(quiet) )
	ExitProgram( 1 );

    ExitProgram( 0 );
}
