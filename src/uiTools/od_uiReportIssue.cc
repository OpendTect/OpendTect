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
    OD::ModDeps().ensureLoaded( "uiTools" );
    uiMain app( argc, argv );
    
    uiIssueReporterDlg* dlg = new uiIssueReporterDlg( 0 );

    if ( !dlg->reporter().parseCommandLine() )
    {
	uiMSG().error( dlg->reporter().errMsg() );
	return ExitProgram( 1 );	
    }
    
    app.setTopLevel( dlg );
    dlg->show();
    
    return ExitProgram( app.exec() );
}
