/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne and Kristofer
 Date:          December 2007
 ________________________________________________________________________

-*/
static const char* rcsID = "$Id: initsood.cc,v 1.25 2010-11-18 08:43:05 cvskarthika Exp $";

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
#include "SoDGBIndexedPointSet.h"
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
#include "SoTextureComposer.h"
#include "SoTextureComposerElement.h"
#include "SoTextureChannelSet.h"
#include "SoTextureChannelSetElement.h"
#include "SoRandomTrackLineDragger.h"
#include "SoGridSurfaceDragger.h"
#include "UTMCamera.h"
#include "UTMElement.h"
#include "UTMPosition.h"
#include "ViewportRegion.h"
#include "LegendKit.h"
#include "SoAxes.h"
#include "SoBeachBall.h"
#include "SoDGBDragPointDragger.h"

void SoOD::initStdClasses()
{
    SoVolumeRendering::init();

    //Elements first 
    SoCameraInfoElement::initClass();
    UTMElement::initClass();
    SoSplitTexture2Element::initClass();
    SoTextureComposerElement::initClass();
    SoTextureChannelSetElement::initClass();

    //Then fields
    SoMFImagei32::initClass();

    //Then nodes
    SoArrow::initClass();
    SoCameraInfo::initClass();
    SoCameraFacingZAxisRotation::initClass();
    SoColTabMultiTexture2::initClass();
    SoColTabTextureChannel2RGBA::initClass();
    SoDepthTabPlaneDragger::initClass();
    SoForegroundTranslation::initClass();
    SoDGBIndexedPointSet::initClass();
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
    SoAxes::initClass();
    SoBeachBall::initClass();
    SoDGBDragPointDragger::initClass();
}
