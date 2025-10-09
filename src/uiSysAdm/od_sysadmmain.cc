/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "genc.h"
#include "moddepmgr.h"
#include "prog.h"

#include "uimain.h"

#include <iostream>

extern int ODSysAdmMain(uiMain&);

int mProgMainFnName( int argc, char** argv )
{
    SetProgramArgs( argc, argv, false );
    mInitProg( OD::RunCtxt::SysAdmCtxt )
    uiMain app( argc, argv );

    return ODSysAdmMain( app );
}
