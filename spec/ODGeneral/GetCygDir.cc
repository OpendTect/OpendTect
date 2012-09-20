/*+
 * AUTHOR   : A.H. Lammertink
 * DATE     : July 2004
 * FUNCTION : Get cygwin directory if installed
-*/

static const char* rcsID = "$Id$";


#include "prog.h"
#include "winutils.h"

#include <iostream>


int main( int argc, char** argv )
{
    const char* cygdir = getCygDir();
    if ( !cygdir || !*cygdir ) return 1;

    std::cout << cygdir << std::endl;

    return 0;
}

