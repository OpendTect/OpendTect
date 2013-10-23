/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Jan 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "vistexturerect.h"
#include "coord.h"
#include "coord.h"
#include "vistexturechannels.h"


#include <osgGeo/TexturePlane>

mCreateFactoryEntry( visBase::TextureRectangle );

using namespace visBase;


TextureRectangle::TextureRectangle()
    : VisualObjectImpl( false )
    , textureplane_( new osgGeo::TexturePlaneNode )
    , displaytrans_( 0 )
    , spanvec0_( 0.0, 0.0, 0.0 )
    , spanvec1_( 0.0, 0.0, 0.0 )
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


visBase::TextureChannels* TextureRectangle::getTextureChannels()
{ return channels_; }


void TextureRectangle::freezeDisplay( bool yn )
{ textureplane_->freezeDisplay( yn ); }


bool TextureRectangle::isDisplayFrozen() const
{ return textureplane_->isDisplayFrozen(); }


void TextureRectangle::setTextureShift( const Coord& shift )
{ textureplane_->setTextureShift( Conv::to<osg::Vec2>(shift) ); }


Coord TextureRectangle::getTextureShift() const
{ return Conv::to<Coord>( textureplane_->getTextureShift() ); }


void TextureRectangle::setTextureGrowth( const Coord& growth )
{ textureplane_->setTextureGrowth( Conv::to<osg::Vec2>(growth) ); }


Coord TextureRectangle::getTextureGrowth() const
{ return Conv::to<Coord>( textureplane_->getTextureGrowth() ); }


void TextureRectangle::setCenter( const Coord3& center )
{
    osg::Vec3 osgcenter;
    mVisTrans::transform( displaytrans_, center, osgcenter );
    textureplane_->setCenter( osgcenter );
}


Coord3 TextureRectangle::getCenter() const
{
    Coord3 res;
    mVisTrans::transformBack( displaytrans_, textureplane_->getCenter(), res );
    return res;
}


void TextureRectangle::setWidth( const Coord3& width )
{
    osg::Vec3f osgwidth;
    mVisTrans::transformDir( displaytrans_, width, osgwidth );
    textureplane_->setWidth( osgwidth );
}


Coord3 TextureRectangle::getWidth() const
{
    Coord3 res;
    mVisTrans::transformBackDir( displaytrans_, textureplane_->getWidth(), res);
    return res;
}


const Coord3& TextureRectangle::getSpanningVector( int idx ) const
{
    return idx<1 ? spanvec0_ : spanvec1_;
}


void TextureRectangle::setRotation( const Coord3& spanvec0,
				    const Coord3& spanvec1 )
{
    osg::Vec3 v0, v1;
    mVisTrans::transformDir( displaytrans_, spanvec0, v0 );
    mVisTrans::transformDir( displaytrans_, spanvec1, v1 );

    if ( !v0.length() || !v1.length() )
	return;

    v0.normalize();
    v1.normalize();
    const osg::Vec3 v2 = v0 ^ v1;

    if ( abs(v0*v1) > 1e-3 )
	pErrMsg( "Rectangle vectors expected to be orthogonal" );

    const osg::Matrix mat( v0[0], v1[0], v2[0], 0.0,
			   v0[1], v1[1], v2[1], 0.0,
			   v0[2], v1[2], v2[2], 0.0,
			     0.0,   0.0,   0.0, 1.0 );

    textureplane_->setRotation( osg::Matrix::inverse(mat).getRotate() );

    spanvec0_ = spanvec0;
    spanvec1_ = spanvec1;
}


void TextureRectangle::setRotationAndWidth( const Coord3& spanvec0,
					    const Coord3& spanvec1 )
{
    setRotation( spanvec0, spanvec1 );

    osg::Vec3 v0, v1;
    mVisTrans::transformDir( displaytrans_, spanvec0, v0 );
    mVisTrans::transformDir( displaytrans_, spanvec1, v1 );

    textureplane_->setWidth( osg::Vec3(v0.length(), v1.length(), 0.0) );
}


void TextureRectangle::swapTextureAxes( bool yn )
{
    textureplane_->swapTextureAxes( yn );
}


bool TextureRectangle::areTextureAxesSwapped() const
{
    return textureplane_->areTextureAxesSwapped();
}


void TextureRectangle::setDisplayTransformation( const mVisTrans* tr )
{
    if ( tr == displaytrans_ )
	return;

    const Coord3 center = getCenter();
    const Coord3 width = getWidth();

    displaytrans_ = tr;

    setCenter( center );
    setWidth( width );
    setRotation( spanvec0_, spanvec1_ );
}


const mVisTrans* TextureRectangle::getDisplayTransformation() const
{ return displaytrans_; }
