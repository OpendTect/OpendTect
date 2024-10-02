/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visimagerect.h"

#include "pickset.h"
#include "odimage.h"

#include "viscoord.h"
#include "vismaterial.h"

#include <osgGeo/TexturePlane>
#include <osgGeo/LayeredTexture>
#include <osg/Image>
#include <osgDB/ReadFile>


mCreateFactoryEntry( visBase::ImageRect );

namespace visBase
{

ImageRect::ImageRect()
    : VisualObjectImpl(true)
    , laytex_(new osgGeo::LayeredTexture)
    , texplane_(new osgGeo::TexturePlaneNode)
{
    ref();
    refOsgPtr( laytex_ );
    refOsgPtr( texplane_ );
    layerid_ = laytex_->addDataLayer();
    laytex_->addProcess( new osgGeo::IdentityLayerProcess(*laytex_, layerid_) );
    texplane_->setLayeredTexture( laytex_ );

    polyoffset_ = PolygonOffset::create();
    polyoffset_->setFactor( -1.0f );
    polyoffset_->setUnits( 1.0f );

    polyoffset_->setMode( PolygonOffset::Protected | PolygonOffset::On	);
    polyoffset_->attachStateSet( texplane_->getOrCreateStateSet() );
    addChild( texplane_ );
    getMaterial()->setTransparency( 0.0 );
    unRefNoDelete();
}


ImageRect::~ImageRect()
{
    unRefOsgPtr( laytex_ );
    unRefOsgPtr( texplane_ );
}


void ImageRect::setPick( const Pick::Location& loc )
{
    setCenterPos( loc.pos() );
    const osg::Quat rot( loc.dir().phi, osg::Vec3(0,0,1) );
    texplane_->setRotation( rot );
}


void ImageRect::setCenterPos( const Coord3& pos )
{
    Coord3 newpos;
    Transformation::transform( trans_, pos, newpos );
    texplane_->setCenter( osg::Vec3(newpos.x_, newpos.y_, newpos.z_) );
}


void ImageRect::setCornerPos( const Coord3& tl, const Coord3& br )
{
    const Coord3 diff = tl - br;
    osg::Vec3 width( diff.x_, diff.y_, diff.z_ );
    texplane_->setWidth( width );
}


void ImageRect::setDisplayTransformation( const mVisTrans* trans )
{
    trans_ = trans;
}


void ImageRect::setRGBImage( const OD::RGBImage& rgbimg )
{
    if ( !rgbimg.getData() )
	return;

    const int totsz = rgbimg.getSize(true) * rgbimg.getSize(false) * 4;
    unsigned char* imgdata = new unsigned char[totsz];
    OD::memCopy( imgdata, rgbimg.getData(), totsz );

    osg::ref_ptr<osg::Image> image = new osg::Image;
    image->setImage( rgbimg.getSize(true), rgbimg.getSize(false), 1,
		     GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE, imgdata,
		     osg::Image::NO_DELETE );
    image->flipHorizontal();
    image->flipVertical();
    laytex_->setDataLayerImage( layerid_, image );
    texplane_->setTextureBrickSize( laytex_->maxTextureSize() );
}


} // namespace visBase
