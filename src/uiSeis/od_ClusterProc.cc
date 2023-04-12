/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiclusterproc.h"

#include "uimain.h"
#include "uimmbatchjobdispatch.h"

#include "commandlineparser.h"
#include "executor.h"
#include "moddepmgr.h"
#include "prog.h"


int mProgMainFnName( int argc, char** argv )
{
    SetProgramArgs( argc, argv );
    OD::ModDeps().ensureLoaded( "uiTools" );

    const CommandLineParser clp( argc, argv );
    const bool withdelete = !clp.hasKey( "nodelete" );
    const bool dosubmit = clp.hasKey( "dosubmit" );
    mInitProg( dosubmit ? OD::UiProgCtxt : OD::BatchProgCtxt )

    IOPar jobpars;
    const uiRetVal uirv = uiMMBatchJobDispatcher::initMMProgram( clp, jobpars );
    if ( !uirv.isOK() )
	{ od_cerr() << uirv.getText() << od_endl; return 1; }

    PIM().loadAuto( false );
    OD::ModDeps().ensureLoaded( "uiSeis" );
    if ( dosubmit )
    {
	uiMain app( argc, argv );
	PtrMan<uiDialog> topdlg = new uiClusterProc( nullptr, jobpars );
	app.setTopLevel( topdlg );
	PIM().loadAuto( true );
	topdlg->setActivateOnFirstShow();
	topdlg->show();

	return app.exec();
    }

    PIM().loadAuto( true );

    od_cout() << "Merging output ..." << od_endl;
    TextTaskRunner taskrunner( od_cout() );
    BufferString msg;
    const bool result =
	uiClusterProc::mergeOutput( jobpars, &taskrunner, msg, withdelete );
    od_cout() << msg << od_endl;

    return result ? 0 : 1;
}
