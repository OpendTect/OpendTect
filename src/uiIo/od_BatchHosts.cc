/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibatchhostsdlg.h"

#include "uimain.h"


#include "moddepmgr.h"
#include "prog.h"

int mProgMainFnName( int argc, char** argv )
{
    mInitProg( OD::UiProgCtxt )
    SetProgramArgs( argc, argv, false );
    uiMain app( argc, argv );

    OD::ModDeps().ensureLoaded( "uiTools" );

    PIM().loadAuto( false );
    OD::ModDeps().ensureLoaded( "uiIo" );
    PtrMan<uiDialog> topdlg = new uiBatchHostsDlg( nullptr );
    topdlg->setActivateOnFirstShow();
    app.setTopLevel( topdlg );
    PIM().loadAuto( true );
    topdlg->show();

    return app.exec();
}
