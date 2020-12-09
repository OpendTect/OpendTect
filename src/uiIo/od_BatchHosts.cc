/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Jan 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uimain.h"
#include "uibatchhostsdlg.h"

#include "moddepmgr.h"
#include "prog.h"

int main( int argc, char** argv )
{
    SetProgramArgs( argc, argv );
    uiMain app( argc, argv );
    OD::ModDeps().ensureLoaded( "uiIo" );

    auto* dlg = new uiBatchHostsDlg( nullptr );
    dlg->showAlwaysOnTop();
    app.setTopLevel( dlg );
    dlg->show();

    const int ret = app.exec();
    delete dlg;
    return ExitProgram( ret );
}
