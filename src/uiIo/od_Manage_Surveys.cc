/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2016
________________________________________________________________________

-*/

#include "prog.h"
#include "uisurveymanager.h"
#include "uimain.h"
#include "file.h"
#include "moddepmgr.h"
#include "plugins.h"
#include <string.h>


int mProgMainFnName( int argc, char ** argv )
{
    mInitProg( OD::UiProgCtxt )
    SetProgramArgs( argc, argv, false );
    uiMain app;

    OD::ModDeps().ensureLoaded( OD::ModDepMgr::sAllNonUI() );
    PIM().loadAuto( false );
    OD::ModDeps().ensureLoaded( OD::ModDepMgr::sAllUI() );
    PIM().loadSurveyRelatedTools();

    PtrMan<uiDialog> dlg = new uiSurveyManagerDlg( nullptr, true );
    dlg->setHelpKey( mODHelpKey(mSurveyHelpID) );
    app.setTopLevel( dlg );
    dlg->show();

    return app.exec();
}
