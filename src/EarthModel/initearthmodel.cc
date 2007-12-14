/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki	
 Date:          December 2007
 RCS:           $Id: initearthmodel.cc,v 1.1 2007-12-14 05:10:16 cvssatyaki Exp $
________________________________________________________________________

-*/


#include "initearthmodel.h"
#include "emfault.h"
#include "emhorizon3d.h"
#include "emhorizon2d.h"
#include "emmarchingcubessurface.h"
#include "emhorizonztransform.h"
#include "emobject.h"
#include "emsurfaceiodata.h"
#include "emsurfacetr.h"


void EarthModel::initStdClasses()
{
    EM::Fault::initClass();
    EM::Horizon2D::initClass();
    EM::Horizon3D::initClass();
    EM::HorizonZTransform::initClass();
    EM::MarchingCubesSurface::initClass();
}

