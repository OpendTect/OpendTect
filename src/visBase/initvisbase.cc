/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: initvisbase.cc,v 1.21 2011-02-14 23:21:16 cvskris Exp $";


#include "initvisbase.h"

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


namespace visBase
{

void initStdClasses()
{
    Anchor::initClass();
    Annotation::initClass();
    BeachBall::initClass();
    BoxDragger::initClass();
    Camera::initClass();
    CameraInfo::initClass();
    ColorSequence::initClass();
    VisColorTab::initClass();
    VisColTabMod::initClass();
    Coordinates::initClass();
    Cube::initClass();
    DataObjectGroup::initClass();
    DepthTabPlaneDragger::initClass();
    Dragger::initClass();
    DrawStyle::initClass();
    Ellipsoid::initClass();
    EventCatcher::initClass();
    FaceSet::initClass();
    FlatViewer::initClass();
    ForegroundLifter::initClass();
    GeomIndexedShape::initClass();
    GridLines::initClass();
    HorizonSection::initClass();
    Image::initClass();
    InvisibleLineDragger::initClass();
    LevelOfDetail::initClass();
    PointLight::initClass();
    DirectionalLight::initClass();
    SpotLight::initClass();
    MarchingCubesSurface::initClass();
    Marker::initClass();
    Material::initClass();
    MultiTexture2::initClass();
    Normals::initClass();
    PickStyle::initClass();
    RandomPos2Body::initClass();
    PointSet::initClass();
    PolygonOffset::initClass();
    PolygonSelection::initClass();
    PolyLine::initClass();
    IndexedPolyLine::initClass();
    IndexedPolyLine3D::initClass();
    RandomTrack::initClass();
    RandomTrackDragger::initClass();
    RectangleDragger::initClass();
    Rectangle::initClass();
    RotationDragger::initClass();
    Scene::initClass();
    SceneColTab::initClass();
    ShapeHints::initClass();
    ShapeScale::initClass();
    SplitTextureRandomLine::initClass();
    SplitTexture2Rectangle::initClass();
    SplitTextureSeis2D::initClass();
    Text2::initClass();
    TextBox::initClass();
    Texture2::initClass();
    Texture2Set::initClass();
    Texture3::initClass();
    Texture3Viewer::initClass();
    MovableTextureSlice::initClass();
    Texture3Slice::initClass();
    TextureCoords::initClass();
    TextureChannels::initClass();
    ColTabTextureChannel2RGBA::initClass();
    TextureChannel2VolData::initClass();
    TextureRect::initClass();
    Transformation::initClass();
    Rotation::initClass();
    TriangleStripSet::initClass();
    ObliqueSlice::initClass();
    OrthogonalSlice::initClass();
    RGBATextureChannel2RGBA::initClass();
    VolrenDisplay::initClass();
    VolumeRenderScalarField::initClass();
    Well::initClass();
    TopBotImage::initClass();
}

}; // namespace visBase
