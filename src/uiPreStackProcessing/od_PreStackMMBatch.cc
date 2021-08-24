/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Mar 2014
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
