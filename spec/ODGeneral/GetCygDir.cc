/*+
 * AUTHOR   : A.H. Lammertink
 * DATE     : July 2004
 * FUNCTION : Get cygwin directory if installed
-*/



#include "prog.h"
#include "winutils.h"


int mProgMainFnName( int argc, char** argv )
{
    const char* cygdir = getCygDir();
    if ( !cygdir || !*cygdir ) return 1;

    od_cout() << cygdir << od_endl;

    return 0;
}
