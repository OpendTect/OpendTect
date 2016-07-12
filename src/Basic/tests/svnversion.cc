/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION :
-*/


#include "genc.h"
#include "testprog.h"


int testMain( int argc, char** argv )
{
    mInitTestProg();

    const char* version = GetVCSVersion();

    if ( !version || !*version )
    {
	if ( !quiet )
	{
	    od_cout() << "Invalid vcs revision. "
		    "Cmake could probably not find svn command-client. "
		    "Take a look in CMakeModules/ODSubversion.cmake.\n";
	}
	return 1;
    }

    return 0;
}
