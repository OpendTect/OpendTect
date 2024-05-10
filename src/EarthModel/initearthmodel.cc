/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "embodytr.h"
#include "emfaultstickset.h"
#include "emfault3d.h"
#include "emfaultset3d.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emhorizonztransform.h"
#include "emmarchingcubessurface.h"
#include "emobject.h"
#include "empolygonbody.h"
#include "emrandomposbody.h"
#include "emsurfaceposprov.h"
#include "emsurfacetr.h"
#include "genc.h"
#include "horizongridder.h"
#include "lmkemfaulttransl.h"
#include "moddepmgr.h"
#include "uistrings.h"
#include "zdomain.h"

using boolFromVoidFn = bool(*)();
using boolFromStringFn = bool(*)(uiString&);
mGlobal(General) void setConvBody_General_Fns(boolFromVoidFn,boolFromStringFn);

extern bool EM_Get_Body_Conversion_Status();
extern bool EM_Convert_Body_To_OD5(uiString&);

namespace EM
{

static bool Get_Body_Conversion_Status()
{
    return EM_Get_Body_Conversion_Status();
}

static bool Convert_Body_To_OD5( uiString& msg )
{
    return EM_Convert_Body_To_OD5( msg );
}

} // namespace EM


mDefModInitFn(EarthModel)
{
    mIfNotFirstTime( return );

    EMFault3DTranslatorGroup::initClass();
    EMFaultSet3DTranslatorGroup::initClass();
    EMFaultStickSetTranslatorGroup::initClass();
    EMBodyTranslatorGroup::initClass();
    EMHorizon3DTranslatorGroup::initClass();
    EMHorizon2DTranslatorGroup::initClass();
    EMAnyHorizonTranslatorGroup::initClass();

    dgbEMHorizon3DTranslator::initClass();
    dgbEMHorizon2DTranslator::initClass();
    dgbEMFault3DTranslator::initClass();
    dgbEMFaultSet3DTranslator::initClass();
    lmkEMFault3DTranslator::initClass();
    dgbEMFaultStickSetTranslator::initClass();

    odEMBodyTranslator::initClass();
    mcEMBodyTranslator::initClass();
    polygonEMBodyTranslator::initClass();
    randposEMBodyTranslator::initClass();

    EM::FaultStickSet::initClass();
    EM::Fault3D::initClass();
    EM::FaultSet3D::initClass();
    EM::Horizon2D::initClass();
    EM::Horizon3D::initClass();
    EM::HorizonZTransform::initClass();
    EM::MarchingCubesSurface::initClass();
    EM::RandomPosBody::initClass();
    EM::PolygonBody::initClass();
    Pos::EMSurfaceProvider3D::initClass();
    Pos::EMSurfaceProvider2D::initClass();
    Pos::EMImplicitBodyProvider::initClass();

    InvDistHor3DGridder::initClass();
    TriangulationHor3DGridder::initClass();
    ExtensionHor3DGridder::initClass();
    ContinuousCurvatureHor3DGridder::initClass();

    ZDomain::Def::add( new ZDomain::Def("Time-Flattened",uiStrings::sTime(),
					"ms",1000) );
    ZDomain::Def::add( new ZDomain::Def("Depth-Flattened",uiStrings::sDepth(),
					"",1) );

    if ( NeedDataBase() )
    {
	setConvBody_General_Fns( EM::Get_Body_Conversion_Status,
				 EM::Convert_Body_To_OD5 );
    }

    EM::addPluginTranslators();
}
