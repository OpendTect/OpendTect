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

int mProgMainFnName( int argc, char** argv )
{
    mInitProg( OD::UiProgCtxt )
    SetProgramArgs( argc, argv, false );
    uiMain app( argc, argv );
    PIM().loadAuto( false );
    OD::ModDeps().ensureLoaded( "uiODMain" );
    PIM().loadAuto( true );

    CommandLineParser parser;
    BufferStringSet proctyp;
    parser.getNormalArguments( proctyp );

    if ( proctyp.isEmpty() )
	return 0;

    const int proctypidx =
	    ProcDesc::DataEntry::ActionTypeDef().indexOf( proctyp[0]->buf() );

#ifdef mUseCrashDumper
    //Disable IssueReporter for IssueReporter itself.
    System::CrashDumper::getInstance().setSendAppl( "" );
#endif
    BufferString path;
    if ( proctyp.size() > 1 )
	path = *proctyp[1];

    ePDD().setPath( path );
    BufferString pythonpath;
    if ( proctyp.size() > 3 )
    {
	BufferString str = GetArgV()[ proctyp.size() - 1];
	bool useextparth = str.isEqual( OD::PythA().sKeyUseExtPyPath(),
							    CaseInsensitive );
	if ( useextparth )
	    pythonpath = GetArgV()[ proctyp.size() ];
    }

    ProcDesc::DataEntry::ActionType type =
	ProcDesc::DataEntry::ActionTypeDef().getEnumForIndex( proctypidx );

    PtrMan<uiDialog> dlg = new uiFirewallProcSetter( nullptr,
						type, &path, &pythonpath );
    app.setTopLevel( dlg );
    dlg->show();

    return app.exec();
}
