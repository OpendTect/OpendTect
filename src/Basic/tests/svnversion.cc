/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "genc.h"
#include "testprog.h"


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    const char* version = GetVCSVersion();

    if ( !version || !*version )
    {
	logStream() << "Invalid vcs revision. "
		"Cmake could probably not find svn command-client. "
		"Take a look in CMakeModules/ODSubversion.cmake.\n";
	return 1;
    }

    return 0;
}
