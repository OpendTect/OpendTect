/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2014
________________________________________________________________________

-*/

#include "prog.h"

#include "uimain.h"
#include "uiprestackmmproc.h"

#include "iopar.h"
#include "moddepmgr.h"


int main( int argc, char ** argv )
{
    OD::SetRunContext( OD::UiProgCtxt );
    SetProgramArgs( argc, argv );
    uiMain app;

    OD::ModDeps().ensureLoaded( "uiPreStackProcessing" );

    IOPar jobpars;
    if ( !uiMMBatchJobDispatcher::initMMProgram(argc,argv,jobpars) )
	return ExitProgram( 1 );

    uiPreStackMMProc* pmmp = new uiPreStackMMProc( 0, jobpars );
    app.setTopLevel( pmmp );
    pmmp->show();

    const int ret = app.exec();
    delete pmmp;
    return ExitProgram( ret );
}
