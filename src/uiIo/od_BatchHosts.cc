/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2003
________________________________________________________________________

-*/

#include "uimain.h"
#include "uibatchhostsdlg.h"

#include "moddepmgr.h"
#include "prog.h"

int mProgMainFnName( int argc, char** argv )
{
    mInitProg( OD::UiProgCtxt )
    SetProgramArgs( argc, argv, false );
    uiMain app;
    OD::ModDeps().ensureLoaded( "uiIo" );

    PtrMan<uiDialog> dlg = new uiBatchHostsDlg( nullptr );
    dlg->setActivateOnFirstShow();
    app.setTopLevel( dlg );
    dlg->show();

    return app.exec();
}
