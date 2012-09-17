/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Jan 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id: vistexture3.cc,v 1.34 2009/08/27 11:29:36 cvsraman Exp $";

#include "vistexture3.h"
#include "arrayndimpl.h"
#include "interpol3d.h"
#include "envvars.h"
#include "string2.h"

#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoTexture3.h>

mCreateFactoryEntry( visBase::Texture3 );

namespace visBase
{

class Texture3InterpolFiller: public ParallelTask
{
public: 

Texture3InterpolFiller( float* res, int sz0, int sz1, int sz2,
		     const Array3D<float>& data )
    : data_( data )
    , res_( res ) 
    , x0sz_( sz0 )
    , x1sz_( sz1 )
    , x2sz_( sz2 )		  
{}

od_int64 nrIterations() const { return x0sz_*x1sz_*x2sz_; }

protected:

bool doWork( od_int64 start, od_int64 stop, int threadid )
{
    const int datax0sz = data_.info().getSize( 0 );
    const int datax1sz = data_.info().getSize( 1 );
    const int datax2sz = data_.info().getSize( 2 );
    const float x0step = (datax0sz-1)/(float)(x0sz_-1);
    const float x1step = (datax1sz-1)/(float)(x1sz_-1);
    const float x2step = (datax2sz-1)/(float)(x2sz_-1);

    const int facesz01 = x0sz_*x1sz_;

    int idx = start;
    int x0 = (idx%facesz01)%x0sz_;
    int x1 = (idx%facesz01)/x0sz_;
    int x2 = idx/facesz01;

    float v000, v001, v010, v011, v100, v101, v110, v111;
    v000 = v001 = v010 = v011 = v100 = v101 = v110 = v111 = mUdf(float);
    for ( ; x2<x2sz_; x2++ )
    {
	const float x2pos = x2*x2step;
	const int x2idx = (int)x2pos;
	const bool x2onedge = x2idx+1==datax2sz;
	const float x2relpos = x2pos-x2idx;

	if ( idx!=start )
	    x1 = 0;

	for ( ; x1<x1sz_; x1++ )
	{
	    const float x1pos = x1*x1step;
	    const int x1idx = (int)x1pos;
	    const bool x1onedge = x1idx+1==datax1sz;
	    const float x1relpos = x1pos-x1idx;

	    if ( idx!=start )
		x0 = 0;

	    float x0pos = x0*x0step;
	    int x0idx = (int)x0pos;
	    v000 = data_.get( x0idx, x1idx, x2idx );
	    if ( !x1onedge )
		v010 = data_.get( x0idx, x1idx+1, x2idx );
	    if ( !x2onedge )
		v001 = data_.get( x0idx, x1idx, x2idx+1 );
	    if ( !x1onedge && !x2onedge )
		v011 = data_.get( x0idx, x1idx+1, x2idx+1 );

	    for ( ; x0<x0sz_; x0++ )
	    {
		x0pos = x0*x0step;
		x0idx = (int)x0pos;
		const bool x0onedge = x0idx+1==datax0sz;
		const float x0relpos = x0pos-x0idx;

		if ( !x0onedge )
		    v100 = data_.get( x0idx+1, x1idx, x2idx );
		if ( !x0onedge && !x1onedge )
		    v110 = data_.get( x0idx+1, x1idx+1, x2idx );
		if ( !x0onedge && !x2onedge )
		    v101 = data_.get( x0idx+1, x1idx, x2idx+1 );
		if ( !x0onedge && !x1onedge && !x2onedge )
		    v111 = data_.get( x0idx+1, x1idx+1, x2idx+1 );

		const float val =
		    Interpolate::linearReg3DWithUdf( v000, v100, v010, v110,
			    v001, v101, v011, v111,
			    x0relpos, x1relpos, x2relpos);

		res_[idx] = val;
		idx++;
		if ( idx>stop )
		    return true;

		v000 = v100;
		v010 = v110;
		v001 = v101;
		v011 = v111;
	    }
	}
    }

    pErrMsg("Hmm");
    return false;
}

    const Array3D<float>& 	data_;
    float*			res_;
    int				x0sz_, x1sz_, x2sz_;	    
};


Texture3::Texture3()
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


Texture3::~Texture3()
{
}


void Texture3::setTextureSize( int x0, int x1, int x2 )
{ 
    x0sz = x0; x1sz = x1; x2sz=x2;
    if ( texture )
	texture->images.setValue( SbVec3s( x0sz, x1sz, x2sz ),
				  usesTransperancy() ? 4 : 3, 0 );
}


int Texture3::getTextureSize( int dim ) const
{
    if ( !dim ) return x0sz;
    
    return dim==1 ? x1sz : x2sz;
}


void Texture3::setData( const Array3D<float>* newdata, DataType sel )
{
    if ( !newdata )
    {
	setResizedData( 0, 0, sel );
	return;
    }

#define mMaxTextSz 512
#define mMinTextSz 64

    int maxsize0 = mMaxTextSz;
    int maxsize1 = mMaxTextSz;
    int maxsize2 = mMaxTextSz;

    const char* envlimit = GetEnvVar("DTECT_3DTEXTURE_LIMIT");
    if ( envlimit && *envlimit )
    {
	int dummy;
	const char* firstend = strchr(envlimit,'x');
	if ( firstend )
	{
	    BufferString first = envlimit;
	    first.buf()[firstend-envlimit] = '\0';
	    BufferString second = firstend+1;
	    if ( getFromString( dummy, first ))
		maxsize0 = mMAX(mMinTextSz,dummy);

	    const char* secondend = strchr(second,'x');

	    if ( secondend )
	    {
		second.buf()[secondend-second.buf()] = '\0';
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

    int newx0 = nextPower2( newdata->info().getSize(0), mMinTextSz, maxsize0 );
    int newx1 = nextPower2( newdata->info().getSize(1), mMinTextSz, maxsize1 );
    int newx2 = nextPower2( newdata->info().getSize(2), mMinTextSz, maxsize2 );
    if ( resolution )
    {
	newx0 *= resolution;
	newx1 *= resolution;
	newx2 *= resolution;
    }

    setTextureSize( newx0, newx1, newx2 );

    const int cachesz = newx0*newx1*newx2;
    mDeclareAndTryAlloc( float*,  resized, float[cachesz] );
    if ( !resized )
    {
	setResizedData( 0, 0, sel );
	return;
    }

    Texture3InterpolFiller filler( resized, newx0, newx1, newx2, *newdata );
    filler.execute();

    setResizedData( resized, cachesz, sel );
}


unsigned char* Texture3::getTexturePtr()
{
    SbVec3s dimensions;
    int components;
    return texture->images.startEditing( dimensions, components ); 
}


void Texture3::finishEditing()
{
    texture->images.finishEditing(); 
    texture->touch();
}

}; // namespace visBase
