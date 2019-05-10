/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : May 2019
-*/


#include "commandlineparser.h"
#include "moddepmgr.h"
#include "od_ostream.h"
#include "odver.h"
#include "prog.h"
#include "wellmanager.h"

static int protocolnr_ = 1;
static const char* sVersionCmd = "version";


static int printUsage()
{
    od_cout() << "Needs some params" << od_endl;
    return ExitProgram( 0 );
}


int main( int argc, char** argv )
{
    OD::SetRunContext( OD::BatchProgCtxt );
    SetProgramArgs( argc, argv );
    OD::ModDeps().ensureLoaded( "Well" );
    CommandLineParser clp;

    if ( clp.nrArgs() < 1 )
	return printUsage();
    else if ( clp.hasKey( sVersionCmd ) )
    {
	od_cout() << protocolnr_ << "@" << GetFullODVersion() << od_endl;
	return ExitProgram( 0 );
    }

    return ExitProgram( 0 );
}
