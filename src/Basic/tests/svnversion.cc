/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION :
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "genc.h"
#include "testprog.h"


int main( int argc, char** argv )
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
	ExitProgram( 1 );
    }

    return ExitProgram( 0 );
}

