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

#include "prog.h"

int main( int argc, char** argv )
{
    SetProgramArgs( argc, argv );
    uiMain app( argc, argv );

    uiBatchHostsDlg* dlg = new uiBatchHostsDlg( 0 );
    dlg->showAlwaysOnTop();
    app.setTopLevel( dlg );
    dlg->show();

    return ExitProgram( app.exec() );
}
