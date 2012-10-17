/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne and Kristofer
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "moddepmgr.h"
#include "gridder2d.h"
#include "array2dinterpolimpl.h"
#include "posfilterstd.h"
#include "windowfunction.h"
#include "fourier.h"
#include "raytrace1d.h"
#include "statrand.h"

mDefModInitFn(Algo)
{
    mIfNotFirstTime( return );

    Stats::randGen().init();

    WindowFunction::addAllStdClasses();

    InverseDistanceGridder2D::initClass();
    TriangulatedGridder2D::initClass();

    Pos::RandomFilter3D::initClass();
    Pos::RandomFilter2D::initClass();
    Pos::SubsampFilter3D::initClass();
    Pos::SubsampFilter2D::initClass();

    InverseDistanceArray2DInterpol::initClass();
    TriangulationArray2DInterpol::initClass();
    Array2DInterpolExtension::initClass();

    Fourier::CC::initClass();

    VrmsRayTracer1D::initClass();
}
