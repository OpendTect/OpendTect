/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          Mar 2012
________________________________________________________________________

-*/
static const char* rcsID = "$Id: od_start_dtect.cc,v 1.1 2012-03-07 06:58:07 cvsranojay Exp $";

#include "prog.h"

#include "envvars.h"
#include "oddirs.h"
#include "strmprov.h"


static BufferString getCmdLine( int argc, char** argv )
{
    BufferString cmdline;
    for ( int idx=1; idx<argc; idx++ )
	cmdline.add( argv[idx] ).add( " " );
    return cmdline;
}


static bool ExecODMain( int argc, char** argv )
{
    BufferString cmd( "od_main " );
    cmd += getCmdLine( argc, argv );
    return ExecOSCmd( cmd, true, true );
}


static bool ExecODInstMgr()
{
    BufferString envvar = GetEnvVar( "OD_INSTALLER_POLICY" );
    if ( envvar == "None" )
	return true;

    BufferString cmd( "uiODInstMgr --updcheck_startup --instdir " );
    cmd += GetSoftwareDir( false );
    return ExecOSCmd( cmd, true, false );
}


int main( int argc, char** argv )
{
    if ( ExecODInstMgr() )
	ExecODMain( argc, argv );
}
