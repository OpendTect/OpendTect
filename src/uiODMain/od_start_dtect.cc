/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          Mar 2012
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "prog.h"

#include "odinst.h"
#include "strmprov.h"

#ifdef __win__
# include <direct.h>
#endif

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


int main( int argc, char** argv )
{
    SetProgramArgs( argc, argv );
    ODInst::runInstMgrForUpdt();
    ExecODMain( argc, argv );
}
