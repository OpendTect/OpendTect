/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsggeolayeredtexture.h"

#include <vsg/state/Sampler.h>

#include <algorithm>

namespace
{

int sMaxTextureSize = 4096;

} // namespace


namespace vsgGeo
{

int LayeredTexture::maxTextureSize()
{
    return sMaxTextureSize;
}


void LayeredTexture::setMaxTextureSize( int size )
{
    sMaxTextureSize = std::max( 1, size );
}


LayeredTexture::LayeredTexture( int width, int height,
				vsg::ref_ptr<ColorLookup> lookup,
				bool interpolate )
    : lookup_(lookup)
    , interpolated_(interpolate)
{
    if ( !lookup_ )
	lookup_ = ColorLookup::create();

    image_ = vsg::ubvec2Array2D::create( uint32_t(std::max(1,width)),
			uint32_t(std::max(1,height)),
			vsg::Data::Properties{VK_FORMAT_R8G8_UNORM} );
    image_->properties.dataVariance = vsg::DYNAMIC_DATA;
    for ( auto& pix : *image_ )
	pix = vsg::ubvec2( 0, 0 );

    const VkFilter filter = interpolated_ ? VK_FILTER_LINEAR
					  : VK_FILTER_NEAREST;
    sampler_ = vsg::Sampler::create();
    sampler_->minFilter = filter;
    sampler_->magFilter = filter;
    sampler_->mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler_->addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_->addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_->maxLod = 0.f;
    sampler_->anisotropyEnable = VK_FALSE;
}


LayeredTexture::~LayeredTexture()
{}


int LayeredTexture::width() const
{
    return int( image_->width() );
}


int LayeredTexture::height() const
{
    return int( image_->height() );
}


bool LayeredTexture::setIndices( const unsigned char* bands, size_t size )
{
    if ( !bands ||
	 size != size_t(width())*size_t(height())*size_t(nrBands()) )
	return false;

    unsigned char* dest = static_cast<unsigned char*>( image_->dataPointer() );
    for ( size_t pos=0; pos<size; pos+=nrBands() )
    {
	const bool defined = bands[pos+1] >= 128;
	dest[pos] = defined ? bands[pos] : 0;
	dest[pos+1] = defined ? 255 : 0;
    }

    image_->dirty();
    return true;
}


unsigned char LayeredTexture::indexAt( int x, int y ) const
{
    return image_->at( uint32_t(x), uint32_t(y) ).x;
}


bool LayeredTexture::isUndefAt( int x, int y ) const
{
    return image_->at( uint32_t(x), uint32_t(y) ).y < 128;
}


vsg::ref_ptr<vsg::DescriptorImage> LayeredTexture::indexDescriptor(
						uint32_t binding ) const
{
    return vsg::DescriptorImage::create( sampler_, image_, binding, 0,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER );
}


vsg::ref_ptr<vsg::DescriptorImage> LayeredTexture::colorDescriptor(
						uint32_t binding ) const
{
    return lookup_->descriptor( binding );
}

} // namespace vsgGeo
