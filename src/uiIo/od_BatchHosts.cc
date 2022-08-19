/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
    uiMain app( argc, argv );
    OD::ModDeps().ensureLoaded( "uiIo" );

    PtrMan<uiDialog> dlg = new uiBatchHostsDlg( nullptr );
    dlg->setActivateOnFirstShow();
    app.setTopLevel( dlg );
    dlg->show();

    return app.exec();
}
