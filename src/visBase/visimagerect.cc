/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Feb 2007
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "visimagerect.h"

#include "viscoord.h"
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
{
    laytex_->ref();
    texplane_->ref();
    layerid_ = laytex_->addDataLayer();
    laytex_->addProcess( new osgGeo::IdentityLayerProcess(*laytex_, layerid_) );
    texplane_->setLayeredTexture( laytex_ );
    addChild( texplane_ );

    //setTransparency( 0.0 );
}


ImageRect::~ImageRect()
{
    if ( trans_ ) trans_->unRef();
    laytex_->unref();
    texplane_->unref();
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


void ImageRect::setFileName( const char* fnm )
{
    fnm_ = fnm;
    osg::ref_ptr<osg::Image> image = osgDB::readImageFile( fnm );
    imagedata_.width_ = image->s();
    imagedata_.height_ = image->t();
    imagedata_.depth_ = image->r();
    imagedata_.data_.setEmpty(); imagedata_.data_ = image->data();
    imagedata_.internalformat_ = image->getInternalTextureFormat();
    imagedata_.format_ = image->getPixelFormat();
    imagedata_.datatype_ = image->getDataType();
    imagedata_.packing_ = image->getPacking();
    laytex_->setDataLayerImage( layerid_, image );
    texplane_->setTextureBrickSize( laytex_->maxTextureSize() );
}


void ImageRect::setImageData( const ImageData& imgd )
{
    osg::ref_ptr<osg::Image> image = new osg::Image;
    image->setImage( imgd.width_, imgd.height_, imgd.depth_, 
		     imgd.internalformat_, imgd.format_, 
		     imgd.datatype_, mCast(unsigned char*,imgd.data_.buf()),
		     osg::Image::NO_DELETE, imgd.packing_ );
    imagedata_ = imgd;
    laytex_->setDataLayerImage( layerid_, image );
    texplane_->setTextureBrickSize( laytex_->maxTextureSize() );
}


ImageRect::ImageData ImageRect::getImageData() const
{
    return imagedata_;
}


const char* ImageRect::getFileName() const
{
   return fnm_;
}


}; // namespace visBase
