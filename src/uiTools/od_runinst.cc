/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/


#include "prog.h"

#include "commandlineparser.h"
#include "odinst.h"

int mProgMainFnName( int argc, char** argv )
{
    mInitProg( OD::InstallerCtxt )
    SetProgramArgs( argc, argv, false );
    CommandLineParser cl( argc, argv );
    const int nrargs = cl.nrArgs();
    const bool useargs = nrargs >= 1;
    if ( useargs )
    {
	const char* reldir = cl.getArg( 0 );
	ODInst::startInstManagementWithRelDir( reldir );
    }
    else
	ODInst::startInstManagement();
 
    return 0;
}
