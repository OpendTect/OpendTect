/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "genc.h"
#include "signal.h"
#include "moddepmgr.h"

#include "prog.h"


/*On windows, this will only trigger outside a debugger. Hence, set the path
(to include libraries and the OpendTect libraries and run from command line.
If everything works Breakpad should trigger and write out a crash report.
*/

int mProgMainFnName( int argc, char** argv )
{
    mInitProg( OD::TestProgCtxt );
    SetProgramArgs( argc, argv, false );

    OD::ModDeps().ensureLoaded( "Network" );
    OD::ModDeps().ensureLoaded( "uiBase" );

    PIM().loadAuto( false );
    OD::ModDeps().ensureLoaded( "uiTools" );
    PIM().loadAuto( true );

    DBG::forceCrash(false);

    return 0;
}
