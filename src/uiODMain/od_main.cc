/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2002
________________________________________________________________________

-*/

#include "prog.h"
#include "dbman.h"
#include "envvars.h"
#include "fixedstring.h"
#include "msgh.h"
#include "odver.h"
#include "commandlineparser.h"
#include "uimain.h"
#include <iostream>

#ifdef __mac__
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#endif

extern int ODMain(uiMain&);


int main( int argc, char** argv )
{
    OD::SetRunContext( OD::NormalCtxt );
    SetProgramArgs( argc, argv, false );
    uiMain::preInitForOpenGL();
    uiMain app;

    auto& clp = app.commandLineParser();
    const int nrargs = clp.nrArgs();
    if ( nrargs == 1 && (clp.getArg(0)=="-v" || clp.getArg(0)=="--version") )
	{ std::cerr << GetFullODVersion() << std::endl; ExitProgram( 0 ); }

    DBM().setDataSource( clp );

    int ret = 0;
    if ( !GetEnvVarYN("OD_I_AM_AN_OPENDTECT_DEVELOPER") )
    {
	const char* msg =
	    "OpendTect can be run under one of three licenses:"
	    " (GPL, Commercial, Academic).\n"
	    "Please consult http://opendtect.org/OpendTect_license.txt.";

	std::cerr << msg << std::endl;
	OD::SetGlobalLogFile( 0 );
	UsrMsg( msg );
    }

#ifdef __mac__
    BufferString datfile( File::Path(GetSoftwareDir(0),
			  "Resources/license.dgb.dat").fullPath());
    if ( File::exists(datfile.buf()) )
    {
	BufferString valstr = GetEnvVar( "LM_LICENSE_FILE" );
	if ( !valstr.isEmpty() )
	    valstr += ":";
	valstr += datfile;
	SetEnvVar( "LM_LICENSE_FILE", valstr.buf() );
    }
#endif

    ret = ODMain( app );
    return ExitProgram( ret );
}
