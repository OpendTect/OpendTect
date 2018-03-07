/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
-*/

#include "genc.h"
#include "signal.h"
#include "moddepmgr.h"


/*On windows, this will only trigger outside a debugger. Hence, set the path
(to include libraries and the OpendTect libraries and run from command line.
If everything works Breakpad should trigger and write out a crash report.
*/

int main( int argc, char** argv )
{
    SetProgramArgs(argc, argv);
    OD::ModDeps().ensureLoaded("uiTools");

    DBG::forceCrash(false);

    return ExitProgram( 0 );
}
