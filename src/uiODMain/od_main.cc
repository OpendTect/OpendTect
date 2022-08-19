/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "prog.h"
#include "envvars.h"
#include "odver.h"
#include "msgh.h"
#include "stringview.h"
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
    const StringView argv1( argv[1] );
    const bool showversiononly = argv1 == "-v" || argv1 == "--version";

    if ( showversiononly )
    {
	mInitProg( OD::BatchProgCtxt )
	errStream() << GetFullODVersion() << od_endl;
	return 0;
    }

    mInitProg( OD::NormalCtxt )
    uiMain::preInitForOpenGL();
    uiMain app( argc, argv );

    const char* msg =
	    "OpendTect can be run under one of three licenses:"
	    " (GPL, Commercial, Academic).\n"
	    "Please consult http://opendtect.org/OpendTect_license.txt.";

    OD::SetGlobalLogFile( nullptr );
    UsrMsg( msg );

    return ODMain( app );
}
