/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Jan 2003
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "vistexture2.h"
#include "viscolortab.h"
#include "arrayndimpl.h"
#include "interpol2d.h"
#include "simpnumer.h"

#include <limits.h>

#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoTexture2.h>

mCreateFactoryEntry( visBase::Texture2 );
mCreateFactoryEntry( visBase::Texture2Set );

namespace visBase
{

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


bool Texture2::isDataClassified( const Array2D<float>* newdata ) const
{
    if ( !newdata ) return false;

    const od_int64 datax0size = newdata->info().getSize(0);
    const od_int64 datax1size = newdata->info().getSize(1);
    return holdsClassValues( newdata->getData(), datax0size * datax1size );
}


void Texture2::polyInterp( const Array2DInfoImpl& newsize,
				    const Array2D<float>* newdata, float* res )
{
    const int datax0size = newdata->info().getSize(0);
    const int datax1size = newdata->info().getSize(1);
    const float x0step = (datax0size-1)/(float)(x0sz-1);
    const float x1step = (datax1size-1)/(float)(x1sz-1);

    const float udf = mUdf(float);
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

	    const float vm10 = x0m1udf ? udf
		: newdata->get( x0idx-1, x1idx );
	    const float vm11 = x0m1udf || x1p1udf ? udf
		: newdata->get( x0idx-1, x1idx+1 );
	    const float v0m1 = x1m1udf ? udf
		: newdata->get( x0idx, x1idx-1 );
	    const float v00 =
		  newdata->get( x0idx, x1idx );
	    const float v01 = x1p1udf ? udf
		: newdata->get( x0idx, x1idx+1 );
	    const float v02 = x1p2udf ? udf
		: newdata->get( x0idx, x1idx+2 );
	    const float v1m1 = x0p1udf || x1m1udf ? udf
		: newdata->get( x0idx+1, x1idx-1 );
	    const float v10 = x0p1udf ? udf
		: newdata->get( x0idx+1, x1idx );
	    const float v11 = x0p1udf || x1p1udf ? udf
		: newdata->get( x0idx+1, x1idx+1 );
	    const float v12 = x0p1udf || x1p2udf ? udf
		: newdata->get( x0idx+1, x1idx+2 );
	    const float v20 = x0p2udf ? udf
		: newdata->get( x0idx+2, x1idx );
	    const float v21 = x0p2udf || x1p1udf ? udf
		: newdata->get( x0idx+2, x1idx+1 );

	    res[newsize.getOffset(x0,x1)] = Interpolate::polyReg2DWithUdf(
		    vm10,vm11,v0m1,v00,v01,v02,v1m1,v10,v11,v12,v20,v21,
		    x0relpos, x1relpos );
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

	    const int x0nearest = mNINT32(x0relpos);
	    const int x1nearest = mNINT32(x1relpos);
	    res[newsize.getOffset(x0,x1)] = 
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


// Texture2Set ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

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
    return idx<0 || idx>=textureset.size() ? 0
	 : const_cast<Texture2*>( textureset[idx] );
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


int Texture2Set::activeTextureNr() const
{
    return  textureswitch->whichChild.getValue();
}


SoNode* Texture2Set::gtInvntrNode()
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
    Interval<float> cliprate = ct->clipRate();
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
