/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          06/02/2002
 RCS:           $Id: uicmain.cc,v 1.6 2002-12-17 11:49:07 nanne Exp $
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
#ifndef __debug__
    putenv( "SOQT_BRIL_X11_SILENCER_HACK=1" );
#endif
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
