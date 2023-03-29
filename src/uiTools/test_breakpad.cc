/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "moddepmgr.h"
#include "testprog.h"


/*On windows, this will only trigger outside a debugger. Hence, set the path
(to include libraries and the OpendTect libraries and run from command line.
If everything works Breakpad should trigger and write out a crash report.
*/

int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();
    OD::ModDeps().ensureLoaded( "uiTools" );

    DBG::forceCrash(false);

    return 0;
}
