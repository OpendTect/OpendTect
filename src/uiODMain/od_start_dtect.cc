/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          Mar 2012
________________________________________________________________________

-*/

#include "prog.h"

#include "odinst.h"
#include "oscommand.h"



static bool ExecODMain( int argc, char** argv )
{
    OS::MachineCommand machcomm( "od_main" );
    for ( int iarg=1; iarg<argc; iarg++ )
	machcomm.addArg( iarg );
    return machcomm.execute( OS::RunInBG );
}


int main( int argc, char** argv )
{
    OD::SetRunContext( OD::NormalCtxt );
    SetProgramArgs( argc, argv );
    return ExecODMain( argc, argv );
}
