/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          06/02/2002
 RCS:           $Id: uicmain.cc,v 1.2 2002-07-22 07:23:08 kristofer Exp $
________________________________________________________________________

-*/

#include "uicmain.h"
#include "visinventorinit.h"
#include <Inventor/Qt/SoQt.h>

uicMain::uicMain(int argc,char** argv)
: uiMain( argc, argv )
{}

void uicMain::init(QWidget* mw)
{
    SoQt::init(mw);
    visBase::initdGBInventorClasses();
}

int uicMain::exec()
{
//    toplevel().raise();
    SoQt::mainLoop();
    return 0;
}
