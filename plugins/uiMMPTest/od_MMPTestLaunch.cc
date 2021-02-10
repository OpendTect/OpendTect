/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		August 2016
________________________________________________________________________

-*/

#include "uimain.h"
#include "uimmptest.h"

#include "iopar.h"
#include "moddepmgr.h"
#include "prog.h"


int mProgMainFnName( int argc, char ** argv )
{
    mInitProg( OD::UiProgCtxt )
    SetProgramArgs( argc, argv );
    OD::ModDeps().ensureLoaded( "General" );

    IOPar jobpars;
    if ( !uiMMBatchJobDispatcher::initMMProgram(argc,argv,jobpars) )
	return 1;

    OD::ModDeps().ensureLoaded( OD::ModDepMgr::sAllNonUI() );

    uiMain app;
    PtrMan<uiDialog> testmmp = new uiMMPTestProc( nullptr, jobpars );
    app.setTopLevel( testmmp );
    testmmp->show();

    return app.exec();
}
