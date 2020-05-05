/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Jan 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiissuereporter.h"
#include "moddepmgr.h"
#include "prog.h"
#include "uimain.h"
#include "uimsg.h"


int main( int argc, char ** argv )
{
    SetProgramArgs( argc, argv );
    uiMain app( argc, argv );

    OD::ModDeps().ensureLoaded( "uiTools" );

#ifdef mUseCrashDumper
    //Disable IssueReporter for IssueReporter itself.
    System::CrashDumper::getInstance().setSendAppl( "" );
#endif

    System::IssueReporter reporter;
    if ( !reporter.parseCommandLine() )
    {
	uiMSG().error( reporter.errMsg() );
	return ExitProgram( 1 );
    }

    uiIssueReporterDlg* dlg = new uiIssueReporterDlg( 0, reporter );
    app.setTopLevel( dlg );
    dlg->show();

    const int ret = app.exec();
    delete dlg;
    return ExitProgram( ret );
}
