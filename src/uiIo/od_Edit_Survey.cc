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

#define mErrRet( msg ) \
{ \
    OD::DisplayErrorMessage( msg ); \
    return 1; \
}


int mProgMainFnName( int argc, char ** argv )
{
    mInitProg( OD::UiProgCtxt )
    SetProgramArgs( argc, argv );
    uiMain app;

    auto& clp = app.commandLineParser();

    BufferString dataroot = clp.keyedString( "dataroot" );
    if ( dataroot.isEmpty() )
	dataroot = GetBaseDataDir();
    BufferStringSet normargs;
    clp.getNormalArguments( normargs );

    BufferString survdir, fullsurvpath;
    const bool createmode = clp.hasKey( "create" );
    if ( !createmode )
    {
	survdir = normargs.size() > 0 ? normargs.get( 0 ) : SI().dirName();
	fullsurvpath = File::Path( dataroot, survdir ).fullPath();
	if ( !File::exists(fullsurvpath) )
	    mErrRet( BufferString( "Passed survey directory does not exist:\n",
					fullsurvpath ) );
    }

    OD::ModDeps().ensureLoaded( OD::ModDepMgr::sAllNonUI() );
    PIM().loadAuto( false );
    OD::ModDeps().ensureLoaded( OD::ModDepMgr::sAllUI() );
    PIM().loadSurveyRelatedTools();

    PtrMan<uiDialog> toplevel = nullptr;
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

    return app.exec();
}
