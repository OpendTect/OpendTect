/*
___________________________________________________________________

 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: vistexture2.cc,v 1.16 2003-05-27 15:26:49 nanne Exp $";

#include "vistexture2.h"

#include "arrayndimpl.h"
#include "simpnumer.h"

#include "Inventor/nodes/SoSwitch.h"
#include "Inventor/nodes/SoTexture2.h"


mCreateFactoryEntry( visBase::Texture2 );

visBase::Texture2::Texture2()
    : x0sz( -1 )
    , x1sz( -1 )
    , texture( new SoTexture2 )
{
    texturegrp->addChild( texture );
    texture->wrapS = SoTexture2::CLAMP;
    texture->wrapT = SoTexture2::CLAMP;
    texture->model = SoTexture2::MODULATE;
    turnOn( true );
}


visBase::Texture2::~Texture2()
{
}


void visBase::Texture2::setTextureSize( int x0, int x1 )
{ 
    x0sz = x0; x1sz = x1;
    texture->image.setValue( SbVec2s( x1sz, x0sz ), 
	    		     usesTransperancy() ? 4 : 3, 0 );
}


void visBase::Texture2::setData( const Array2D<float>* newdata )
{
    if ( !newdata )
    {
	x0sz = -1;
	x1sz = -1;

	setResizedData( 0, 0 );
	return;
    }

    if ( x0sz==-1 || x1sz==-1 )
    {
	x0sz = 128;
	x1sz = 128;
	//TODO Change this to nearest bigger sz from data
    }

    const int datax0size = newdata->info().getSize(0);
    const int datax1size = newdata->info().getSize(1);

    int newx0 = datax0size;
    int newx1 = datax1size;
    if ( resolution )
    {
	newx0 = nextPower2( datax0size, 128, 1024 ) * resolution;
	newx1 = nextPower2( datax1size, 128, 1024 ) * resolution;
    }
    setTextureSize( newx0, newx1 );

    Array2DInfoImpl newsize(  x0sz, x1sz );
    const int cachesz = newsize.getTotalSz();
    float* resized = new float[cachesz];

    const float x0step = (datax0size-1)/(float)(x0sz-1);
    const float x1step = (datax1size-1)/(float)(x1sz-1);

    for ( int x0=0; x0<x0sz; x0++ )
    {
	const float x0pos=x0*x0step;
	const int x0idx = (int)x0pos;
	const bool x0onedge = x0idx+1==datax0size;
	const float x0relpos = x0pos-x0idx;

	for ( int x1=0; x1<x1sz; x1++ )
	{
	    const float x1pos = x1*x1step;
	    const int x1idx = (int)x1pos;
	    const bool x1onedge = x1idx+1==datax1size;
	    const float x1relpos = x1pos-x1idx;

	    const float val00 = newdata->get( x0idx, x1idx );
	    float val10, val01, val11;

	    if ( !x0onedge )
		val10 = newdata->get( x0idx+1, x1idx );
	    if ( !x1onedge )
		val01 = newdata->get( x0idx, x1idx+1 );
	    if ( !x0onedge && !x1onedge )
		val11 = newdata->get( x0idx+1, x1idx+1 );

	    const bool x0undef = (mIsUndefined(val00) && mIsUndefined(val01)) ||
				 (mIsUndefined(val10) && mIsUndefined(val11));
	    const bool x1undef = (mIsUndefined(val00) && mIsUndefined(val10)) ||
				 (mIsUndefined(val01) && mIsUndefined(val11));

	    float val = 0;
	    if ( (x0onedge && x1onedge) || (x0undef && x1undef) )
		val = val00;
	    else if ( x0onedge || x0undef )
		val = linearInterpolate( val00, val01, x1relpos );
	    else if ( x1onedge || x1undef )
		val = linearInterpolate( val00, val10, x0relpos );
	    else
		val = linearInterpolate2D( val00, val01, val10, val11,
			x0relpos, x1relpos );

	    resized[newsize.getMemPos( x0, x1 )] = val;
	}
    }

    setResizedData( resized, cachesz );
}


void visBase::Texture2::setColorData( const Array2D<float>* colordata ,int sel )
{
}


unsigned char* visBase::Texture2::getTexturePtr()
{
    SbVec2s dimensions;
    int components;
    return texture->image.startEditing( dimensions, components );
}


void visBase::Texture2::finishEditing()
{ texture->image.finishEditing(); }
