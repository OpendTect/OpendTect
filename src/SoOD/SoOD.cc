/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: SoOD.cc,v 1.1 2004-12-08 10:24:22 kristofer Exp $";


#include "SoOD.h"

#include "SoArrow.h"
#include "SoCameraInfo.h"
#include "SoCameraInfoElement.h"
#include "SoDepthTabPlaneDragger.h"
#include "SoForegroundTranslation.h"
#include "SoIndexedTriangleFanSet.h"
#include "SoMeshSurface.h"
#include "SoMeshSurfaceBrick.h"
#include "SoMeshSurfaceBrickWire.h"
#include "SoMeshSurfaceSquare.h"
#include "SoManLevelOfDetail.h"
#include "SoPerspectiveSel.h"
#include "SoPlaneWellLog.h"
#include "SoIndexedLineSet3D.h"
#include "SoShapeScale.h"
#include "SoTranslateRectangleDragger.h"
#include "SoRandomTrackLineDragger.h"
#include "SoGridSurfaceDragger.h"
#include "UTMCamera.h"
#include "UTMElement.h"
#include "UTMPosition.h"

void SoOD::init()
{
    //Elements first 
    SoCameraInfoElement::initClass();
    UTMElement::initClass();

    //Then nodes
    SoArrow::initClass();
    SoCameraInfo::initClass();
    SoDepthTabPlaneDragger::initClass();
    SoForegroundTranslation::initClass();
    SoIndexedTriangleFanSet::initClass();
    SoMeshSurface::initClass();
    SoMeshSurfaceBrick::initClass();
    SoMeshSurfaceBrickWire::initClass();
    SoMeshSurfaceSquare::initClass();
    SoManLevelOfDetail::initClass();
    SoPerspectiveSel::initClass();
    SoPlaneWellLog::initClass();
    SoIndexedLineSet3D::initClass();
    SoShapeScale::initClass();
    SoTranslateRectangleDragger::initClass();
    SoRandomTrackLineDragger::initClass();
    SoGridSurfaceDragger::initClass();
    UTMCamera::initClass();
    UTMPosition::initClass();
}
