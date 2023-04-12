/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiissuereporter.h"
#include "moddepmgr.h"
#include "prog.h"
#include "uimain.h"
#include "uimsg.h"


int mProgMainFnName( int argc, char** argv )
{
    mInitProg( OD::UiProgCtxt )
    SetProgramArgs( argc, argv, false );
    uiMain app( argc, argv );

    OD::ModDeps().ensureLoaded( "Network" );
    OD::ModDeps().ensureLoaded( "uiBase" );

#ifdef mUseCrashDumper
    //Disable IssueReporter for IssueReporter itself.
    System::CrashDumper::getInstance().setSendAppl( "" );
#endif

    System::IssueReporter reporter;
    if ( !reporter.parseCommandLine() )
    {
	uiMSG().error( reporter.errMsg() );
	return 1;
    }

    PIM().loadAuto( false );
    OD::ModDeps().ensureLoaded( "uiTools" );
    PtrMan<uiDialog> topdlg = new uiIssueReporterDlg( nullptr, reporter );
    app.setTopLevel( topdlg );
    PIM().loadAuto( true );
    topdlg->show();

    return app.exec();
}
