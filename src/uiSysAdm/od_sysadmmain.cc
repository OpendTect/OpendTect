/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jul 2006
________________________________________________________________________

-*/

#include "prog.h"
#include "genc.h"
#include <iostream>

extern int ODSysAdmMain(int,char**);

int main( int argc, char** argv )
{
    SetProgramArgs( argc, argv );

    int ret = ODSysAdmMain( argc, argv );
    return ExitProgram( ret );
}

