/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: vistexture2.cc,v 1.31 2005-02-04 14:31:34 kristofer Exp $";

#include "vistexture2.h"
#include "viscolortab.h"

#include "arrayndimpl.h"
#include "interpol.h"
#include "simpnumer.h"
#include <limits.h>

#include "Inventor/nodes/SoSwitch.h"
#include "Inventor/nodes/SoTexture2.h"

namespace visBase
{

mCreateFactoryEntry( Texture2 );

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


Texture2::Texture2()
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


Texture2::~Texture2()
{
    texturegrp->removeChild( texture );
}


void Texture2::setTextureSize( int x0, int x1 )
{ 
    x0sz = x0; x1sz = x1;
    texture->image.setValue( SbVec2s( x1sz, x0sz ), 
	    		     usesTransperancy() ? 4 : 3, 0 );
}


#define mSodOff { x0sz = x1sz = -1; setResizedData( 0, 0, sel ); return; }


void Texture2::setData( const Array2D<float>* newdata, DataType sel )
{
    if ( !newdata )
    {
	if ( !sel ) setTextureSize(0,0);
	mSodOff
    }

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

    if ( isDataClassified(newdata) )
	nearestValInterp( newsize, newdata, resized );
    else
	polyInterp( newsize, newdata, resized );

    setResizedData( resized, cachesz, sel );
}


static const int sMaxNrClasses = 100;

bool Texture2::isDataClassified( const Array2D<float>* newdata ) const
{
    const int datax0size = newdata->info().getSize(0);
    const int datax1size = newdata->info().getSize(1);
    for ( int x0=0; x0<datax0size; x0++ )
    {
	int nrint = 0;
	for ( int x1=0; x1<datax1size; x1++ )
	{
	    const float val = newdata->get( x0, x1 );
	    if ( mIsUndefined(val) ) continue;
	    const int ival = mNINT(val);
	    if ( !mIsEqual(val,ival,mDefEps)
	      || ival > sMaxNrClasses ) return false;
	    nrint++;
	    if ( nrint > 100 ) break;
	}
    }

    return true;
}


void Texture2::polyInterp( const Array2DInfoImpl& newsize,
				    const Array2D<float>* newdata, float* res )
{
    const int datax0size = newdata->info().getSize(0);
    const int datax1size = newdata->info().getSize(1);
    const float x0step = (datax0size-1)/(float)(x0sz-1);
    const float x1step = (datax1size-1)/(float)(x1sz-1);

    float val; const float udf = mUndefValue;
    for ( int x0=0; x0<x0sz; x0++ )
    {
	const float x0pos=x0*x0step;
	const int x0idx = (int)x0pos;
	const float x0relpos = x0pos-x0idx;
	const bool x0m1udf = x0idx == 0;
	const bool x0p2udf = x0idx >= datax0size-2;
	const bool x0p1udf = x0idx == datax0size-1;

	for ( int x1=0; x1<x1sz; x1++ )
	{
	    const float x1pos = x1*x1step;
	    const int x1idx = (int)x1pos;
	    const float x1relpos = x1pos-x1idx;
	    const bool x1m1udf = x1idx == 0;
	    const bool x1p2udf = x1idx >= datax1size-2;
	    const bool x1p1udf = x1idx == datax1size-1;

	    const float v01 = x0m1udf ? udf
		: newdata->get( x0idx-1, x1idx );
	    const float v02 = x0m1udf || x1p1udf ? udf
		: newdata->get( x0idx-1, x1idx+1 );
	    const float v10 = x1m1udf ? udf
		: newdata->get( x0idx,   x1idx-1 );
	    const float v11 =
		  newdata->get( x0idx,   x1idx );
	    const float v12 = x1p1udf ? udf
		: newdata->get( x0idx,   x1idx+1 );
	    const float v13 = x1p2udf ? udf
		: newdata->get( x0idx,   x1idx+2 );
	    const float v20 = x0p1udf || x1m1udf ? udf
		: newdata->get( x0idx+1, x1idx-1 );
	    const float v21 = x0p1udf ? udf
		: newdata->get( x0idx+1, x1idx );
	    const float v22 = x0p1udf || x1p1udf ? udf
		: newdata->get( x0idx+1, x1idx+1 );
	    const float v23 = x0p1udf || x1p2udf ? udf
		: newdata->get( x0idx+1, x1idx+2 );
	    const float v31 = x0p2udf ? udf
		: newdata->get( x0idx+2, x1idx );
	    const float v32 = x0p2udf || x1p1udf ? udf
		: newdata->get( x0idx+2, x1idx+1 );

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

	    res[newsize.getMemPos(x0,x1)] = val;
	}
    }
}


void Texture2::nearestValInterp( const Array2DInfoImpl& newsize,
				    const Array2D<float>* newdata, float* res )
{
    const int datax0size = newdata->info().getSize(0);
    const int datax1size = newdata->info().getSize(1);
    const float x0step = (datax0size-1)/(float)(x0sz-1);
    const float x1step = (datax1size-1)/(float)(x1sz-1);

    for ( int x0=0; x0<x0sz; x0++ )
    {
	const float x0pos=x0*x0step;
	const int x0idx = (int)x0pos;
	const float x0relpos = x0pos-x0idx;

	for ( int x1=0; x1<x1sz; x1++ )
	{
	    const float x1pos = x1*x1step;
	    const int x1idx = (int)x1pos;
	    const float x1relpos = x1pos-x1idx;

	    const int x0nearest = mNINT(x0relpos);
	    const int x1nearest = mNINT(x1relpos);
	    res[newsize.getMemPos(x0,x1)] = 
			    newdata->get( x0idx+x0nearest, x1idx+x1nearest );
	}
    }
}


unsigned char* Texture2::getTexturePtr()
{
    SbVec2s dimensions;
    int components;
    return texture->image.startEditing( dimensions, components );
}


void Texture2::finishEditing()
{ 
    texture->image.finishEditing();
    texture->touch();
}


mCreateFactoryEntry( Texture2Set );

Texture2Set::Texture2Set()
    : textureswitch(new SoSwitch)
    , shareres(true)
    , sharecolseq(true)
{
    textureswitch->ref();
}


Texture2Set::~Texture2Set()
{
    removeAll(false);
    textureswitch->unref();
}


#define mColTabCB \
    mCB(this,Texture2Set,colTabChanged)

void Texture2Set::addTexture( Texture2* text )
{
    if ( !text ) return;
    if ( textureset.size() )
    {
	if ( shareres )
	    text->setResolution( textureset[0]->getResolution() );
	if ( sharecolseq )
	    text->getColorTab().setColorSeq( 
		    		&textureset[0]->getColorTab().colorSeq() );
    }

    textureset += text;
    text->ref();
    textureswitch->addChild( text->getInventorNode() );
}


void Texture2Set::removeTexture( Texture2* text )
{
    if ( !text ) return;
    textureswitch->removeChild( text->getInventorNode() );
    textureset -= text;
    text->unRef();
}


void Texture2Set::removeTexture( int idx )
{
    Texture2* text = idx>=0 || idx<textureset.size() ? textureset[idx]
							      : 0;
    removeTexture( text );
}


void Texture2Set::removeAll( bool keepfirst )
{
    int minsz = keepfirst ? 1 : 0;
    while ( textureset.size() > minsz )
    {
        const int idx = textureset.size()-1;
        Texture2* text = textureset[idx];
	text->getColorTab().rangechange.remove( mColTabCB );
	text->getColorTab().autoscalechange.remove( mColTabCB );
        textureswitch->removeChild( text->getInventorNode() );
        text->unRef();
        textureset.remove( idx );
    }

    if ( keepfirst )
	setActiveTexture( 0 );
}


int Texture2Set::nrTextures() const
{
    return textureset.size();
}


Texture2* Texture2Set::getTexture( int idx ) const
{
    return idx>=0 && idx<textureset.size() ? textureset[idx] : 0;
}


void Texture2Set::setActiveTexture( int idx )
{
    int nrchildren = textureswitch->getNumChildren();
    textureswitch->whichChild = 
	idx < 0 ? SO_SWITCH_NONE : ( idx >= nrchildren ? nrchildren-1 : idx ); 
}


Texture2* Texture2Set::activeTexture() const
{
    int idx = textureswitch->whichChild.getValue();
    return getTexture(idx);
}


SoNode* Texture2Set::getInventorNode()
{
    return textureswitch;
}


void Texture2Set::finishTextures()
{
    if ( sharecolseq )
    {
	for ( int idx=0; idx<textureset.size(); idx++ )
	{
	    VisColorTab& ct = textureset[idx]->getColorTab();
	    ct.rangechange.notify( mColTabCB );
	    ct.autoscalechange.notify( mColTabCB );
	}
    }
}


void Texture2Set::colTabChanged( CallBacker* cb )
{
    mDynamicCastGet(VisColorTab*,ct,cb)
    if ( !ct ) return;
    ct->autoscalechange.remove( mColTabCB );
    ct->rangechange.remove( mColTabCB );
    int curidx = textureswitch->whichChild.getValue();
    bool autoscale = ct->autoScale();
    float cliprate = ct->clipRate();
    for ( int idx=0; idx<textureset.size(); idx++ )
    {
	if ( idx==curidx ) continue;
	VisColorTab& coltab = textureset[idx]->getColorTab();
	coltab.autoscalechange.remove( mColTabCB );
	coltab.rangechange.remove( mColTabCB );
	if ( autoscale )
	{
	    VisColorTab* newct = VisColorTab::create();
	    textureset[idx]->setColorTab( *newct );
	    newct->setClipRate( cliprate );
	    newct->triggerAutoScaleChange();
	    newct->autoscalechange.notify( mColTabCB );
	    newct->rangechange.notify( mColTabCB );
	}
	else if ( &coltab != ct )
	{
	    textureset[idx]->setColorTab( *ct );
	    ct->triggerRangeChange();
	}
    }

    ct->autoscalechange.notify( mColTabCB );
    ct->rangechange.notify( mColTabCB );
}

}; // namespace visBase
