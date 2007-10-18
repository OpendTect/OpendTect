/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: SoOD.cc,v 1.14 2007-10-18 14:15:52 cvskris Exp $";


#include "SoOD.h"

#include <VolumeViz/nodes/SoVolumeRendering.h>
#include "Inventor/nodes/SoShaderProgram.h"
#include "Inventor/nodes/SoFragmentShader.h"

#include "SoArrow.h"
#include "SoCameraInfo.h"
#include "SoCameraInfoElement.h"
#include "SoCameraFacingZAxisRotation.h"
#include "SoColTabMultiTexture2.h"
#include "SoDepthTabPlaneDragger.h"
#include "SoForegroundTranslation.h"
#include "SoIndexedTriangleFanSet.h"
#include "SoLODMeshSurface.h"
#include "SoManLevelOfDetail.h"
#include "SoMFImage.h"
#include "SoInvisibleLineDragger.h"
#include "SoPerspectiveSel.h"
#include "SoPlaneWellLog.h"
#include "SoIndexedLineSet3D.h"
#include "SoShapeScale.h"
#include "SoText2Set.h"
#include "SoTranslateRectangleDragger.h"
#include "SoShaderTexture2.h"
#include "SoRandomTrackLineDragger.h"
#include "SoGridSurfaceDragger.h"
#include "UTMCamera.h"
#include "UTMElement.h"
#include "UTMPosition.h"

#ifndef win
extern "C" { extern void* glXGetCurrentContext(); }
#endif

void SoOD::init()
{
    SoVolumeRendering::init();

    //Elements first 
    SoCameraInfoElement::initClass();
    UTMElement::initClass();

    //Then fields
    SoMFImage::initClass();

    //Then nodes
    SoArrow::initClass();
    SoCameraInfo::initClass();
    SoCameraFacingZAxisRotation::initClass();
    SoColTabMultiTexture2::initClass();
    SoDepthTabPlaneDragger::initClass();
    SoForegroundTranslation::initClass();
    SoIndexedTriangleFanSet::initClass();
    SoLODMeshSurface::initClass();
    SoManLevelOfDetail::initClass();
    SoInvisibleLineDragger::initClass();
    SoPerspectiveSel::initClass();
    SoPlaneWellLog::initClass();
    SoIndexedLineSet3D::initClass();
    SoShapeScale::initClass();
    SoText2Set::initClass();
    SoTranslateRectangleDragger::initClass();
    SoRandomTrackLineDragger::initClass();
    SoShaderTexture2::initClass();
    SoGridSurfaceDragger::initClass();
    UTMCamera::initClass();
    UTMPosition::initClass();
}


int SoOD::supportsFragShading()
{
    static int answer = 0;
    if ( !answer )
    {
#ifdef win
	if ( wglGetCurrentContext() )
#else
	if ( glXGetCurrentContext() )
#endif
	    answer = SoFragmentShader::isSupported(
		    		SoShaderObject::GLSL_PROGRAM) ? 1 : -1;
    }

    return answer;
}
