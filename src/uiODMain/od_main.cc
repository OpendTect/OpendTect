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

#ifdef __mac__
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#endif

extern int ODMain(uiMain&);


int mProgMainFnName( int argc, char** argv )
{
    SetProgramArgs( argc, argv );
    const FixedString argv1( argv[1] );
    const bool showversiononly = argv1 == "-v" || argv1 == "--version";
    if ( showversiononly )
    {
	mInitProg( OD::BatchProgCtxt )
	errStream() << GetFullODVersion() << od_endl;
	return 0;
    }

    mInitProg( OD::NormalCtxt )
    uiMain::preInitForOpenGL();
    uiMain app;

    DBM().setDataSource( app.commandLineParser() );

    if ( !GetEnvVarYN("OD_I_AM_AN_OPENDTECT_DEVELOPER") )
    {
	const char* msg =
	    "OpendTect can be run under one of three licenses:"
	    " (GPL, Commercial, Academic).\n"
	    "Please consult http://opendtect.org/OpendTect_license.txt.";

	OD::SetGlobalLogFile( nullptr );
	UsrMsg( msg );
    }

    return ODMain( app );
}
