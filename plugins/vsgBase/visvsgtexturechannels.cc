/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visvsgtexturechannels.h"

#include "visvsgdata.h"
#include "vsggeocolorlookup.h"
#include "vsggeolayeredtexture.h"

#include "errmsg.h"
#include "undefval.h"

#include <algorithm>

namespace dGBVis
{

RefMan<TextureChannels> TextureChannels::create()
{
    RefMan<TextureChannels> ret = new TextureChannels;
    return ret;
}


TextureChannels::TextureChannels()
    : textureChanged(this)
    , colTabChanged(this)
{
}


TextureChannels::~TextureChannels()
{}


void TextureChannels::setSize( int channel, int sz0, int sz1, int sz2 )
{
    if ( !validIdx(channel) )
	return;

    if ( sz0!=1 || sz1<1 || sz2<1 )
    {
	pErrMsg("Only planes are supported");
	return;
    }

    if ( size_[0]==sz0 && size_[1]==sz1 && size_[2]==sz2 )
	return;

    size_[0] = sz0;
    size_[1] = sz1;
    size_[2] = sz2;
    data_.setSize( sz1*sz2, mUdf(float) );
    data_.setAll( mUdf(float) );
    ispremapped_ = false;
    mapping_.setData( data_.arr(), data_.size() );
    createTiles();
    mapToTexture( true, nullptr );
    textureChanged.trigger();
}


void TextureChannels::createTiles()
{
    const int maxsz = vsgGeo::LayeredTexture::maxTextureSize();
    const int width = size_[2];
    const int height = size_[1];
    const int stepx = width<=maxsz ? width : std::max( 1, maxsz-2 );
    const int stepy = height<=maxsz ? height : std::max( 1, maxsz-2 );
    tiles_.setEmpty();
    for ( int y0=0; y0<height; y0+=stepy )
    {
	for ( int x0=0; x0<width; x0+=stepx )
	{
	    Tile tile;
	    tile.x0_ = x0;
	    tile.y0_ = y0;
	    tile.nrx_ = std::min( stepx, width-x0 );
	    tile.nry_ = std::min( stepy, height-y0 );
	    tile.overx_ = x0>0 ? 1 : 0;
	    tile.overy_ = y0>0 ? 1 : 0;
	    const int overafterx = x0+tile.nrx_<width ? 1 : 0;
	    const int overaftery = y0+tile.nry_<height ? 1 : 0;
	    tile.texture_ = vsgGeo::LayeredTexture::create(
			tile.overx_+tile.nrx_+overafterx,
			tile.overy_+tile.nry_+overaftery,
			lookup_.colorLookupPtr(), interpolate_ );
	    tiles_ += tile;
	}
    }
}


int TextureChannels::getSize( int channel, unsigned char dim ) const
{
    return validIdx(channel) && dim<3 ? size_[dim] : 0;
}


void TextureChannels::setColTabMapperSetup( int channel,
					    const ColTab::MapperSetup& setup )
{
    if ( validIdx(channel) )
	mapping_.mapper().setup_ = setup;
}


const ColTab::MapperSetup& TextureChannels::getColTabMapperSetup( int, int )
									const
{
    return mapping_.mapper().setup_;
}


const ColTab::Mapper& TextureChannels::getColTabMapper( int, int ) const
{
    return mapping_.mapper();
}


bool TextureChannels::setUnMappedData( int channel, int version,
				       const float* data, OD::PtrPolicy policy,
				       TaskRunner* taskr, bool skipclip )
{
    const bool ok = validIdx( channel ) && version==0 && data &&
		    !tiles_.isEmpty();
    if ( ok )
    {
	OD::sysMemCopy( data_.arr(), data, data_.size()*sizeof(float) );
	ispremapped_ = false;
	mapping_.setData( data_.arr(), data_.size(), taskr );
	mapToTexture( !skipclip, taskr );
    }

    if ( policy == OD::TakeOverPtr )
	delete [] const_cast<float*>( data );

    return ok;
}


bool TextureChannels::setUnMappedVSData( int channel, int version,
					 const ValueSeries<float>* vs,
					 OD::PtrPolicy policy,
					 TaskRunner* taskr, bool skipclip )
{
    const bool ok = validIdx( channel ) && version==0 && vs &&
		    !tiles_.isEmpty();
    if ( ok )
    {
	if ( vs->arr() )
	    OD::sysMemCopy( data_.arr(), vs->arr(),
			    data_.size()*sizeof(float) );
	else
	{
	    for ( int idx=0; idx<data_.size(); idx++ )
		data_[idx] = vs->value( idx );
	}

	ispremapped_ = false;
	mapping_.setData( data_.arr(), data_.size(), taskr );
	mapToTexture( !skipclip, taskr );
    }

    if ( policy == OD::TakeOverPtr )
	delete vs;

    return ok;
}


bool TextureChannels::setMappedData( int channel, int version,
				     unsigned char* data, OD::PtrPolicy policy )
{
    const int nrbytes = size_[1] * size_[2] * ColTabMapping::nrBands();
    const bool ok = validIdx( channel ) && version==0 && data &&
		    !tiles_.isEmpty();
    if ( ok )
    {
	premapped_.setSize( nrbytes );
	OD::sysMemCopy( premapped_.arr(), data, nrbytes );
	ispremapped_ = true;
	mapToTexture( false, nullptr );
    }

    if ( policy == OD::TakeOverPtr )
	delete [] data;

    return ok;
}


void TextureChannels::reMapData( int channel, bool dontreclip,
				 TaskRunner* taskr )
{
    if ( validIdx(channel) )
	mapToTexture( !dontreclip, taskr );
}


const TypeSet<float>* TextureChannels::getHistogram( int channel ) const
{
    return validIdx(channel) && !ispremapped_ ? &mapping_.histogram()
					      : nullptr;
}


void TextureChannels::enableTextureInterpolation( bool yn )
{
    if ( interpolate_ == yn )
	return;

    interpolate_ = yn;
    if ( tiles_.isEmpty() )
	return;

    createTiles();
    mapToTexture( false, nullptr );
    textureChanged.trigger();
}


void TextureChannels::setColTabSequence( const ColTab::Sequence& seq )
{
    lookup_.setSequence( seq );
    colTabChanged.trigger();
    DataObject::requestSingleRedraw();
}


void TextureChannels::mapToTexture( bool full, TaskRunner* taskr )
{
    if ( tiles_.isEmpty() )
	return;

    if ( !ispremapped_ )
	mapping_.update( full, taskr );

    const unsigned char* bands = ispremapped_ ? premapped_.arr()
					      : mapping_.indices();
    const od_int64 nrbands = ColTabMapping::nrBands();
    TypeSet<unsigned char> tilebands;
    for ( const Tile& tile : tiles_ )
    {
	const od_int64 width = tile.texture_->width();
	const od_int64 height = tile.texture_->height();
	tilebands.setSize( int(width*height*nrbands) );
	for ( od_int64 idy=0; idy<height; idy++ )
	{
	    const od_int64 srcrow = tile.y0_ - tile.overy_ + idy;
	    const unsigned char* row = bands +
		( srcrow*size_[2] + tile.x0_-tile.overx_ ) * nrbands;
	    OD::sysMemCopy( tilebands.arr()+idy*width*nrbands, row,
			    width*nrbands );
	}

	tile.texture_->setIndices( tilebands.arr(), size_t(tilebands.size()) );
    }

    colTabChanged.trigger();
    DataObject::requestSingleRedraw();
}

} // namespace dGBVis
