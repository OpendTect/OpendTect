/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          Mar 2012
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "prog.h"

#include "envvars.h"
#include "strmprov.h"

#ifdef __win__
# include <direct.h>
#endif

static BufferString getInstDir()
{
    BufferString dirnm( _getcwd(NULL,0) );
    const int len = dirnm.size() - 10;
    if ( len > 0 )
	dirnm[len] = '\0';
    return dirnm;
}


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

    BufferString cmd( "od_instmgr.exe --updcheck_startup --instdir " );
    cmd += "\"";
    cmd += getInstDir();
    cmd += "\""; 
    return ExecOSCmd( cmd, true, false );
}


int main( int argc, char** argv )
{
    //ExecODInstMgr(); disable launch of od_instmanager for windows
    ExecODMain( argc, argv );
}
