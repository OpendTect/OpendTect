/*
___________________________________________________________________

 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: vistexture3.cc,v 1.4 2003-01-10 10:18:23 kristofer Exp $";

#include "vistexture3.h"

#include "arrayndimpl.h"

#include "Inventor/nodes/SoGroup.h"
#include "Inventor/nodes/SoTexture3.h"

mCreateFactoryEntry( visBase::Texture3 );

visBase::Texture3::Texture3()
    : x0sz( -1 )
    , x1sz( -1 )
    , x2sz( -1 )
    , texture( new SoTexture3 )
    , root( new SoGroup )
{
    root->ref();
    root->addChild( texture );
    texture->wrapR = SoTexture3::CLAMP;
    texture->wrapS = SoTexture3::CLAMP;
    texture->wrapT = SoTexture3::CLAMP;

    texture->model = SoTexture3::MODULATE;
}


visBase::Texture3::~Texture3()
{
    root->unref();
}


void visBase::Texture3::setTextureSize( int x0, int x1, int x2 )
{ x0sz = x0; x1sz = x1; x2sz=x2; }


void visBase::Texture3::setData( const Array3D<float>* newdata )
{
    if ( !newdata )
    {
	setResizedData( 0, 0 );
	return;
    }

    x0sz = newdata->info().getSize( 0 );
    x1sz = newdata->info().getSize( 1 );
    x2sz = newdata->info().getSize( 2 );

    const int totalsz = x0sz*x1sz*x2sz;
    float* datacopy = new float[totalsz];

    memcpy( datacopy, newdata->getData(), totalsz*sizeof(float) );

    setResizedData( datacopy, totalsz );
}


SoNode* visBase::Texture3::getData()
{ return root; }


void visBase::Texture3::setTexture(const unsigned char* imagedata)
{
    if ( !imagedata )
    {
	texture->images.setValue( SbVec3s( 0, 0, 0 ), 3, 0 );
	return;
    }

    texture->images.setValue( SbVec3s( x0sz, x1sz, x2sz ),
	    		     usesTransperancy() ? 4 : 3, imagedata );
}
