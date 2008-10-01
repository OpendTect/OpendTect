/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki	
 Date:          December 2007
 RCS:           $Id: initearthmodel.cc,v 1.5 2008-10-01 03:44:37 cvsnanne Exp $
________________________________________________________________________

-*/


#include "initearthmodel.h"
#include "emfault2d.h"
#include "emfault3d.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emmarchingcubessurface.h"
#include "emhorizonztransform.h"
#include "emobject.h"
#include "empolygonbody.h"
#include "emsurfaceiodata.h"
#include "emsurfacetr.h"
#include "emsurfaceposprov.h"


void EarthModel::initStdClasses()
{
    EM::Fault2D::initClass();
    EM::Fault3D::initClass();
    EM::Horizon2D::initClass();
    EM::Horizon3D::initClass();
    EM::HorizonZTransform::initClass();
    EM::MarchingCubesSurface::initClass();
    EM::PolygonBody::initClass();
    Pos::EMSurfaceProvider3D::initClass();
    Pos::EMSurfaceProvider2D::initClass();
}

