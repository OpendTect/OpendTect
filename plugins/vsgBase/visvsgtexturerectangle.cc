/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visvsgtexturerectangle.h"

#include "visvsgtexturechannels.h"
#include "visvsgtransform.h"
#include "vsggeolayeredtexture.h"
#include "vsggeotextureplane.h"

#include "errmsg.h"

#include <vsg/maths/vec3.h>
#include <vsg/utils/SharedObjects.h>

namespace
{

/*!The part of one image direction that a tile shows, as fractions of the
  rectangle in that direction, and the texture coordinates at those fractions.
  The texture envelope has the size of the image minus one cell, starts
  half a cell in, and is moved and resized by the shift and growth. */

struct AxisSpan
{
    bool	visible_	= false;
    double	start_		= 0.;
    double	end_		= 0.;
    double	tcstart_	= 0.;
    double	tcend_		= 0.;
};


AxisSpan spanOf( int nrcells, double shift, double growth, int first, int nr,
		 int over, int texsize )
{
    AxisSpan ret;
    const double size = ( nrcells>1 ? nrcells-1 : 1 ) + growth;
    if ( size <= 0. )
	return ret;

    const double envelopestart = 0.5*nrcells + shift - 0.5*size;
    ret.start_ = mMAX( 0., mMIN(1.,(first-envelopestart)/size) );
    ret.end_ = mMAX( 0., mMIN(1.,(first+nr-envelopestart)/size) );
    if ( ret.end_ <= ret.start_ )
	return ret;

    const double texstart = first - over;
    ret.tcstart_ = ( envelopestart + ret.start_*size - texstart ) / texsize;
    ret.tcend_ = ( envelopestart + ret.end_*size - texstart ) / texsize;
    ret.visible_ = true;
    return ret;
}


vsg::vec3 toVec3( const Coord3& crd )
{
    return vsg::vec3( float(crd.x_), float(crd.y_), float(crd.z_) );
}


Coord3 unitVec( int dim )
{
    return Coord3( dim==0 ? 1. : 0., dim==1 ? 1. : 0., dim==2 ? 1. : 0. );
}

} // namespace


