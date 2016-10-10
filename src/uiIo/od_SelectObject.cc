/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2016
________________________________________________________________________

-*/

#include "prog.h"
#include "uiselobjothersurv.h"
#include "uimain.h"
#include "moddepmgr.h"
#include "plugins.h"
#include <string.h>


int main( int argc, char ** argv )
{
    SetProgramArgs( argc, argv );

    OD::ModDeps().ensureLoaded( OD::ModDepMgr::sAllNonUI() );
    PIM().loadAuto( false );
    OD::ModDeps().ensureLoaded( OD::ModDepMgr::sAllUI() );

    uiMain app( GetArgC(), GetArgV() );
    // uiSelObjFromOtherSurvey* dlg = new uiSelObjFromOtherSurvey( 0 );
    // app.setTopLevel( dlg );
    // dlg->show();
    return ExitProgram( app.exec() );
}
