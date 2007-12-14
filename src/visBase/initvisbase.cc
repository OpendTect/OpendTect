/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: initvisbase.cc,v 1.1 2007-12-14 05:10:16 cvssatyaki Exp $";


#include "initvisbase.h"

#include "visanchor.h"
#include "visannot.h"
#include "visboxdragger.h"
#include "viscamera.h"
#include "viscamerainfo.h"
#include "viscolorseq.h"
#include "viscolortab.h"
#include "viscoltabmod.h"
#include "viscoord.h"
#include "viscube.h"
#include "viscubicbeziercurve.h"
#include "viscubicbeziersurface.h"
#include "visdatagroup.h"
#include "visdepthtabplanedragger.h"
#include "visdragger.h"
#include "visdrawstyle.h"
#include "visellipsoid.h"
#include "visevent.h"
#include "visfaceset.h"
#include "visflatviewer.h"
#include "visforegroundlifter.h"
#include "visgeomindexedshape.h"
#include "visgridlines.h"
#include "visimage.h"
#include "vislevelofdetail.h"
#include "vislight.h"
#include "vismarchingcubessurface.h"
#include "visinvisiblelinedragger.h"
#include "vismarker.h"
#include "vismaterial.h"
#include "vismultitexture2.h"
#include "visnormals.h"
#include "visparametricsurface.h"
#include "vispickstyle.h"
#include "vispointset.h"
#include "vispolygonoffset.h"
#include "vispolyline.h"
#include "visrandomtrack.h"
#include "visrandomtrackdragger.h"
#include "visrectangle.h"
#include "visrotationdragger.h"
#include "visscene.h"
#include "visshapescale.h"
#include "vistext.h"
#include "vistexture2.h"
#include "vistexture3.h"
#include "vistexture3viewer.h"
#include "vistexturecoords.h"
#include "vistexturerect.h"
#include "visthread.h"
#include "vistransform.h"
#include "vistristripset.h"
#include "visvolobliqueslice.h"
#include "visvolorthoslice.h"
#include "visvolren.h"
#include "visvolrenscalarfield.h"
#include "viswell.h"


namespace visBase
{

void initStdClasses()
{
    visBase::Anchor::initClass();
    visBase::Annotation::initClass();
    visBase::BoxDragger::initClass();
    visBase::Camera::initClass();
    visBase::CameraInfo::initClass();
    visBase::ColorSequence::initClass();
    visBase::VisColorTab::initClass();
    visBase::VisColTabMod::initClass();
    visBase::Coordinates::initClass();
    visBase::Cube::initClass();
    visBase::CubicBezierCurve::initClass();
    visBase::CubicBezierSurface::initClass();
    visBase::DataObjectGroup::initClass();
    visBase::DepthTabPlaneDragger::initClass();
    visBase::Dragger::initClass();
    visBase::DrawStyle::initClass();
    visBase::Ellipsoid::initClass();
    visBase::EventCatcher::initClass();
    visBase::FaceSet::initClass();
    visBase::FlatViewer::initClass();
    visBase::ForegroundLifter::initClass();
    visBase::GeomIndexedShape::initClass();
    visBase::GridLines::initClass();
    visBase::Image::initClass();
    visBase::InvisibleLineDragger::initClass();
    visBase::LevelOfDetail::initClass();
    visBase::PointLight::initClass();
    visBase::DirectionalLight::initClass();
    visBase::SpotLight::initClass();
    visBase::MarchingCubesSurface::initClass();
    visBase::Marker::initClass();
    visBase::Material::initClass();
    visBase::MultiTexture2::initClass();
    visBase::Normals::initClass();
    visBase::ParametricSurface::initClass();
    visBase::PickStyle::initClass();
    visBase::PointSet::initClass();
    visBase::PolygonOffset::initClass();
    visBase::PolyLine::initClass();
    visBase::IndexedPolyLine::initClass();
    visBase::IndexedPolyLine3D::initClass();
    visBase::RandomTrack::initClass();
    visBase::RandomTrackDragger::initClass();
    visBase::RectangleDragger::initClass();
    visBase::Rectangle::initClass();
    visBase::RotationDragger::initClass();
    visBase::Scene::initClass();
    visBase::ShapeScale::initClass();
    visBase::Text2::initClass();
    visBase::TextBox::initClass();
    visBase::Texture2::initClass();
    visBase::Texture2Set::initClass();
    visBase::Texture3::initClass();
    visBase::Texture3Viewer::initClass();
    visBase::MovableTextureSlice::initClass();
    visBase::Texture3Slice::initClass();
    visBase::TextureCoords::initClass();
    visBase::TextureRect::initClass();
    visBase::ThreadWorker::initClass();
    visBase::Transformation::initClass();
    visBase::Rotation::initClass();
    visBase::TriangleStripSet::initClass();
    visBase::ObliqueSlice::initClass();
    visBase::OrthogonalSlice::initClass();
    visBase::VolrenDisplay::initClass();
    visBase::VolumeRenderScalarField::initClass();
    visBase::Well::initClass();
}

}; // namespace visBase
