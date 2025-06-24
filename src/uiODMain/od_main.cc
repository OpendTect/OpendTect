/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"

#include "prog.h"
#include "envvars.h"
#include "odver.h"
#include "msgh.h"
#include "stringview.h"
#include "uidialog.h"
#include "uimain.h"
#include "uiodmain.h"

#ifdef __mac__
# include "envvars.h"
# include "file.h"
# include "filepath.h"
# include "oddirs.h"
#endif

#include <iostream>

mGlobal(uiODMain) void ODMain(std::unique_ptr<uiDialog>&,
			      std::unique_ptr<uiODMain>&);

int mProgMainFnName( int argc, char** argv )
{
    SetProgramArgs( argc, argv );
    const StringView argv1( argv[1] );
    const bool showversiononly = argv1 == "-v" || argv1 == "--version";

    if ( showversiononly )
    {
	mInitProg( OD::RunCtxt::BatchProgCtxt )
	errStream() << GetFullODVersion() << od_endl;
	return 0;
    }

    mInitProg( OD::RunCtxt::NormalCtxt )
    uiMain::preInitForOpenGL();
    uiMain app( argc, argv );

    const char* msg =
	    "OpendTect can be run under one of three licenses:"
	    " (GPL, Commercial, Academic).\n"
	    "Please consult https://dgbes.com/licensing.\n";

    OD::SetGlobalLogFile( nullptr );
    std::cout << msg;

    std::unique_ptr<uiDialog> prodseldlg;
    std::unique_ptr<uiODMain> odmain;
    ODMain( prodseldlg, odmain );
    return app.exec();
}
