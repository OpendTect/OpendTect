/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION : 
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "genc.h"

#include "debug.h"

#include <iostream>


int main( int narg, char** argv )
{
    od_init_test_program( narg, argv );

    const int svnversion = GetSubversionRevision();
    const char* svnurl = GetSubversionUrl();

    if ( svnversion<1 || !svnurl || !*svnurl )
    {
	std::cerr << "Invalid svn revision. "
		"Cmake could probably not find svn command-client. "
		"Take a look in CMakeModules/ODSubversion.cmake.\n";
	return 1;
    }

    return 0;
}
