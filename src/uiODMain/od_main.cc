/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"

#include "envvars.h"
#include "file.h"
#include "msgh.h"
#include "odver.h"
#include "od_ostream.h"
#include "prog.h"
#include "stringview.h"
#include "uidialog.h"
#include "uimain.h"
#include "uiodmain.h"

#include <iostream>


mGlobal(uiODMain) void ODMain(std::unique_ptr<uiDialog>&,
			      std::unique_ptr<uiODMain>&);

int mProgMainFnName( int argc, char** argv )
{
    const StringView argv1( argv[1] );
    const bool showversiononly = argv1 == "-v" || argv1 == "--version";
    if ( showversiononly )
    {
	SetProgramArgs( argc, argv, false );
	mInitProg( OD::RunCtxt::BatchProgCtxt )
	od_cout() << GetFullODVersion() << od_endl;
	return 0;
    }

    SetProgramArgs( argc, argv );
    mInitProg( OD::RunCtxt::NormalCtxt )
    uiMain::preInitForOpenGL();
    uiMain app( argc, argv );
    ApplicationData::setApplicationName( "OpendTect" );

     const BufferString msg(
	    "OpendTect can be run under one of three licenses:"
	    " (GPL, Commercial, Academic).\n"
	    "Please consult https://dgbes.com/licensing.\n" );

    OD::SetGlobalLogFile( nullptr );
    std::cout << msg.str();
    if ( File::exists(od_ostream::logStream().fileName()) )
	UsrMsg( msg.str() );

    std::unique_ptr<uiDialog> prodseldlg;
    std::unique_ptr<uiODMain> odmain;
    ODMain( prodseldlg, odmain );
    return app.exec();
}
