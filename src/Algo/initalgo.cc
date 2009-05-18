/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne and Kristofer
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: initalgo.cc,v 1.16 2009-05-18 21:21:49 cvskris Exp $";

#include "initalgo.h"

#include "fft.h"
#include "gridder2d.h"
#include "array2dinterpolimpl.h"
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
//TriangulatedNeighborhoodGridder2D::initClass();
//Not good enough for production
    TriangulatedGridder2D::initClass();

    Pos::RandomFilter3D::initClass();
    Pos::RandomFilter2D::initClass();
    Pos::SubsampFilter3D::initClass();
    Pos::SubsampFilter2D::initClass();

    InverseDistanceArray2DInterpol::initClass();
    TriangulationArray2DInterpol::initClass();
}
