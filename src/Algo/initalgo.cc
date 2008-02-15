/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne and Kristofer
 Date:          December 2007
 RCS:           $Id: initalgo.cc,v 1.5 2008-02-15 20:29:24 cvskris Exp $
________________________________________________________________________

-*/

#include "initalgo.h"
#include "windowfunction.h"
#include "gridding.h"

void Algo::initStdClasses()
{
    BoxWindow::initClass();
    HammingWindow::initClass();
    HanningWindow::initClass();
    BlackmanWindow::initClass();
    BartlettWindow::initClass();
    CosTaperWindow::initClass();

    TriangulatedGridding::initClass();
    InverseDistanceGridding::initClass();
}
