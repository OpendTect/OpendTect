/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: vistexture3.cc,v 1.16 2003-11-07 12:22:02 bert Exp $";

#include "vistexture3.h"
#include "arrayndimpl.h"
#include "interpol.h"

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
    texture->images.setValue( SbVec3s( x0sz, x1sz, x2sz ),
	    		      usesTransperancy() ? 4 : 3, 0 );
}


void visBase::Texture3::setData( const Array3D<float>* newdata, DataType sel )
{
    if ( !newdata )
    {
	setResizedData( 0, 0, sel );
	return;
    }

    const int datax0sz = newdata->info().getSize( 0 );
    const int datax1sz = newdata->info().getSize( 1 );
    const int datax2sz = newdata->info().getSize( 2 );

#define mMaxTextSz 256
#define mMinTextSz 64

    int maxsize0 = mMaxTextSz;
    int maxsize1 = mMaxTextSz;
    int maxsize2 = mMaxTextSz;

    const char* envlimit = getenv("dTECT_3DTEXTURE_LIMIT");
    if ( envlimit )
    {
	int dummy;
	const char* firstend = strchr(envlimit,'x');
	if ( firstend )
	{
	    BufferString first = envlimit;
	    first[firstend-envlimit] = 0;
	    BufferString second = firstend+1;
	    if ( getFromString( dummy, first ))
		maxsize0 = mMAX(mMinTextSz,dummy);

	    const char* secondend = strchr(second,'x');

	    if ( secondend )
	    {
		second[secondend-second.buf()] = 0;
		BufferString third = secondend+1;
		if ( getFromString( dummy, second ))
		    maxsize1 = mMAX(mMinTextSz,dummy );
		if ( getFromString( dummy, third ))
		    maxsize2 = mMAX(mMinTextSz, dummy );
		}
	}
	else if ( getFromString( dummy,envlimit ) )
	{
	    maxsize0 = mMAX(mMinTextSz, dummy );
	    maxsize1 = maxsize0;
	    maxsize2 = maxsize0;
	}

    }

    int newx0 = nextPower2( datax0sz, mMinTextSz, maxsize0 );
    int newx1 = nextPower2( datax1sz, mMinTextSz, maxsize1 );
    int newx2 = nextPower2( datax2sz, mMinTextSz, maxsize2 );
    setTextureSize( newx0, newx1, newx2 );

    const int cachesz = newx0*newx1*newx2;
    float* resized = new float[cachesz];
	
    const float x0step = (datax0sz-1)/(float)(x0sz-1);
    const float x1step = (datax1sz-1)/(float)(x1sz-1);
    const float x2step = (datax2sz-1)/(float)(x2sz-1);
    
    int idx=0;
    float val000, val001, val010, val011, val100, val101, val110, val111;
    for ( int x2=0; x2<x2sz; x2++ )
    {
	const float x2pos=x2*x2step;
	const int x2idx = (int)x2pos;
	const bool x2onedge = x2idx+1==datax2sz;
	const float x2relpos = x2pos-x2idx;

	for ( int x1=0; x1<x1sz; x1++ )
	{
	    const float x1pos=x1*x1step;
	    const int x1idx = (int)x1pos;
	    const bool x1onedge = x1idx+1==datax1sz;
	    const float x1relpos = x1pos-x1idx;

	    for ( int x0=0; x0<x0sz; x0++ )
	    {
		const float x0pos=x0*x0step;
		const int x0idx = (int)x0pos;
		const bool x0onedge = x0idx+1==datax0sz;
		const float x0relpos = x0pos-x0idx;

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

		resized[idx++] = val;
		/*!< Note that this is the coin-ordering, not
		     the standard Array size
		 */
		    
	    }
	}
    }

    setResizedData( resized, cachesz, sel );
}


unsigned char* visBase::Texture3::getTexturePtr()
{
    SbVec3s dimensions;
    int components;
    return texture->images.startEditing( dimensions, components ); 
}


void visBase::Texture3::finishEditing()
{
    texture->images.finishEditing(); 
    texture->touch();
}
