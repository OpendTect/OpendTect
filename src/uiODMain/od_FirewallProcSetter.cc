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
    uiMain app( argc, argv );
    PIM().loadAuto( false );
    OD::ModDeps().ensureLoaded( "uiODMain" );
    PIM().loadAuto( true );
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
    BufferString path;
    if ( proctyp.size() > 1 )
	path = *proctyp[1];

    uiFirewallProcSetter* dlg = new uiFirewallProcSetter( 0,
	uiFirewallProcSetter::ActionTypeDef().getEnumForIndex(proctypidx),
        path );

    app.setTopLevel( dlg );
    dlg->show();

    const int ret = app.exec();
    delete dlg;
    return ExitProgram( ret );
}
