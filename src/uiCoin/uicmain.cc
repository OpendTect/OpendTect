/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          06/02/2002
 RCS:           $Id: uicmain.cc,v 1.1 2002-02-07 14:58:34 arend Exp $
________________________________________________________________________

-*/

#include "uicmain.h"
#include <Inventor/Qt/SoQt.h>

uicMain::uicMain(int argc,char** argv)
: uiMain( argc, argv )
{}

void uicMain::init(QWidget* mw)
{
    SoQt::init(mw);
}

int uicMain::exec()
{
//    toplevel().raise();
    SoQt::mainLoop();
    return 0;
}
