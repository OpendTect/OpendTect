/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Jan 2003
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: od_uiReportIssue.cc,v 1.1 2012-06-26 12:50:14 cvskris Exp $";

#include "uiissuereporter.h"

#include "uimain.h"

#include "prog.h"

#include "uimsg.h"


int main( int argc, char ** argv )
{
    uiMain app( argc, argv );
    
    uiIssueReporterDlg* dlg = new uiIssueReporterDlg( 0 );

    if ( !dlg->reporter().parseCommandLine( argc, argv ) )
    {
	uiMSG().error( dlg->reporter().errMsg() );
	ExitProgram( 1 );	
    }
    
    app.setTopLevel( dlg );
    dlg->show();
    
    
    ExitProgram( app.exec() );
}
