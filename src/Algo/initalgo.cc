/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne and Kristofer
 Date:          December 2007
 RCS:           $Id: initalgo.cc,v 1.3 2008-02-04 16:22:59 cvsbert Exp $
________________________________________________________________________

-*/

#include "initalgo.h"
#include "windowfunction.h"
#include "posprovider.h"

void Algo::initStdClasses()
{
    BoxWindow::initClass();
    HammingWindow::initClass();
    HanningWindow::initClass();
    BlackmanWindow::initClass();
    BartlettWindow::initClass();
    CosTaperWindow::initClass();
    Pos::Rect3DProvider::initClass();
    Pos::Rect2DProvider::initClass();
}