namespace dGBVis
{

RefMan<TextureRectangle> TextureRectangle::create()
{
    RefMan<TextureRectangle> ret = new TextureRectangle;
    return ret;
}


TextureRectangle::TextureRectangle()
    : VisualObjectImpl(false)
{}


TextureRectangle::~TextureRectangle()
{
    detachAllNotifiers();
}


void TextureRectangle::setTextureChannels( TextureChannels* channels )
{
    if ( channels_.ptr() == channels )
	return;

    if ( channels_ )
    {
	mDetachCB( channels_->textureChanged,
		   TextureRectangle::textureChangedCB );
	mDetachCB( channels_->colTabChanged,
		   TextureRectangle::colTabChangedCB );
    }

    channels_ = channels;
    if ( channels_ )
    {
	mAttachCB( channels_->textureChanged,
		   TextureRectangle::textureChangedCB );
	mAttachCB( channels_->colTabChanged,
		   TextureRectangle::colTabChangedCB );
    }

    rebuild();
}


void TextureRectangle::freezeDisplay( bool yn )
{
    frozen_ = yn;
    if ( !yn && needsrebuild_ )
	rebuild();
}


void TextureRectangle::setTextureShift( const Coord& shift )
{
    shift_ = shift;
    rebuild();
}


void TextureRectangle::setTextureGrowth( const Coord& growth )
{
    growth_ = growth;
    rebuild();
}


void TextureRectangle::setCenter( const Coord3& center )
{
    center_ = center;
    rebuild();
}


void TextureRectangle::setWidth( const Coord3& width )
{
    width_ = width;
    rebuild();
}


void TextureRectangle::setRotation( const Coord3& spanvec0,
				    const Coord3& spanvec1 )
{
    spanvec0_ = spanvec0;
    spanvec1_ = spanvec1;
    rotated_ = true;
    rebuild();
}


void TextureRectangle::setRotationAndWidth( const Coord3& spanvec0,
					    const Coord3& spanvec1 )
{
    spanvec0_ = spanvec0;
    spanvec1_ = spanvec1;
    rotated_ = true;
    width_ = Coord3( spanvec0.abs(), spanvec1.abs(), 0. );
    rebuild();
}


void TextureRectangle::swapTextureAxes( bool yn )
{
    swapped_ = yn;
    rebuild();
}


void TextureRectangle::setDisplayTransformation( const mDGBVisTrans* trans )
{
    displaytrans_ = trans;
    rebuild();
}


void TextureRectangle::textureChangedCB( CallBacker* )
{
    rebuild();
}


void TextureRectangle::colTabChangedCB( CallBacker* )
{
    if ( channels_ && channels_->hasTransparency()!=blended_ )
	rebuild();
}


Coord3 TextureRectangle::toDisplay( const Coord3& pos ) const
{
    Coord3 disp = pos;
    Transformation::transform( displaytrans_.ptr(), pos, disp );
    return disp;
}


void TextureRectangle::rebuild()
{
    if ( frozen_ )
    {
	needsrebuild_ = true;
	return;
    }

    needsrebuild_ = false;
    for ( vsg::ref_ptr<vsgGeo::TexturePlane>& plane : planes_ )
    {
	removeChild( plane.get() );
	releaseNode( plane.get() );
    }

    planes_.setEmpty();
    if ( !channels_ || channels_->nrTiles()<1 )
	return;

    const double widths[] = { width_.x_, width_.y_, width_.z_ };
    TypeSet<int> dims;
    for ( int idim=0; idim<3; idim++ )
    {
	if ( !mIsZero(widths[idim],mDefEps) )
	    dims += idim;
    }

    if ( dims.size() != 2 )
	return;

    const Coord3 dir0 = rotated_ ? spanvec0_.normalize() : unitVec( dims[0] );
    const Coord3 dir1 = rotated_ ? spanvec1_.normalize() : unitVec( dims[1] );
    const Coord3 uaxis = dir0 * widths[dims[0]];
    const Coord3 vaxis = dir1 * widths[dims[1]];
    const Coord3 origin = center_ - (uaxis+vaxis)*0.5;
    const int imagewidth = channels_->getSize( 0, 2 );
    const int imageheight = channels_->getSize( 0, 1 );
    blended_ = channels_->hasTransparency();

    vsg::ref_ptr<vsg::SharedObjects> shared = vsg::SharedObjects::create();
    for ( int idx=0; idx<channels_->nrTiles(); idx++ )
    {
	const TextureChannels::Tile& tile = channels_->tile( idx );
	const AxisSpan xspan = spanOf( imagewidth, shift_.x_, growth_.x_,
				       tile.x0_, tile.nrx_, tile.overx_,
				       tile.texture_->width() );
	const AxisSpan yspan = spanOf( imageheight, shift_.y_, growth_.y_,
				       tile.y0_, tile.nry_, tile.overy_,
				       tile.texture_->height() );
	if ( !xspan.visible_ || !yspan.visible_ )
	    continue;

	const AxisSpan& uspan = swapped_ ? yspan : xspan;
	const AxisSpan& vspan = swapped_ ? xspan : yspan;
	const double dtcx = xspan.tcend_ - xspan.tcstart_;
	const double dtcy = yspan.tcend_ - yspan.tcstart_;
	const vsg::vec2 tcu = swapped_ ? vsg::vec2( 0.f, float(dtcy) )
				       : vsg::vec2( float(dtcx), 0.f );
	const vsg::vec2 tcv = swapped_ ? vsg::vec2( float(dtcx), 0.f )
				       : vsg::vec2( 0.f, float(dtcy) );
	const Coord3 tileorigin = origin + uaxis*uspan.start_ +
				  vaxis*vspan.start_;
	const Coord3 tileu = uaxis * (uspan.end_-uspan.start_);
	const Coord3 tilev = vaxis * (vspan.end_-vspan.start_);
	const Coord3 start = toDisplay( tileorigin );
	const Coord3 ustart = toDisplay( tileorigin+tileu );
	const Coord3 vstart = toDisplay( tileorigin+tilev );
	vsg::ref_ptr<vsgGeo::TexturePlane> plane = vsgGeo::TexturePlane::create(
	    tile.texture_, toVec3(start), toVec3(ustart-start),
	    toVec3(vstart-start),
	    vsg::vec2( float(xspan.tcstart_), float(yspan.tcstart_) ),
	    tcu, tcv, shared, blended_ );
	planes_ += plane;
	addChild( plane.get() );
    }
}

} // namespace dGBVis
