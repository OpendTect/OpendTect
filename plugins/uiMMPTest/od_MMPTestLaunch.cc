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


int main( int argc, char ** argv )
{
    SetProgramArgs( argc, argv );

    IOPar jobpars;
    if ( !uiMMBatchJobDispatcher::initMMProgram(argc,argv,jobpars) )
	return ExitProgram( 1 );

    OD::ModDeps().ensureLoaded( OD::ModDepMgr::sAllNonUI() );

    uiMain app( argc, argv );
    uiMMPTestProc* testmmp = new uiMMPTestProc( 0, jobpars );
    app.setTopLevel( testmmp );
    testmmp->show();
    const int ret = app.exec();
    delete testmmp;
    return ExitProgram( ret );
}
