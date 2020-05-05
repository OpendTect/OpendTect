/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          April 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "prog.h"

#include "uimain.h"
#include "uiseismmproc.h"

#include "iopar.h"
#include "moddepmgr.h"


int main( int argc, char ** argv )
{
    SetProgramArgs( argc, argv );

    IOPar jobpars;
    if ( !uiMMBatchJobDispatcher::initMMProgram(argc,argv,jobpars) )
	return ExitProgram( 1 );

    uiMain app( argc, argv );
    OD::ModDeps().ensureLoaded( "uiSeis" );

    uiSeisMMProc* smmp = new uiSeisMMProc( 0, jobpars );
    app.setTopLevel( smmp );
    smmp->show();

    const int ret = app.exec();
    delete smmp;
    return ExitProgram( ret );
}
