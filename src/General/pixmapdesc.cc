/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "pixmapdesc.h"
#include "bufstringset.h"
#include "separstr.h"

// PixmapDesc
PixmapDesc::PixmapDesc()
{}


PixmapDesc::PixmapDesc( const PixmapDesc& desc )
    : source_(desc.source_)
    , width_(desc.width_)
    , height_(desc.height_)
    , color_(desc.color_)
{}


PixmapDesc::PixmapDesc( const char* src, int width, int height,
			OD::Color col )
{
    set( src, width, height, col );
}


PixmapDesc::~PixmapDesc()
{}


PixmapDesc& PixmapDesc::operator=( const PixmapDesc& oth )
{
    set( oth.source_, oth.width_, oth.height_, oth.color_ );
    return *this;
}


void PixmapDesc::set( const char* src, int width, int height, OD::Color col )
{
    source_ = src;
    width_ = width;
    height_ = height;
    color_ = col;
}


bool PixmapDesc::isValid() const
{
    const bool isinvalid = width_==0 || height_==0;
    return !isinvalid;
}


BufferString PixmapDesc::toString() const
{
    FileMultiString fms;
    fms.add( source_ ).add( width_ ).add( height_ ).add( color_.getStdStr() );
    return fms.buf();
}


BufferStringSet PixmapDesc::toStringSet() const
{
    BufferStringSet pmsrc;
    pmsrc.add( source_ )
	 .add( ::toString(width_) )
	 .add( ::toString(height_) )
	 .add( color_.getStdStr() );
    return pmsrc;
}


void PixmapDesc::fromString( const char* str )
{
    FileMultiString fms( str );
    source_ = fms[0];
    width_ = fms.getIValue( 1 );
    height_ = fms.getIValue( 2 );
    color_.setStdStr( fms[3] );
}


void PixmapDesc::fromStringSet( const BufferStringSet& pmsrc )
{
    FileMultiString fms( nullptr );
    fms.set( pmsrc );
    fromString( fms.buf() );
}
