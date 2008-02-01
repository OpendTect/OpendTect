/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne and Kristofer
 Date:          December 2007
 RCS:           $Id: initalgo.cc,v 1.2 2008-02-01 14:02:15 cvsbert Exp $
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
    Pos::RectVolumeProvider::initClass();
}
