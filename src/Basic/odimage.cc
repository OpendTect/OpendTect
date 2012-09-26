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


bool RGBImage::put( unsigned char const* source )
{
    const int xsize = getSize( true );
    const int ysize = getSize( false );
    const char nrcomponents = nrComponents();

    Color col;
    for ( int idx=0; idx<xsize; idx++ )
    {
	for ( int idy=0; idy<ysize; idy++ )
	{
	    if ( nrcomponents==1 )
		col.set( *source, *source, *source, 0 );
	    else if ( nrcomponents==2 )
		col.set( *source, *source, *source, source[1] );
	    else if ( nrcomponents==3 )
		col.set( *source, source[1], source[2], 0 );
	    else
		col.set( *source, source[1], source[2], source[3] );

	    if ( !set( idx, idy, col ) )
		return false;

	    source += nrcomponents;
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
