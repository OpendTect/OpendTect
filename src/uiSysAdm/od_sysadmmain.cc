/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H.Bril
 Date:          Jul 2006
________________________________________________________________________

-*/


#include "prog.h"
#include "genc.h"
#include <iostream>

extern int ODSysAdmMain(int,char**);

int mProgMainFnName( int argc, char** argv )
{
    mInitProg( OD::SysAdmCtxt )
    SetProgramArgs( argc, argv );

    return ODSysAdmMain( argc, argv );
}
