/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiprestackmmproc.h"

#include "uimain.h"

#include "commandlineparser.h"
#include "iopar.h"
#include "moddepmgr.h"
#include "prog.h"


int mProgMainFnName( int argc, char** argv )
{
    mInitProg( OD::RunCtxt::UiProgCtxt )
    SetProgramArgs( argc, argv );
    uiMain app( argc, argv );

    OD::ModDeps().ensureLoaded( "uiIo" );

    const CommandLineParser clp( argc, argv );
    IOPar jobpars;
    const uiRetVal uirv = uiMMBatchJobDispatcher::initMMProgram( clp, jobpars );
    if ( !uirv.isOK() )
	{ od_cerr() << uirv.getText() << od_endl; return 1; }

    PIM().loadAuto( false );
    OD::ModDeps().ensureLoaded( "uiPreStackProcessing" );
    PtrMan<uiDialog> topdlg = new uiPreStackMMProc( nullptr, jobpars );
    app.setTopLevel( topdlg );
    PIM().loadAuto( true );
    topdlg->show();

    return app.exec();
}
