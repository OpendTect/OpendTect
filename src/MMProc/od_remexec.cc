/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          August 2010
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "hostdata.h"
#include "iopar.h"
#include "prog.h"
#include "remjobexec.h"
#include "systeminfo.h"


int main( int argc, char** argv )
{
    if ( argc < 4 )
	return 1;

    SetProgramArgs( argc, argv );
    
    IOPar par;
    par.set( "Proc Name", argv[2] );
    if ( argc <= 4  )
	par.set( "Par File", argv[3] );
    else
    {
	par.set( "Host Name", argv[4] );
	par.set( "Port Name", argv[6] );
	par.set( "Job ID", argv[8] );
	par.set( "Par File", argv[9] );
    }

    const char* hostnmarg = argv[1];
    HostData hd( hostnmarg );
    BufferString remhostaddress = hd.getIPAddress();
    if ( remhostaddress.isEmpty() )
	remhostaddress = System::hostAddress( hostnmarg );
    if ( remhostaddress.isEmpty() )
	remhostaddress = hostnmarg;

    RemoteJobExec* rje = new RemoteJobExec( remhostaddress, 5050 );
    rje->addPar( par );
    rje->launchProc();
    return ExitProgram( 0 );
}

