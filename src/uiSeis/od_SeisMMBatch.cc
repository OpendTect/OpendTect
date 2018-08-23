/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          April 2002
________________________________________________________________________

-*/

#include "prog.h"

#include "uimain.h"
#include "uiseismmproc.h"

#include "iopar.h"
#include "moddepmgr.h"


int main( int argc, char ** argv )
{
    OD::SetRunContext( OD::UiProgCtxt );
    SetProgramArgs( argc, argv );
    uiMain app;

    OD::ModDeps().ensureLoaded( "uiSeis" );

    IOPar jobpars;
    if ( !uiMMBatchJobDispatcher::initMMProgram(argc,argv,jobpars) )
	return ExitProgram( 1 );

    uiSeisMMProc* smmp = new uiSeisMMProc( 0, jobpars );
    app.setTopLevel( smmp );
    smmp->show();

    const int ret = app.exec();
    delete smmp;
    return ExitProgram( ret );
}
