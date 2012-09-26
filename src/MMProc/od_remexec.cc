/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          August 2010
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "bufstring.h"
#include "genc.h"
#include "iopar.h"
#include "oddirs.h"
#include "remjobexec.h"
#include "strmprov.h"
#include "systeminfo.h"
#include <iostream>


static BufferString createCmdLine( int argc, char** argv )
{
    BufferString cmdline( "@" );
    for ( int idx=2; idx<argc-1; idx++ )
	cmdline.add( argv[idx] ).add( " " );

    cmdline += " \"";
    cmdline += argv[argc-1];
    cmdline += "\"";
    return cmdline;
}

static int executeLocal( int argc, char** argv )
{
    BufferString cmdline = createCmdLine( argc, argv );
    StreamProvider strmprov( cmdline );
    return strmprov.executeCommand( true );
}


int main( int argc, char** argv )
{
    if ( argc < 4 )
	return 1;

    const char* remhost = argv[1];
    BufferString remhostaddress = System::hostAddress( remhost );
    if ( remhostaddress == System::localAddress() )
	return executeLocal( argc, argv );

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
    	
    RemoteJobExec* rje = new RemoteJobExec( remhostaddress, 5050 );
    rje->addPar( par );
    rje->launchProc();
    return 0;
}

