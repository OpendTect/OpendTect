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
#include "survinfo.h"
#include "uimain.h"
#include "moddepmgr.h"
#include "plugins.h"
#include <string.h>


int main( int argc, char ** argv )
{
    SetProgramArgs( argc, argv );
    const bool createmode = argc > 1 && FixedString(argv[1]) == "--create";
    BufferString survdir;
    if ( !createmode && argc > 1 )
    {
	survdir = argv[1];
	if ( !File::exists(survdir) )
	{
	    ErrMsg( BufferString( "Passed survey directory does not exist:\n",
					survdir ) );
	    ExitProgram( 1 );
	}
    }

    OD::ModDeps().ensureLoaded( OD::ModDepMgr::sAllNonUI() );
    PIM().loadAuto( false );
    OD::ModDeps().ensureLoaded( OD::ModDepMgr::sAllUI() );
    PIM().loadSurveyRelatedTools();

    uiMain app( GetArgC(), GetArgV() );
    uiDialog* toplevel = 0;
    if ( createmode )
	toplevel = new uiUserCreateSurvey( 0 );
    else
    {
	if ( !survdir.isEmpty() )
	{
	    uiRetVal uirv = DBM().setDataSource( survdir );
	    if ( uirv.isError() )
	    {
		ErrMsg( uirv.getText() );
		ExitProgram( 1 );
	    }
	}
	toplevel = new uiSurveyInfoEditor( 0 );
    }

    app.setTopLevel( toplevel );
    toplevel->show();
    return ExitProgram( app.exec() );
}
