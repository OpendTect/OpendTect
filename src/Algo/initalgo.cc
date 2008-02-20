/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne and Kristofer
 Date:          December 2007
 RCS:           $Id: initalgo.cc,v 1.6 2008-02-20 12:44:02 cvsbert Exp $
________________________________________________________________________

-*/

#include "initalgo.h"
#include "windowfunction.h"
#include "gridding.h"
#include "posrandomfilter.h"

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

    Pos::RandomFilter3D::initClass();
    Pos::RandomFilter2D::initClass();
}
