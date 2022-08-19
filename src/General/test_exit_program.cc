/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "prog.h"


int mProgMainFnName( int argc, char** argv )
{
    mInitProg( OD::TestProgCtxt );

    logStream() << "Exiting with return code 1" << od_endl;
    return 1;
}
