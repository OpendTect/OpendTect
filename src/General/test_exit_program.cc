/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "moddepmgr.h"
#include "testprog.h"


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();
    OD::ModDeps().ensureLoaded( "General" );

    logStream() << "Exiting with return code 1" << od_endl;

    return 1;
}
