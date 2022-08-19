/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
