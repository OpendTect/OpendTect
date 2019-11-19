/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Prajjaval Singh
 Date:          November 2019
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uifirewallprocsetterdlg.h"
#include "commandlineparser.h"
#include "moddepmgr.h"
#include "prog.h"
#include "uimain.h"
#include "uimsg.h"


int main( int argc, char ** argv )
{
    SetProgramArgs( argc, argv );
    OD::ModDeps().ensureLoaded( "uiTools" );
    OD::ModDeps().ensureLoaded( "uiSeis" );
    uiMain app( argc, argv );

    SetProgramArgs( argc, argv );

    CommandLineParser parser;
    BufferStringSet proctyp;
    parser.getNormalArguments( proctyp );

    const int proctypidx =
	    uiFirewallProcSetter::ActionTypeDef().indexOf( proctyp[0]->buf() );

#ifdef mUseCrashDumper
    //Disable IssueReporter for IssueReporter itself.
    System::CrashDumper::getInstance().setSendAppl( "" );
#endif
    uiFirewallProcSetter* dlg = new uiFirewallProcSetter( 0,
	uiFirewallProcSetter::ActionTypeDef().getEnumForIndex(proctypidx) );

    app.setTopLevel( dlg );
    dlg->show();

    const int ret = app.exec();
    delete dlg;
    return ExitProgram( ret );
}
