/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          Mar 2012
________________________________________________________________________

-*/
static const char* rcsID = "$Id: od_runinst.cc 8761 2013-12-06 12:06:24Z bert.bril@dgbes.com $";

#include "prog.h"

#include "commandlineparser.h"
#include "odinst.h"

int main( int argc, char** argv )
{
    SetProgramArgs( argc, argv );
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
 
    return ExitProgram( 0 );
}
