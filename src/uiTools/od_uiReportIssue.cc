/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2003
________________________________________________________________________

-*/

#include "uiissuereporter.h"
#include "moddepmgr.h"
#include "prog.h"
#include "uimain.h"
#include "uimsg.h"


int main( int argc, char ** argv )
{
    OD::SetRunContext( OD::UiProgCtxt );
    SetProgramArgs( argc, argv );
    uiMain app;

    OD::ModDeps().ensureLoaded( "uiTools" );

#ifdef mUseCrashDumper
    //Disable IssueReporter for IssueReporter itself.
    System::CrashDumper::getInstance().setSendAppl( "" );
#endif

    System::IssueReporter reporter;
    if ( !reporter.parseCommandLine() )
    {
	gUiMsg().error( reporter.errMsg() );
	return ExitProgram( 1 );
    }

    uiIssueReporterDlg* dlg = new uiIssueReporterDlg( 0, reporter );
    app.setTopLevel( dlg );
    dlg->show();

    const int ret = app.exec();
    delete dlg;
    return ExitProgram( ret );
}
