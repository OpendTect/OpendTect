/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : August 2010
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "odimage.h"

namespace OD
{


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


bool RGBImage::put( unsigned char const* source, 
		    bool xdir_slowest, bool opacity )
{
    const int xsize = getSize( true );
    const int ysize = getSize( false );
    const char nrcomponents = nrComponents();

    Color col;
    for ( int idx=0; idx<xsize; idx++ )
    {
	for ( int idy=0; idy<ysize; idy++ )
	{
	    const int pixeloffset = xdir_slowest
		? (idx * ysize + idy) * nrcomponents
		: (idy * xsize + idx) * nrcomponents;

	    unsigned char const* pixelsource = source + pixeloffset;

	    if ( nrcomponents==1 )
		col.set( *pixelsource, *pixelsource, *pixelsource, 0 );
	    else if ( nrcomponents==2 )
		col.set( *pixelsource, *pixelsource, *pixelsource,
			 pixelsource[1] );
	    else if ( nrcomponents==3 )
		col.set( *pixelsource, pixelsource[1], pixelsource[2], 0 );
	    else
		col.set( *pixelsource, pixelsource[1], pixelsource[2],
		         opacity ? 255-pixelsource[3] : pixelsource[3] );

	    if ( !set( idx, idy, col ) )
		return false;
	}
    }

    return true;
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


};
