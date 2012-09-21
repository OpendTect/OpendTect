/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Jan 2002
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "vistexturerect.h"

#include "cubesampling.h"
#include "vistexturechannels.h"

#include <osgGeo/TexturePlane>

mCreateFactoryEntry( visBase::TextureRectangle );

namespace visBase
{

TextureRectangle::TextureRectangle()
    : VisualObjectImpl( false )
    , textureplane_( new osgGeo::TexturePlaneNode )
{
    textureplane_->ref();
    addChild( textureplane_ );
}


TextureRectangle::~TextureRectangle()
{
    textureplane_->unref();
}


void TextureRectangle::setTextureChannels( visBase::TextureChannels* channels )
{
    channels_ = channels;
    textureplane_->setLayeredTexture( channels_->getOsgTexture() );
}


void TextureRectangle::setCenter( const Coord3& center )
{
    textureplane_->setCenter( osg::Vec3f(center.x, center.y, center.z ) );
}


Coord3 TextureRectangle::getCenter() const
{
    const osg::Vec3f center = textureplane_->getCenter();
    return Coord3( center.x(), center.y(), center.z() );
}


void TextureRectangle::setWidth( const Coord3& width )
{
    textureplane_->setWidth( osg::Vec3f(width.x, width.y, width.z ) );
}


Coord3 TextureRectangle::getWidth() const
{
    const osg::Vec3f width = textureplane_->getWidth();
    return Coord3( width.x(), width.y(), width.z() );
}


void TextureRectangle::swapTextureAxes( bool yn )
{
    textureplane_->swapTextureAxes( yn );
}


bool TextureRectangle::areTextureAxesSwapped() const
{
    return textureplane_->areTextureAxesSwapped();
}


}; // namespace visBase
