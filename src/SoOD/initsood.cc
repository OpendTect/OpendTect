/*+
 ________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne and Kristofer
 Date:          December 2007
 ________________________________________________________________________

-*/
static const char* rcsID = "$Id: initsood.cc,v 1.14 2009-03-21 02:05:05 cvskris Exp $";

#include <VolumeViz/nodes/SoVolumeRendering.h>

#include "initsood.h"
#include "SoArrow.h"
#include "SoCameraInfo.h"
#include "SoCameraInfoElement.h"
#include "SoCameraFacingZAxisRotation.h"
#include "SoColTabMultiTexture2.h"
#include "SoColTabTextureChannel2RGBA.h"
#include "SoDepthTabPlaneDragger.h"
#include "SoForegroundTranslation.h"
#include "SoIndexedPointSet.h"
#include "SoIndexedTriangleFanSet.h"
#include "SoLODMeshSurface.h"
#include "SoManLevelOfDetail.h"
#include "SoMFImage.h"
#include "SoInvisibleLineDragger.h"
#include "SoPerspectiveSel.h"
#include "SoPlaneWellLog.h"
#include "SoPolygonSelect.h"
#include "SoIndexedLineSet3D.h"
#include "SoLockableSeparator.h"
#include "SoRGBATextureChannel2RGBA.h"
#include "SoScale3Dragger.h"
#include "SoShapeScale.h"
#include "SoText2Set.h"
#include "SoTranslateRectangleDragger.h"
#include "SoShaderTexture2.h"
#include "SoSplitTexture2.h"
#include "SoSplitTexture2Element.h"
#include "SoTextureAnisotropicFilterElement.h"
#include "SoTextureComposer.h"
#include "SoTextureComposerElement.h"
#include "SoTextureChannelSet.h"
#include "SoTextureChannelSetElement.h"
#include "SoTextureMipMapBiasElement.h"
#include "SoRandomTrackLineDragger.h"
#include "SoGridSurfaceDragger.h"
#include "UTMCamera.h"
#include "UTMElement.h"
#include "UTMPosition.h"
#include "ViewportRegion.h"
#include "LegendKit.h"


void SoOD::initStdClasses()
{
    SoVolumeRendering::init();

    //Elements first 
    SoCameraInfoElement::initClass();
    UTMElement::initClass();
    SoSplitTexture2Element::initClass();
    SoTextureComposerElement::initClass();
    SoTextureChannelSetElement::initClass();
    SoTextureMipMapBiasElement::initClass();
    SoTextureAnisotropicFilterElement::initClass();

    //Then fields
    SoMFImage::initClass();

    //Then nodes
    SoArrow::initClass();
    SoCameraInfo::initClass();
    SoCameraFacingZAxisRotation::initClass();
    SoColTabMultiTexture2::initClass();
    SoColTabTextureChannel2RGBA::initClass();
    SoDepthTabPlaneDragger::initClass();
    SoForegroundTranslation::initClass();
    SoIndexedPointSet::initClass();
    SoIndexedTriangleFanSet::initClass();
    SoLODMeshSurface::initClass();
    SoManLevelOfDetail::initClass();
    SoInvisibleLineDragger::initClass();
    SoPerspectiveSel::initClass();
    SoPlaneWellLog::initClass();
    SoPolygonSelect::initClass();
    SoIndexedLineSet3D::initClass();
    SoLockableSeparator::initClass();
    SoShapeScale::initClass();
    SoText2Set::initClass();
    SoTextureChannelSet::initClass();
    SoTranslateRectangleDragger::initClass();
    SoRandomTrackLineDragger::initClass();
    SoRGBATextureChannel2RGBA::initClass();
    SoScale3Dragger::initClass();
    SoShaderTexture2::initClass();
    SoSplitTexture2::initClass();
    SoSplitTexture2Part::initClass();
    SoGridSurfaceDragger::initClass();
    UTMCamera::initClass();
    UTMPosition::initClass();
    ViewportRegion::initClass();
    LegendKit::initClass();
    SoTextureComposer::initClass();
    SoTextureComposerInfo::initClass();
}
