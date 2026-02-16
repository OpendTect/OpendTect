/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "odimage.h"

#include "paralleltask.h"

namespace OD
{
// RGBImageLoader
PtrMan<RGBImageLoader> RGBImageLoader::imageloader_;


RGBImageLoader::RGBImageLoader()
{}


RGBImageLoader::~RGBImageLoader()
{}


RGBImage* RGBImageLoader::loadRGBImage( const char* fnm, uiString& errmsg )
{ 
    return imageloader_ ? imageloader_->loadImage( fnm, errmsg ) : 0; 
}


void RGBImageLoader::setImageLoader( RGBImageLoader* il )
{
    imageloader_ = il;
}


//RGBImage
RGBImage::RGBImage()
{}


RGBImage::~RGBImage()
{}


bool RGBImage::hasAlpha() const
{
    const char nc = nrComponents();
    return nc==2||nc==4;
}


void RGBImage::fill( unsigned char* res ) const
{
    const int xsize = getSize( true );
    const int ysize = getSize( false );

    const char nrcomponents = nrComponents();

    for ( int idx=0; idx<xsize; idx++ )
    {
	for ( int idy=0; idy<ysize; idy++ )
	{
	    const Color col = get( idx, idy );
	    (*res++) = col.r();
	    if ( nrcomponents==2 )
		(*res++) = col.t();
	    else
	    {
		(*res++) = col.g();
		(*res++) = col.b();
		if ( nrcomponents==4 )
		    (*res++) = col.t();
	    }
	}
    }
}


class ImageFiller : public ParallelTask
{
public:
ImageFiller( RGBImage& image, unsigned char const* source,bool xdir_slowest,
	     bool opacity )
    : ParallelTask("Filling Image")
    , image_(image)
    , source_(source)
    , xdir_slowest_(xdir_slowest)
    , opacity_(opacity)
    , xsize_(image.getSize(true))
    , ysize_(image.getSize(false))
    , nrcomponents_(image.nrComponents())
{}


od_int64 nrIterations() const override
{
    return xsize_ * ysize_;
}


bool doWork( od_int64 start, od_int64 stop, int threadidx ) override
{
    Color col;
    for ( od_int64 gidx=start; gidx<=stop; gidx++ )
    {
	const od_int64 idx = gidx / ysize_;
	const od_int64 idy = gidx % ysize_;

	const int pixeloffset = xdir_slowest_
	    ? (idx * ysize_ + idy) * nrcomponents_
	    : (idy * xsize_ + idx) * nrcomponents_;

	unsigned char const* pixelsource = source_ + pixeloffset;

	if ( nrcomponents_==1 )
	    col.set( *pixelsource, *pixelsource, *pixelsource, 0 );
	else if ( nrcomponents_==2 )
	    col.set( *pixelsource, *pixelsource, *pixelsource,
	    pixelsource[1] );
	else if ( nrcomponents_==3 )
	    col.set( *pixelsource, pixelsource[1], pixelsource[2], 0 );
	else
	{
	    col.set( *pixelsource, pixelsource[1], pixelsource[2],
	    opacity_ ? 255-pixelsource[3] : pixelsource[3] );
	}

	if ( !image_.set(idx,idy,col) )
	    return false;
    }

    return true;
}


RGBImage&		image_;
unsigned char const*	source_;
bool			xdir_slowest_;
bool			opacity_;
od_int64		xsize_;
od_int64		ysize_;
int			nrcomponents_;

}; // class ImageFiller

bool RGBImage::put(unsigned char const* source,bool xdir_slowest, bool opacity)
{
    if ( !source )
	return false;

    ImageFiller filler( *this, source, xdir_slowest, opacity );
    return filler.execute();
}


class ImageBlender : public ParallelTask
{
public:
ImageBlender( RGBImage& image, const RGBImage& sourceimage,
	      bool blendtransparency,
	      unsigned char blendtransparencyval,
	      bool blendequaltransparency,
	      bool opacity )
    : ParallelTask("Filling Image")
    , image_(image)
    , sourceimage_(sourceimage)
    , blendtransparency_(blendtransparency)
    , blendtransparencyval_(blendtransparencyval)
    , blendequaltransparency_(blendequaltransparency)
    , opacity_(opacity)
    , xsize_(image.getSize(true))
    , ysize_(image.getSize(false))
    , nrcomponents_(image.nrComponents())
{}


od_int64 nrIterations() const override
{
    return xsize_ * ysize_;
}


bool doWork( od_int64 start, od_int64 stop, int threadidx ) override
{
    for ( od_int64 gidx=start; gidx<=stop; gidx++ )
    {
	const od_int64 idx = gidx / ysize_;
	const od_int64 idy = gidx % ysize_;

	const Color color = image_.get( idx, idy );
	const Color srccolor = sourceimage_.get( idx, idy );

	double a2 = .0f;
	double a1 = .0f;

	bool forceblendsourceimg =
	    blendtransparency_ && srccolor.t()== blendtransparencyval_;

	if ( forceblendsourceimg )
	{
	    a1 = 0; a2 = 1.0f;
	}
	else if ( !blendequaltransparency_ )
	{
	    a1 = ( color.t()==srccolor.t()  ) ? 1.0f : color.t()/255.0f;
	    a2 = ( color.t()==srccolor.t()  ) ? 0.0f : srccolor.t()/255.0f;
	}
	else
	{
	    a1 = color.t() / 255.0f;
	    a2 = srccolor.t() / 255.0f;
	}

	const unsigned char r =
	    (unsigned char)(a1 * color.r() + a2 * (1 - a1) * srccolor.r());
	const unsigned char g =
	    (unsigned char)(a1 * color.g() + a2 * (1 - a1) * srccolor.g());
	const unsigned char b =
	    (unsigned char)(a1 * color.b() + a2 * (1 - a1) * srccolor.b());

	unsigned char t = forceblendsourceimg ? 255 :
	    (unsigned char)( 255 * ( a1 + a2 * ( 1 - a1 ) ) );

	if ( !image_.set( idx, idy, Color(r,g,b,opacity_ ? 255-t : t) ) )
	    return false;
    }

    return true;
}


RGBImage&		image_;
const RGBImage&		sourceimage_;
bool			blendtransparency_;
unsigned char		blendtransparencyval_;
bool			blendequaltransparency_;
bool			opacity_;
od_int64		xsize_;
od_int64		ysize_;
int			nrcomponents_;

}; // class ImageBlender

bool RGBImage::blendWith( const RGBImage& sourceimage,
			  bool blendtransparency,
			  unsigned char blendtransparencyval,
			  bool blendequaltransparency,
			  bool with_opacity )
{
    if ( sourceimage.bufferSize() != bufferSize() )
	return false;

    const int xsize = getSize( true );
    const int ysize = getSize( false );

    if ( sourceimage.getSize( true )  != xsize || 
	 sourceimage.getSize( false ) != ysize )
	return false;

    ImageBlender blender( *this, sourceimage, blendtransparency,
			  blendtransparencyval, blendequaltransparency,
			  with_opacity );
    return blender.execute();
}


bool RGBImage::putFromBitmap(const unsigned char* bitmap,
    const unsigned char* maskptr )
{
    const int xsize = getSize( true );
    const int ysize = getSize( false );

    Color col;
    char bytecount = 0;

    for ( int idx=0; idx<xsize; idx++ )
    {
	for ( int idy=0; idy<ysize; idy++ )
	{
	    unsigned char byte = *bitmap;
	    unsigned char mask = maskptr ? *maskptr : 255;

	    byte >>= bytecount;
	    mask >>= bytecount;

	    byte &= 1;
	    mask &= 1;

	    byte *=255;
	    mask = 255*mask;

	    col.set( byte, byte, byte );
	    col.setTransparency( 255-mask );

	    if ( !set( idx, idy, col ) )
		return false;

	    bytecount++;
	    if ( bytecount == 8 )
	    {
		bytecount = 0;
		bitmap++;
		maskptr++;
	    }
	}
    }

    return true;
}



int RGBImage::bufferSize() const
{
    return nrComponents()*getSize(true)*getSize(false);
}

} // namespace OD
