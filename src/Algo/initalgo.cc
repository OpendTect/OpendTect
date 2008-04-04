/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne and Kristofer
 Date:          December 2007
 RCS:           $Id: initalgo.cc,v 1.9 2008-04-04 04:10:18 cvsnanne Exp $
________________________________________________________________________

-*/

#include "initalgo.h"

#include "fft.h"
#include "gridder2d.h"
#include "posfilterstd.h"
#include "windowfunction.h"

void Algo::initStdClasses()
{
    FFT::initClass();

    BartlettWindow::initClass();
    BoxWindow::initClass();
    CosTaperWindow::initClass();
    HammingWindow::initClass();
    HanningWindow::initClass();

    InverseDistanceGridder2D::initClass();
    TriangulatedGridder2D::initClass();

    Pos::RandomFilter3D::initClass();
    Pos::RandomFilter2D::initClass();
    Pos::SubsampFilter3D::initClass();
    Pos::SubsampFilter2D::initClass();
}
