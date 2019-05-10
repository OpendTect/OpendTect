/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : May 2019
-*/


#include "wellman.h"
#include "commandlineparser.h"
#include "moddepmgr.h"
#include "od_ostream.h"


static void printUsage()
{
    od_cout << "Needs some params" << od_endl;
}


int main( int argc, char** argv )
{
    OD::SetRunContext( OD::BatchProgCtxt );
    SetProgramArgs( argc, argv );
    OD::ModDeps().ensureLoaded( "Well" );
    CommandLineParser clp;
    auto* jobj = &jret_;
    if ( clp.nrArgs() < 1 )
	return printUsage();
    else if ( clp.hasKey( sVersionCmd ) )
    {
	od_cout() << protocolnr_ << "@" << GetFullODVersion() << od_endl;
	return ExitProgram( 0 );
    }

    return ExitProgram( 0 );
}
