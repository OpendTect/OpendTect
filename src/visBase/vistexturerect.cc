/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Jan 2002
________________________________________________________________________

-*/

#include "vistexturerect.h"
#include "coord.h"
#include "coord.h"
#include "vistexturechannels.h"
#include "vistexturechannel2rgba.h"
#include "odversion.h"

#include <osgGeo/LayeredTexture>
#include <osgGeo/TexturePlane>
#include <osg/Geometry>
#include <osg/TexMat>

mCreateFactoryEntry( visBase::TextureRectangle )

namespace visBase
{

TextureRectangle::TextureRectangle()
    : VisualObjectImpl( false )
    , textureplane_( new osgGeo::TexturePlaneNode )
    , displaytrans_( 0 )
    , spanvec0_( 0.0, 0.0, 0.0 )
    , spanvec1_( 0.0, 0.0, 0.0 )
{
    textureplane_->ref();
    addChild( textureplane_ );
    setWidth( Coord3(0.0,0.0,0.0) );
}


TextureRectangle::~TextureRectangle()
{
    textureplane_->unref();
}


void TextureRectangle::setTextureChannels( visBase::TextureChannels* channels )
{
    channels_ = channels;
    textureplane_->setLayeredTexture( channels_->getOsgTexture() );

    const int maxtexsize = channels_->getOsgTexture()->maxTextureSize();
    textureplane_->setTextureBrickSize( maxtexsize, false );
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

    if ( fabs(v0*v1) > 1e-3 )
    {
	pErrMsg( "Rectangle vectors expected to be orthogonal" );
    }

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


void TextureRectangle::getTextureCoordinates( TypeSet<Coord3>& coords ) const
{
    Coord3 width = getWidth();
    Coord3 center = getCenter();

    char thindim = 0;
    if ( width.x == 0 )
	thindim = 0;
    else if ( width.y == 0 )
	thindim = 1;
    else thindim = 2;

    if ( displaytrans_ )
    {
	displaytrans_->transformDir( width );
	displaytrans_->transform( center );
    }

    const int tw = channels_->getChannels2RGBA()->getTextureWidth();
    const int th = channels_->getChannels2RGBA()->getTextureHeight();

    coords.erase();

    for ( int idx=0; idx<4; idx++ )
	coords += Coord3();

    coords[0] = Coord3( 0.0f, 0.0f, 0.0f );
    coords[1] = Coord3( tw, 0.0f, 0.0f );
    coords[2] = Coord3( 0.0f, th, 0.0f );
    coords[3] = Coord3( tw, th, 0.0f );

    const osg::Quat rotation = textureplane_->getRotation();
    osg::Matrix rotmt;
    rotmt.makeRotate( rotation );

    for ( int idx=0; idx<4; idx++ )
    {
	coords[idx].x /= tw;
	coords[idx].y /= th;
	coords[idx] -= Coord3( 0.5f, 0.5f, 0.0f );

	if ( textureplane_->areTextureAxesSwapped() )
	    coords[idx] = Coord3( coords[idx].y, coords[idx].x, 0.0f );
	if ( thindim==0 )
	    coords[idx]  = Coord3( 0.0f, coords[idx].x, coords[idx].y );
	else if ( thindim==1 )
	    coords[idx] = Coord3( coords[idx].x, 0.0f, coords[idx].y );

	coords[idx].x *= width.x;
	coords[idx].y *= width.y;
	coords[idx].z *= width.z;

	osg::Vec3 crd = Conv::to<osg::Vec3>( coords[idx] );
	crd = rotmt.preMult( crd );
	coords[idx] = Conv::to<Coord3>(crd) + center;
    }
}


int TextureRectangle::getNrTextures() const
{
    const std::vector<osg::Geometry*>& geometries =
					    textureplane_->getGeometries();
    return geometries.size();
}


const unsigned char* TextureRectangle::getTextureData() const
{
    const osg::Image* image = textureplane_->getCompositeTextureImage();
    return image ? image->data() : 0;
}


bool TextureRectangle::getTextureDataInfo( int tidx,
					   TextureDataInfo& texinfo ) const
{
    texinfo.setEmpty();
    const std::vector<osg::Geometry*>& geometries =
					textureplane_->getGeometries();

    if ( tidx >= geometries.size() )
	return false;

    const osg::Array* coords = geometries[tidx]->getVertexArray();
    const osg::Vec3Array* vtxcoords =
				dynamic_cast<const osg::Vec3Array*>( coords );

    const osg::PrimitiveSet* ps = geometries[tidx]->getPrimitiveSet( 0 );
    if ( !vtxcoords || !ps )
	return false;

    for ( int idx=0; idx<vtxcoords->size(); idx++ )
    {
	texinfo.coords_ += Coord3( vtxcoords->at(idx)[0],
				   vtxcoords->at(idx)[1],
				   vtxcoords->at(idx)[2] );
    }

    for ( int idx=0; idx<ps->getNumIndices(); idx++ )
	texinfo.ps_ += ps->index( idx );

    texinfo.texcoords_.setEmpty();
    osg::ref_ptr<const osg::Vec2Array> osgcoords =
			textureplane_->getCompositeTextureCoords( tidx );

    if ( !osgcoords.valid() )
	return false;

    for ( int idx=0; idx<osgcoords->size(); idx++ )
	 texinfo.texcoords_ += Conv::to<Coord>( osgcoords->at(idx) );

    return true;
}


bool TextureRectangle::getTextureInfo( int& width, int& height,
				       int& pixsize ) const
{
    const osg::Image* image = textureplane_->getCompositeTextureImage();
    if ( !image )
	return false;

    width = image->s();
    height = image->t();
    pixsize = image->getPixelSizeInBits() / 8;

    return true;
}

} // namespace visBase
