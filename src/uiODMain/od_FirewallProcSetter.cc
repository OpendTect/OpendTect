/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Prajjaval Singh
 Date:          November 2019
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "commandlineparser.h"
#include "file.h"
#include "moddepmgr.h"
#include "oddirs.h"
#include "prog.h"
#include "pythonaccess.h"
#include "procdescdata.h"
#include "uifirewallprocsetterdlg.h"
#include "uimain.h"
#include "uimsg.h"


/*
Arguments :
		0 : Process Type
		1 : Path to OD Dir
		2 : Bool, Is process launched from installer
*/

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
	    ProcDesc::DataEntry::ActionTypeDef().indexOf( proctyp[0]->buf() );

#ifdef mUseCrashDumper
    //Disable IssueReporter for IssueReporter itself.
    System::CrashDumper::getInstance().setSendAppl( "" );
#endif
    BufferString path;
    if ( proctyp.size() > 1 )
	path = *proctyp[1];
    bool instlrlaunchedproc = false;
    if ( proctyp.size() > 2 )
    {
	BufferString str = *proctyp[2];
	instlrlaunchedproc = str.isEqual( "Yes", CaseInsensitive );
    }

    ePDD().setPath( path );

    ProcDesc::DataEntry::ActionType typ = ePDD().getActionType();
    if ( instlrlaunchedproc && typ == ProcDesc::DataEntry::Remove )
	return 0;

    BufferString pythonpath;
    if ( proctyp.size() > 4 )
    {
	BufferString str = GetArgV()[ proctyp.size() - 1];
	bool useextparth = str.isEqual( OD::PythA().sKeyUseExtPyPath(),
							    CaseInsensitive );
	if (useextparth)
	    pythonpath = GetArgV()[ proctyp.size() ];
    }

    uiFirewallProcSetter* dlg = new uiFirewallProcSetter( 0,
	ProcDesc::DataEntry::ActionTypeDef().getEnumForIndex(proctypidx),
	path, pythonpath );

    app.setTopLevel( dlg );
    dlg->show();

    const int ret = app.exec();
    delete dlg;
    return ExitProgram( ret );
}
