/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Feb 2007
 RCS:           $Id: visimage.cc,v 1.1 2007-02-07 14:38:29 cvskris Exp $
________________________________________________________________________

-*/

#include "visimage.h"

#include "arraynd.h"
#include "color.h"

#include "Inventor/nodes/SoTexture2.h"

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

}; // namespace visBase
