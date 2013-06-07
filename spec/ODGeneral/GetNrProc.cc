/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Nov 2002
 * FUNCTION : Generate file to include in make.Vars
-*/

static const char* rcsID = "$Id: GetNrProc.cc,v 1.3 2009/07/22 16:01:29 cvsbert Exp $";

#include "prog.h"
#include "thread.h"
#include <stdlib.h>
#include <iostream>

int main( int argc, char** argv )
{
    std::cout << Threads::getNrProcessors();
    std::cout.flush();
    ExitProgram( 0 ); return 0;
}
