/*
___________________________________________________________________

 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: vistexture3.cc,v 1.9 2003-02-14 11:49:54 nanne Exp $";

#include "vistexture3.h"
#include "arrayndimpl.h"
#include "simpnumer.h"

#include "Inventor/nodes/SoSwitch.h"
#include "Inventor/nodes/SoTexture3.h"


mCreateFactoryEntry( visBase::Texture3 );

visBase::Texture3::Texture3()
    : x0sz( -1 )
    , x1sz( -1 )
    , x2sz( -1 )
    , texture( new SoTexture3 )
{
    texturegrp->addChild( texture );
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

    const int datax0sz = newdata->info().getSize( 0 );
    const int datax1sz = newdata->info().getSize( 1 );
    const int datax2sz = newdata->info().getSize( 2 );

    int newx0 = nextPower2( datax0sz, 64, 256 );
    int newx1 = nextPower2( datax1sz, 64, 256 );
    int newx2 = nextPower2( datax2sz, 64, 256 );
    setTextureSize( newx0, newx1, newx2 );

    Array3DInfoImpl newsize( x0sz, x1sz, x2sz );
    const int cachesz = newsize.getTotalSz();
    float* resized = new float[cachesz];
	
    const float x0step = (datax0sz-1)/(float)(x0sz-1);
    const float x1step = (datax1sz-1)/(float)(x1sz-1);
    const float x2step = (datax2sz-1)/(float)(x2sz-1);
    
    int idx=0;
    float val000, val001, val010, val011, val100, val101, val110, val111;
    for ( int x0=0; x0<x0sz; x0++ )
    {
	const float x0pos=x0*x0step;
	const int x0idx = (int)x0pos;
	const bool x0onedge = x0pos+1==datax0sz;
	const float x0relpos = x0pos-x0idx;

	for ( int x1=0; x1<x1sz; x1++ )
	{
	    const float x1pos=x1*x1step;
	    const int x1idx = (int) x1pos;
	    const bool x1onedge = x1pos+1==datax1sz;
	    const float x1relpos = x1pos-x1idx;

	    for ( int x2=0; x2<x2sz; x2++ )
	    {
		const float x2pos=x2*x2step;
		const int x2idx = (int) x2pos;
		const bool x2onedge = x2pos+1==datax2sz;
		const float x2relpos = x2pos-x2idx;

		val000 = newdata->get( x0idx, x1idx, x2idx );
		if ( !x1onedge )
		    val010 = newdata->get( x0idx, x1idx+1, x2idx );
		if ( !x2onedge )
		    val001 = newdata->get( x0idx, x1idx, x2idx+1 );
		if ( !x1onedge && !x2onedge )
		    val011 = newdata->get( x0idx, x1idx+1, x2idx+1 );
		if ( !x0onedge )
		    val100 = newdata->get( x0idx+1, x1idx, x2idx );
		if ( !x0onedge && !x1onedge )
		    val110 = newdata->get( x0idx+1, x1idx+1, x2idx );
		if ( !x0onedge && !x2onedge )
		    val101 = newdata->get( x0idx+1, x1idx, x2idx+1 );
		if ( !x0onedge && !x1onedge && !x2onedge )
		    val111 = newdata->get( x0idx+1, x1idx+1, x2idx+1 );

		float val = 0;
		if ( x0onedge && x1onedge && x2onedge )
		    val = val000;
		else if ( x0onedge && x1onedge )
		    val = linearInterpolate( val000, val001, x2relpos );
		else if ( x0onedge && x2onedge )
		    val = linearInterpolate( val000, val010, x1relpos );
		else if ( x1onedge && x2onedge )
		    val = linearInterpolate( val000, val100, x0relpos );
		else if ( x0onedge )
		    val = linearInterpolate2D( val000, val001, val010, val011,
					       x1relpos, x2relpos );
		else if ( x1onedge )
		    val = linearInterpolate2D( val000, val001, val100, val101,
					       x0relpos, x2relpos );
		else if ( x2onedge )
		    val = linearInterpolate2D( val000, val010, val100, val110,
					       x0relpos, x1relpos );
		else 
		    val = linearInterpolate3D( val000, val001, val010, val011,
					       val100, val101, val110, val111,
					       x0relpos, x1relpos, x2relpos );

		resized[newsize.getMemPos(x0,x1,x2)] = val;
	    }
	}
    }

    setResizedData( resized, cachesz );
}


unsigned char* visBase::Texture3::getTexturePtr()
{
    SbVec3s dimensions;
    int components;
    return texture->images.startEditing( dimensions, components ); 
}


void visBase::Texture3::finishEditing()
{ texture->images.finishEditing(); }
