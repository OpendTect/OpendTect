/*
___________________________________________________________________

 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: vistexture3.cc,v 1.7 2003-01-28 08:00:06 kristofer Exp $";

#include "vistexture3.h"

#include "arrayndimpl.h"
#include "Inventor/nodes/SoSwitch.h"
#include "Inventor/nodes/SoTexture3.h"

mCreateFactoryEntry( visBase::Texture3 );

visBase::Texture3::Texture3()
    : x0sz( -1 )
    , x1sz( -1 )
    , x2sz( -1 )
    , texture( new SoTexture3 )
{
    onoff->addChild( texture );
    texture->wrapR = SoTexture3::CLAMP;
    texture->wrapS = SoTexture3::CLAMP;
    texture->wrapT = SoTexture3::CLAMP;
    texture->model = SoTexture3::MODULATE;

    turnOn( true );
}


visBase::Texture3::~Texture3()
{
}


void visBase::Texture3::setTextureSize( int x0, int x1, int x2 )
{ 
    x0sz = x0; x1sz = x1; x2sz=x2;
    texture->images.setValue( SbVec3s( x2sz, x1sz, x0sz ),
	    		      usesTransperancy() ? 4 : 3, 0 );
}


void visBase::Texture3::setData( const Array3D<float>* newdata )
{
    if ( !newdata )
    {
	setResizedData( 0, 0 );
	return;
    }

    int x0 = newdata->info().getSize( 0 );
    int x1 = newdata->info().getSize( 1 );
    int x2 = newdata->info().getSize( 2 );

    // TODO resize data
    setTextureSize( x0, x1, x2 );

    const int totalsz = x0*x1*x2;
    float* datacopy = new float[totalsz];

    memcpy( datacopy, newdata->getData(), totalsz*sizeof(float) );

    setResizedData( datacopy, totalsz );
}


unsigned char* visBase::Texture3::getTexturePtr()
{
    SbVec3s dimensions;
    int components;
    return texture->images.startEditing( dimensions, components ); 
}


void visBase::Texture3::finishEditing()
{ texture->images.finishEditing(); }

