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
    SetProgramArgs( argc, argv );
    CommandLineParser clp( argc, argv );
    const int nrargs = clp.nrArgs();
    const bool useargs = nrargs >= 1;
    if ( useargs )
    {
	const char* reldir = clp.getArg( 0 );
	ODInst::startInstManagementWithRelDir( reldir );
    }
    else
	ODInst::startInstManagement();
 
    return 0;
}
