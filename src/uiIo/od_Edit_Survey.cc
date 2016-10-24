/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2016
________________________________________________________________________

-*/

#include "prog.h"
#include "uiusercreatesurvey.h"
#include "uisurvinfoed.h"
#include "dbman.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "survinfo.h"
#include "uimain.h"
#include "moddepmgr.h"
#include "plugins.h"
#include "oscommand.h"
#include "commandlineparser.h"
#include <string.h>

#define mErrRet( s ) exitWithErrMsg( s )

static void exitWithErrMsg( const char* msg )
{
    BufferString cmd( "od_DispMsg --err ", msg );
    OS::ExecCommand( cmd );
    ExitProgram( 1 );
}


int main( int argc, char ** argv )
{
    SetProgramArgs( argc, argv );

    CommandLineParser clp;
    clp.setKeyHasValue( "dataroot" );
    BufferStringSet normargs;
    BufferString dataroot, survdir, fullsurvpath;
    const bool createmode = clp.hasKey( "create" );
    const bool altdataroot = clp.getVal( "dataroot", dataroot );
    clp.getNormalArguments( normargs );
    if ( !altdataroot || dataroot.isEmpty() )
	dataroot = GetBaseDataDir();

    if ( !createmode )
    {
	survdir = normargs.size() > 0 ? normargs.get( 0 ) : SI().getDirName();
	fullsurvpath = File::Path( dataroot, survdir ).fullPath();
	if ( !File::exists(fullsurvpath) )
	    mErrRet( BufferString( "Passed survey directory does not exist:\n",
					fullsurvpath ) );
    }

    OD::ModDeps().ensureLoaded( OD::ModDepMgr::sAllNonUI() );
    PIM().loadAuto( false );
    OD::ModDeps().ensureLoaded( OD::ModDepMgr::sAllUI() );
    PIM().loadSurveyRelatedTools();

    uiMain app( GetArgC(), GetArgV() );
    uiDialog* toplevel = 0;
    if ( createmode )
	toplevel = new uiUserCreateSurvey( 0, dataroot );
    else
    {
	if ( !survdir.isEmpty() )
	{
	    uiRetVal uirv = DBM().setDataSource( fullsurvpath );
	    if ( uirv.isError() )
		mErrRet( uirv.getText() );
	}
	toplevel = new uiSurveyInfoEditor( 0 );
    }

    app.setTopLevel( toplevel );
    toplevel->show();
    return ExitProgram( app.exec() );
}
