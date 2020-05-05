/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Mar 2014
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "prog.h"

#include "uimain.h"
#include "uiprestackmmproc.h"

#include "iopar.h"
#include "moddepmgr.h"


int main( int argc, char ** argv )
{
    SetProgramArgs( argc, argv );
    uiMain app( argc, argv );

    IOPar jobpars;
    if ( !uiMMBatchJobDispatcher::initMMProgram(argc,argv,jobpars) )
	return ExitProgram( 1 );

    OD::ModDeps().ensureLoaded( "uiPreStackProcessing" );

    uiPreStackMMProc* pmmp = new uiPreStackMMProc( 0, jobpars );
    app.setTopLevel( pmmp );
    pmmp->show();

    const int ret = app.exec();
    delete pmmp;
    return ExitProgram( ret );
}
