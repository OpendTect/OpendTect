/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki	
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "moddepmgr.h"
#include "embodytr.h"
#include "emfaultstickset.h"
#include "emfault3d.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emhorizonztransform.h"
#include "emmarchingcubessurface.h"
#include "emobject.h"
#include "empolygonbody.h"
#include "emrandomposbody.h"
#include "emsurfaceiodata.h"
#include "emsurfaceposprov.h"
#include "emsurfacetr.h"
#include "lmkemfaulttransl.h"
#include "zdomain.h"

mDefModInitFn(EarthModel)
{
    mIfNotFirstTime( return );
    
    EMFault3DTranslatorGroup::initClass();
    EMFaultStickSetTranslatorGroup::initClass();
    EMBodyTranslatorGroup::initClass();
    EMHorizon3DTranslatorGroup::initClass();
    EMHorizon2DTranslatorGroup::initClass();
    EMAnyHorizonTranslatorGroup::initClass();
    
    dgbEMHorizon3DTranslator::initClass();
    dgbEMHorizon2DTranslator::initClass();
    dgbEMFault3DTranslator::initClass();
    lmkEMFault3DTranslator::initClass();
    dgbEMFaultStickSetTranslator::initClass();
    
    mcEMBodyTranslator::initClass();
    polygonEMBodyTranslator::initClass();
    randposEMBodyTranslator::initClass();
    dGBEMBodyTranslator::initClass();
    
    EM::FaultStickSet::initClass();
    EM::Fault3D::initClass();
    EM::Horizon2D::initClass();
    EM::Horizon3D::initClass();
    EM::HorizonZTransform::initClass();
    EM::MarchingCubesSurface::initClass();
    EM::RandomPosBody::initClass();
    EM::PolygonBody::initClass();
    Pos::EMSurfaceProvider3D::initClass();
    Pos::EMSurfaceProvider2D::initClass();
    Pos::EMImplicitBodyProvider::initClass();

    ZDomain::Def::add( new ZDomain::Def("Time-Flattened","Time","ms",1000) );
    ZDomain::Def::add( new ZDomain::Def("Depth-Flattened","Depth","",1) );
}
