/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uimain.h"
#include "uiseismmproc.h"

#include "iopar.h"
#include "moddepmgr.h"
#include "prog.h"


int mProgMainFnName( int argc, char** argv )
{
    mInitProg( OD::UiProgCtxt )
    SetProgramArgs( argc, argv, false );
    uiMain app( argc, argv );

    OD::ModDeps().ensureLoaded( "uiIo" );

    PIM().loadAuto( false );
    IOPar jobpars;
    if ( !uiMMBatchJobDispatcher::initMMProgram(argc,argv,jobpars) )
	return 1;

    OD::ModDeps().ensureLoaded( "uiSeis" );
    PtrMan<uiDialog> smmp = new uiSeisMMProc( nullptr, jobpars );
    app.setTopLevel( smmp );
    PIM().loadAuto( true );
    smmp->show();

    return app.exec();
}
