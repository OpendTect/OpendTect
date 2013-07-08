/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION : 
-*/

static const char* rcsID mUsedVar = "$Id: refcount.cc 30402 2013-06-21 07:20:27Z kristofer.tingdahl@dgbes.com $";

#include "genc.h"
#include "commandlineparser.h"
#include "keystrs.h"

#include <iostream>


int main( int narg, char** argv )
{
    od_init_test_program( narg, argv );

    CommandLineParser clparser;
    const bool quiet = clparser.hasKey( sKey::Quiet() );

    if ( !quiet )
	std::cout << "Exiting with return code 1\n";

    return ExitProgram( 1 );
}
