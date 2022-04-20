/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Feb 2007
________________________________________________________________________

-*/

#include "visimagerect.h"

#include "pickset.h"
#include "odimage.h"

#include "viscoord.h"
#include "vismaterial.h"
#include "vispolygonoffset.h"
#include "vistransform.h"

#include <osgGeo/TexturePlane>
#include <osgGeo/LayeredTexture>
#include <osg/Image>
#include <osgDB/ReadFile>


mCreateFactoryEntry( visBase::ImageRect );

namespace visBase
{

ImageRect::ImageRect()
    : VisualObjectImpl(true)
    , trans_(0)
    , laytex_(new osgGeo::LayeredTexture)
    , texplane_(new osgGeo::TexturePlaneNode)
    , polyoffset_(new visBase::PolygonOffset)
{
    laytex_->ref();
    texplane_->ref();
    layerid_ = laytex_->addDataLayer();
    laytex_->addProcess( new osgGeo::IdentityLayerProcess(*laytex_, layerid_) );
    texplane_->setLayeredTexture( laytex_ );

    polyoffset_->ref();
    polyoffset_->setFactor( -1.0f );
    polyoffset_->setUnits( 1.0f );

    polyoffset_->setMode(
	visBase::PolygonOffset::Protected | visBase::PolygonOffset::On  );
    polyoffset_->attachStateSet( texplane_->getOrCreateStateSet() );
    addChild( texplane_ );
    getMaterial()->setTransparency( 0.0 );
}


ImageRect::~ImageRect()
{
    if ( trans_ ) trans_->unRef();
    laytex_->unref();
    polyoffset_->unRef();
    texplane_->unref();
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
    texplane_->setCenter( osg::Vec3(newpos.x, newpos.y, newpos.z) );
}


void ImageRect::setCornerPos( const Coord3& tl, const Coord3& br )
{
    const Coord3 diff = tl - br;
    osg::Vec3 width( diff.x, diff.y, diff.z );
    texplane_->setWidth( width );
}


void ImageRect::setDisplayTransformation( const mVisTrans* trans )
{
    if ( trans_ ) trans_->unRef();
    trans_ = trans;
    if ( trans_ ) trans_->ref();
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


}; // namespace visBase
