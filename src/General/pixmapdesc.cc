/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "pixmapdesc.h"
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
    const bool hassource = !source_.isEmpty();
    const bool hasdimension = width_!=0 || height_!=0;
    const bool isvalid = hassource || hasdimension;
    return isvalid;
}


bool PixmapDesc::sourceIsIcon() const
{
    if ( source_.isEmpty() )
	return false;

    if ( source_==sKeyXpmSrc() || source_==sKeyRgbSrc()
	 || source_==sKeyGradientSrc() || source_==sKeyColorTabSrc()
	 || source_==sKeySingleColorSrc() || source_==sKeyNoSrc() )
	return false;

    return true;
}


BufferString PixmapDesc::toString() const
{
    SeparString ss;
    ss.setSepChar( '`' );
    ss.add( source_ ).add( width_ ).add( height_ ).add( color_.getStdStr() );
    return ss.buf();
}


void PixmapDesc::fromString( const char* str )
{
    SeparString ss( str, '`' );
    source_ = ss[0];
    width_ = ss.getIValue( 1 );
    height_ = ss.getIValue( 2 );
    color_.setStdStr( ss[3] );
}
