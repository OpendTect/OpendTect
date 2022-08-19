/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "prog.h"

#include "uimain.h"
#include "uiprestackmmproc.h"

#include "iopar.h"
#include "moddepmgr.h"


int mProgMainFnName( int argc, char** argv )
{
    mInitProg( OD::UiProgCtxt )
    SetProgramArgs( argc, argv );
    OD::ModDeps().ensureLoaded( "General" );

    IOPar jobpars;
    if ( !uiMMBatchJobDispatcher::initMMProgram(argc,argv,jobpars) )
	return 1;

    uiMain app( argc, argv );
    OD::ModDeps().ensureLoaded( "uiPreStackProcessing" );

    PtrMan<uiDialog> pmmp = new uiPreStackMMProc( 0, jobpars );
    app.setTopLevel( pmmp );
    pmmp->show();

    return app.exec();
}
