/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          Mar 2012
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
