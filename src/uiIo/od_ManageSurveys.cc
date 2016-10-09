/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2016
________________________________________________________________________

-*/

#include "prog.h"
#include "uisurvey.h"
#include "uimain.h"
#include "file.h"
#include "moddepmgr.h"
#include "plugins.h"
#include <string.h>


int main( int argc, char ** argv )
{
    SetProgramArgs( argc, argv, false );

    OD::ModDeps().ensureLoaded( OD::ModDepMgr::sAllNonUI() );
    PIM().loadAuto( false );
    OD::ModDeps().ensureLoaded( OD::ModDepMgr::sAllUI() );
    PIM().loadSurveyInfoProviders();

    uiMain app( GetArgC(), GetArgV() );
    uiSurvey* dlg = new uiSurvey( 0 );
    app.setTopLevel( dlg );
    dlg->show();
    return ExitProgram( app.exec() );
}
