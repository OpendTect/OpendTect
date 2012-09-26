/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Feb 2007
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "visimage.h"

#include "arraynd.h"
#include "color.h"

#include "Inventor/nodes/SoTexture2.h"
#include "Inventor/SbImage.h"

mCreateFactoryEntry( visBase::Image );

namespace visBase
{

Image::Image()
    : texture_( new SoTexture2 )
{
    texture_->ref();

    texture_->wrapS = SoTexture2::CLAMP;
    texture_->wrapT = SoTexture2::CLAMP;
    texture_->model = SoTexture2::MODULATE;
}


Image::~Image()
{
    texture_->unref();
}


bool Image::replacesMaterial() const
{
    return texture_->model.getValue() == SoTexture2::REPLACE;
}


void Image::replaceMaterial( bool yn )
{
    texture_->model = yn ? SoTexture2::REPLACE : SoTexture2::MODULATE;
}


void Image::setData( const Array2D<Color>& arr, bool usetrans )
{ 
    texture_->image.setValue( SbVec2s(arr.info().getSize(0),
				      arr.info().getSize(1)),
	    		      usetrans ? 4 : 3, 0 );
    SbVec2s sz;
    int nrcomponents;
    unsigned char* ptr = texture_->image.startEditing( sz, nrcomponents );
    for ( int idx=sz[0]-1; idx>=0; idx-- )
    {
	for ( int idy=sz[1]-1; idy>=0; idy-- )
	{
	    const Color col = arr.get( idx, idy );
	    (*ptr++) = col.r();
	    (*ptr++) = col.g();
	    (*ptr++) = col.b();
	    if ( usetrans ) (*ptr++) = col.t();
	}
    }

    texture_->image.finishEditing();
}


void Image::setFileName( const char* fn )
{
    texture_->filename.setValue( fn );
}


const char* Image::getFileName() const
{
    const char* fn = texture_->filename.getValue().getString();
    if ( !fn || !*fn )
	return 0;

    return fn;
}


SoNode* Image::gtInvntrNode()
{ return texture_; }


bool RGBImage::hasAlpha() const
{ return nrComponents()==4; }


char RGBImage::nrComponents() const
{
    int bytesperpixel;
    SbVec2s size;

    image_->getValue( size, bytesperpixel );
    return bytesperpixel;
}


bool RGBImage::setSize( int xsz, int ysz )
{ return false; }


int RGBImage::getSize( bool xdir ) const
{
    const SbVec3s size = image_->getSize();
    return size[xdir?0:1];
}


void RGBImage::fill( unsigned char* res ) const
{
    int bytesperpixel;
    SbVec2s size;

    unsigned char* ptr = image_->getValue( size, bytesperpixel );
    const int ptrsz = size[0]*size[1]*bytesperpixel;

    memcpy( res, ptr, ptrsz );
}


bool RGBImage::set(int idx, int idy, const Color& col )
{ return false; }


Color RGBImage::get(int idx, int idy ) const
{
    int bytesperpixel;
    SbVec2s size;

    unsigned char* ptr = image_->getValue( size, bytesperpixel );
    ptr += idx*size[1]+idy;

    Color res;
    const char nc = nrComponents();
    if ( nc<3 )
    {
	unsigned char col = *ptr;
	res.set( col, col, col, nc==2 ? ptr[1] : 0 );
    }
    else
    {
	res.set( *ptr, ptr[1], ptr[2], nc==4 ? ptr[3] : 0 );
    }

    return res;
}


    

}; // namespace visBase
