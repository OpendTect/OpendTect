/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID mUsedVar = "$Id$";



#include "moddepmgr.h"
#include "visanchor.h"
#include "visannot.h"
#include "visbeachball.h"
#include "visboxdragger.h"
#include "viscamera.h"
#include "viscamerainfo.h"
#include "viscolorseq.h"
#include "viscolortab.h"
#include "viscoltabmod.h"
#include "viscoord.h"
#include "viscube.h"
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
#include "vishorizonsection.h"
#include "visimage.h"
#include "vislevelofdetail.h"
#include "vislight.h"
#include "vismarchingcubessurface.h"
#include "visinvisiblelinedragger.h"
#include "vismarker.h"
#include "vismaterial.h"
#include "vismultitexture2.h"
#include "visnormals.h"
#include "vissplittexturerandomline.h"
#include "vissplittexture2rectangle.h"
#include "vissplittextureseis2d.h"
#include "vispickstyle.h"
#include "visrandompos2body.h"
#include "vispointset.h"
#include "vispolygonoffset.h"
#include "vispolygonselection.h"
#include "vispolyline.h"
#include "visrandomtrack.h"
#include "visrandomtrackdragger.h"
#include "visrgbatexturechannel2rgba.h"
#include "vistexturechannel2voldata.h"
#include "visrectangle.h"
#include "visrotationdragger.h"
#include "visscene.h"
#include "visscenecoltab.h"
#include "visshapehints.h"
#include "visshapescale.h"
#include "vistext.h"
#include "vistexture2.h"
#include "vistexture3.h"
#include "vistexture3viewer.h"
#include "vistexturecoords.h"
#include "vistexturerect.h"
#include "vistexturechannel2rgba.h"
#include "vistexturechannels.h"
#include "vistopbotimage.h"
#include "vistransform.h"
#include "vistristripset.h"
#include "visvolobliqueslice.h"
#include "visvolorthoslice.h"
#include "visvolren.h"
#include "visvolrenscalarfield.h"
#include "viswell.h"
#include "indexedshape.h"


mDefModInitFn(visBase)
{
    mIfNotFirstTime( return );

    visBase::Anchor::initClass();
    visBase::Annotation::initClass();
    visBase::BeachBall::initClass();
    visBase::BoxDragger::initClass();
    visBase::Camera::initClass();
    visBase::CameraInfo::initClass();
    visBase::ColorSequence::initClass();
    visBase::VisColorTab::initClass();
    visBase::VisColTabMod::initClass();
    visBase::Coordinates::initClass();
    visBase::Cube::initClass();
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
    visBase::HorizonSection::initClass();
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
    visBase::PickStyle::initClass();
    visBase::RandomPos2Body::initClass();
    visBase::PointSet::initClass();
    visBase::PolygonOffset::initClass();
    visBase::PolygonSelection::initClass();
    visBase::PolyLine::initClass();
    visBase::IndexedPolyLine::initClass();
    visBase::IndexedPolyLine3D::initClass();
    visBase::RandomTrack::initClass();
    visBase::RandomTrackDragger::initClass();
    visBase::RectangleDragger::initClass();
    visBase::Rectangle::initClass();
    visBase::RotationDragger::initClass();
    visBase::Scene::initClass();
    visBase::SceneColTab::initClass();
    visBase::ShapeHints::initClass();
    visBase::ShapeScale::initClass();
    visBase::SplitTextureRandomLine::initClass();
    visBase::SplitTexture2Rectangle::initClass();
    visBase::SplitTextureSeis2D::initClass();
    visBase::Text2::initClass();
    visBase::TextBox::initClass();
    visBase::Texture2::initClass();
    visBase::Texture2Set::initClass();
    visBase::Texture3::initClass();
    visBase::Texture3Viewer::initClass();
    visBase::MovableTextureSlice::initClass();
    visBase::Texture3Slice::initClass();
    visBase::TextureCoords::initClass();
    visBase::TextureCoords2::initClass();
    visBase::TextureChannels::initClass();
    visBase::ColTabTextureChannel2RGBA::initClass();
    visBase::TextureChannel2VolData::initClass();
    visBase::TextureRectangle::initClass();
    visBase::Transformation::initClass();
    visBase::Rotation::initClass();
    visBase::TriangleStripSet::initClass();
    visBase::ObliqueSlice::initClass();
    visBase::OrthogonalSlice::initClass();
    visBase::RGBATextureChannel2RGBA::initClass();
    visBase::VolrenDisplay::initClass();
    visBase::VolumeRenderScalarField::initClass();
    visBase::Well::initClass();
    visBase::TopBotImage::initClass();
    
    Geometry::PrimitiveSetCreator::setCreator(
				    new visBase::PrimitiveSetCreator );
}
