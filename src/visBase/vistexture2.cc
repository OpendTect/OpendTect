/*
___________________________________________________________________

 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: vistexture2.cc,v 1.19 2003-08-29 14:56:17 bert Exp $";

#include "vistexture2.h"

#include "arrayndimpl.h"
#include "interpol.h"
#include "simpnumer.h"
#include <limits.h>

#include "Inventor/nodes/SoSwitch.h"
#include "Inventor/nodes/SoTexture2.h"

mCreateFactoryEntry( visBase::Texture2 );

//TODO make these class members and get them from COIN
static const int minpix2d = 128;
static const int maxpix2d = 1024;


inline int getPow2Sz( int actsz, bool above=true, int minsz=1,
		      int maxsz=INT_MAX )
{
    char npow = 0; char npowextra = actsz == 1 ? 1 : 0;
    int sz = actsz;
    while ( sz > 1 )
    {
	if ( above && !npowextra && sz % 2 )
	    npowextra = 1;
	sz /= 2; npow++;
    }
    sz = intpow( 2, npow + npowextra );
    if ( sz < minsz ) sz = minsz;
    if ( sz > maxsz ) sz = maxsz;
    return sz;
}


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


#define mSodOff { x0sz = x1sz = -1; setResizedData( 0, 0, sel ); return; }


void visBase::Texture2::setData( const Array2D<float>* newdata, DataType sel )
{
    if ( !newdata )
	mSodOff

    const int datax0size = newdata->info().getSize(0);
    const int datax1size = newdata->info().getSize(1);
    if ( datax0size < 2 || datax1size < 2  )
	mSodOff

    if ( x0sz==-1 || x1sz==-1 )
    {
	x0sz = getPow2Sz( datax0size, true, minpix2d, maxpix2d );
	x1sz = getPow2Sz( datax1size, true, minpix2d, maxpix2d );
    }

    int newx0 = x0sz;
    int newx1 = x1sz;
    if ( resolution )
    {
	newx0 = nextPower2( datax0size, minpix2d, maxpix2d ) * resolution;
	newx1 = nextPower2( datax1size, minpix2d, maxpix2d ) * resolution;
    }
    setTextureSize( newx0, newx1 );

    Array2DInfoImpl newsize( x0sz, x1sz );
    const int cachesz = newsize.getTotalSz();
    float* resized = new float[cachesz];

    const float x0step = (datax0size-1)/(float)(x0sz-1);
    const float x1step = (datax1size-1)/(float)(x1sz-1);

    float val; const float udf = mUndefValue;
    for ( int x0=0; x0<x0sz; x0++ )
    {
	const float x0pos=x0*x0step;
	const int x0idx = (int)x0pos;
	const float x0relpos = x0pos-x0idx;
	const bool x0onedge = x0idx == 0 || x0idx == x0sz-1;

	for ( int x1=0; x1<x1sz; x1++ )
	{
	    const float x1pos = x1*x1step;
	    const int x1idx = (int)x1pos;
	    const float x1relpos = x1pos-x1idx;
	    const bool x1onedge = x1idx == 0 || x1idx == x1sz-1;

	    const float v01 = x0onedge ? udf : newdata->get( x0idx-1, x1idx );
	    const float v02 = x0onedge ? udf : newdata->get( x0idx-1, x1idx+1 );
	    const float v10 = x1onedge ? udf : newdata->get( x0idx,   x1idx-1 );
	    const float v11 = newdata->get( x0idx,   x1idx );
	    const float v12 = newdata->get( x0idx,   x1idx+1 );
	    const float v13 = x1onedge ? udf : newdata->get( x0idx,   x1idx+2 );
	    const float v20 = x1onedge ? udf : newdata->get( x0idx+1, x1idx-1 );
	    const float v21 = newdata->get( x0idx+1, x1idx );
	    const float v22 = newdata->get( x0idx+1, x1idx+1 );
	    const float v23 = x1onedge ? udf : newdata->get( x0idx+1, x1idx+2 );
	    const float v31 = x0onedge ? udf : newdata->get( x0idx+2, x1idx );
	    const float v32 = x0onedge ? udf : newdata->get( x0idx+2, x1idx+1 );

	    if ( mIsUndefined(v11) || mIsUndefined(v12) ||
		 mIsUndefined(v21) || mIsUndefined(v22) )
	    {
		val = x0relpos > 0.5 ? ( x1relpos > 0.5 ? v22 : v21 )
		    		     : ( x1relpos > 0.5 ? v12 : v11 );
	    }
	    else if ( mIsUndefined(v01) || mIsUndefined(v02) ||
		      mIsUndefined(v10) || mIsUndefined(v13) ||
		      mIsUndefined(v20) || mIsUndefined(v23) ||
		      mIsUndefined(v31) || mIsUndefined(v32) )
	    {
		val = linearInterpolate2D( v11, v12, v21, v22,
					   x0relpos, x1relpos );
	    }
	    else
	    {
		val = polyInterpolate2DDual1D( v01, v02, v10, v11, v12, v13,
						v20, v21, v22, v23, v31, v32,
						x0relpos, x1relpos );
	    }

	    resized[newsize.getMemPos( x0, x1 )] = val;
	}
    }

    setResizedData( resized, cachesz, sel );
}


unsigned char* visBase::Texture2::getTexturePtr()
{
    SbVec2s dimensions;
    int components;
    return texture->image.startEditing( dimensions, components );
}


void visBase::Texture2::finishEditing()
{ texture->image.finishEditing(); }
