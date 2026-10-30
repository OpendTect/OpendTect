/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsggeocolorlookup.h"

#include <vsg/state/Sampler.h>

#include <algorithm>
#include <cmath>

namespace vsgGeo
{

namespace
{

uint8_t toByte( float val )
{
    const float clipped = std::min( 1.f, std::max(0.f,val) );
    return uint8_t( clipped*255.f + 0.5f );
}


vsg::ubvec4 toPixel( const vsg::vec4& col )
{
    return vsg::ubvec4( toByte(col.r), toByte(col.g), toByte(col.b),
			toByte(col.a) );
}

} // anonymous namespace


ColorLookup::ColorLookup( int nrcolors )
{
    const uint32_t width = uint32_t( std::max(2,nrcolors) ) + 1;
    strip_ = vsg::ubvec4Array2D::create( width, 1,
			vsg::Data::Properties{VK_FORMAT_R8G8B8A8_UNORM} );
    strip_->properties.dataVariance = vsg::DYNAMIC_DATA;
    for ( auto& pix : *strip_ )
	pix = vsg::ubvec4( 255, 255, 255, 255 );

    sampler_ = vsg::Sampler::create();
    sampler_->minFilter = VK_FILTER_LINEAR;
    sampler_->magFilter = VK_FILTER_LINEAR;
    sampler_->mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler_->addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_->addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_->maxLod = 0.f;
    sampler_->anisotropyEnable = VK_FALSE;
}


ColorLookup::~ColorLookup()
{}


int ColorLookup::nrColors() const
{
    return int( strip_->width() ) - 1;
}


bool ColorLookup::setColors( const vsg::vec4* colors, int nr,
			     const vsg::vec4& undefcolor )
{
    if ( !colors || nr!=nrColors() )
	return false;

    for ( int idx=0; idx<nr; idx++ )
	strip_->set( uint32_t(idx), 0, toPixel(colors[idx]) );

    strip_->set( uint32_t(nr), 0, toPixel(undefcolor) );
    strip_->dirty();
    return true;
}


vsg::vec4 ColorLookup::undefColor() const
{
    const vsg::ubvec4& col = strip_->at( strip_->width()-1, 0 );
    return vsg::vec4( float(col.r)/255.f, float(col.g)/255.f,
		      float(col.b)/255.f, float(col.a)/255.f );
}


vsg::vec4 ColorLookup::colorAt( float pos ) const
{
    const int nr = nrColors();
    const float clipped = std::min( 1.f, std::max(0.f,pos) );
    const float scaled = clipped * float( nr-1 );
    const int idx0 = std::min( nr-2, int(scaled) );
    const float frac = scaled - float( idx0 );
    const vsg::ubvec4& col0 = strip_->at( uint32_t(idx0), 0 );
    const vsg::ubvec4& col1 = strip_->at( uint32_t(idx0+1), 0 );
    vsg::vec4 ret;
    for ( int comp=0; comp<4; comp++ )
    {
	ret[comp] = ( float(col0[comp])*(1.f-frac) + float(col1[comp])*frac )
								/ 255.f;
    }

    return ret;
}


vsg::ref_ptr<vsg::DescriptorImage> ColorLookup::descriptor(
						uint32_t binding ) const
{
    return vsg::DescriptorImage::create( sampler_, strip_, binding, 0,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER );
}

} // namespace vsgGeo
