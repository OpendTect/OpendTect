/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          06/02/2002
 RCS:           $Id: uicmain.cc,v 1.8 2003-11-07 12:22:01 bert Exp $
________________________________________________________________________

-*/

#include "uicmain.h"
#include "visinventorinit.h"
#include <Inventor/Qt/SoQt.h>
#include <Inventor/SoDB.h>
#include <stdlib.h>

uicMain::uicMain(int argc,char** argv)
: uiMain( argc, argv )
{}

void uicMain::init( QWidget* mw )
{
    if ( !getenv("SOQT_BRIL_X11_SILENCER_HACK") )
	putenv( "SOQT_BRIL_X11_SILENCER_HACK=1" );
    if ( !getenv("COIN_FULL_INDIRECT_RENDERING") )
	putenv( "COIN_FULL_INDIRECT_RENDERING=1" );
    SoQt::init(mw);
    visBase::initdGBInventorClasses();
    SoDB::init();
}

int uicMain::exec()
{
//    toplevel().raise();
    SoQt::mainLoop();
    return 0;
}
