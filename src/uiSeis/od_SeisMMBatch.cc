/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          April 2002
________________________________________________________________________

-*/

#include "prog.h"

#include "uimain.h"
#include "uiseismmproc.h"

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
    OD::ModDeps().ensureLoaded( "uiSeis" );

    PtrMan<uiDialog> smmp = new uiSeisMMProc( nullptr, jobpars );
    app.setTopLevel( smmp );
    smmp->show();

    return app.exec();
}
