/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne and Kristofer
 Date:          December 2007
________________________________________________________________________

-*/

#include "moddepmgr.h"

#include "array2dinterpolimpl.h"
#include "fourier.h"
#include "gridder2d.h"
#include "interpollayermodel.h"
#include "posfilterstd.h"
#include "raytrace1d.h"
#include "reflcalc1d.h"
#include "statrand.h"
#include "checksum.h"
#include "windowfunction.h"

mDefModInitFn(Algo)
{
    mIfNotFirstTime( return );

    WindowFunction::addAllStdClasses();

    InverseDistanceGridder2D::initClass();
    TriangulatedGridder2D::initClass();
    RadialBasisFunctionGridder2D::initClass();

    Pos::RandomFilter3D::initClass();
    Pos::RandomFilter2D::initClass();
    Pos::SubsampFilter3D::initClass();
    Pos::SubsampFilter2D::initClass();

    InverseDistanceArray2DInterpol::initClass();
    TriangulationArray2DInterpol::initClass();
    ExtensionArray2DInterpol::initClass();

    Fourier::CC::initClass();

    AICalc1D::initClass();
    VrmsRayTracer1D::initClass();
    ZSliceInterpolationModel::initClass();

    FactoryBase& reflfact = ReflCalc1D::factory();
    BufferString defnm = reflfact.getDefaultName();
    if ( defnm.isEmpty() )
    {
	const int defidx = reflfact.getNames().indexOf(
			   AICalc1D::sFactoryKeyword() );
	reflfact.setDefaultName( defidx );
    }

    FactoryBase& rtfact = RayTracer1D::factory();
    defnm = rtfact.getDefaultName();
    if ( defnm.isEmpty() )
    {
	const int defidx = rtfact.getNames().indexOf(
			   VrmsRayTracer1D::sFactoryKeyword() );
	rtfact.setDefaultName( defidx );
    }

    initChecksum();
}
