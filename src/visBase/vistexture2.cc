/*
___________________________________________________________________

 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: vistexture2.cc,v 1.1 2003-01-03 11:20:51 kristofer Exp $";

#include "vistexture2.h"

#include "arrayndimpl.h"
#include "dataclipper.h"
#include "simpnumer.h"
#include "viscolortab.h"

#include "Inventor/nodes/SoGroup.h"
#include "Inventor/nodes/SoTexture2.h"


visBase::Texture2::Texture2()
    : cachedata( 0 )
    , cacheddataindexes( 0 )
    , x0sz( -1 )
    , x1sz( -1 )
    , autoscale( true )
    , colortab( 0 )
    , texture( new SoTexture2 )
    , root( new SoGroup )
    , dataclipper( *new DataClipper( 0.05) )
{
    root->ref();
    root->addChild( texture );

    setColorTab( *visBase::VisColorTab::create());
}


visBase::Texture2::~Texture2()
{
    colortab->unRef();
    root->unref();
    delete &dataclipper;
}


void visBase::Texture2::setTextureSize( int x0, int x1 )
{ x0sz = x0; x1sz = x1; }


void visBase::Texture2::setAutoScale( bool yn )
{
    autoscale = yn;
    reClipData();
}


bool visBase::Texture2::autoScale() const
{ return autoscale; }


void visBase::Texture2::setColorTab( VisColorTab& newct )
{
    if ( colortab )
    {
	colortab->rangechange.remove(
		mCB( this, visBase::Texture2, colorTabChCB ));
	colortab->sequencechange.remove(
		mCB( this, visBase::Texture2, colorSeqChCB ));
	colortab->unRef();
    }

    colortab = &newct;
    colortab->rangechange.notify( mCB( this, visBase::Texture2, colorTabChCB ));
    colortab->sequencechange.notify( mCB( this, visBase::Texture2,
					colorSeqChCB ));
    colortab->ref();
    colortab->setNrSteps(255);
    reMapData();
}


visBase::VisColorTab& visBase::Texture2::getColorTab()
{ return *colortab; }


void visBase::Texture2::setClipRate( float nv )
{
    dataclipper.setClipRate( nv );
    reClipData();
}


float visBase::Texture2::clipRate() const
{ return dataclipper.clipRate(); }


void visBase::Texture2::setData( const Array2D<float>* newdata )
{
    if ( x0sz==-1 || x1sz==-1 )
    {
	x0sz = 128;
	x1sz = 128;
	//TODO Change this to nearest bigger sz from data
    }

    const int datax0size = newdata->info().getSize(0);
    const int datax1size = newdata->info().getSize(1);

    if ( cachedata ) delete cachedata;
    cachedata = new Array2DImpl<float>( x0sz, x1sz );

    const float x0step = (datax0size-1)/(float)(x0sz-1);
    const float x1step = (datax1size-1)/(float)(x1sz-1);

    for ( int x0=0; x0<x0sz; x0++ )
    {
	const float x0pos=x0*x0step;
	const int x0idx = (int) x0pos;
	const bool x0onedge = x0pos==datax0size;
	const float x0relpos = x0pos-x0idx;

	for ( int x1=0; x1<x1sz; x1++ )
	{
	    const float x1pos = x1*x1step;
	    const int x1idx = (int) x1pos;
	    const bool x1onedge = x1pos==datax1size;
	    const float x1relpos = x1pos-x1idx;

	    const float val00 = newdata->get( x0idx, x1idx );
	    float val10, val01, val11;

	    if ( !x0onedge )
		val10 = newdata->get( x0idx+1, x1idx );
	    if ( !x1onedge )
		val01 = newdata->get( x0idx, x1idx+1 );
	    if ( !x0onedge && !x1onedge )
		val11 = newdata->get( x0idx+1, x1idx+1 );

	    float val = 0;
	    if ( x0onedge && x0onedge )
		val = val00;
	    else if ( x0onedge )
		val = linearInterpolate( val00, val01, x1relpos );
	    else if ( x1onedge )
		val = linearInterpolate( val00, val10, x0relpos );
	    else
		val = linearInterpolate2D( val00, val01, val10, val11,
			x0relpos, x1relpos );

	    cachedata->set( x0, x1, val );
	}
    }

    if ( autoscale ) reClipData();
}


SoNode* visBase::Texture2::getData()
{ return root; }


void visBase::Texture2::colorTabChCB(CallBacker*)
{
    reMapData();
}


void visBase::Texture2::colorSeqChCB(CallBacker*)
{
    updateTexture();
}


void visBase::Texture2::reClipData()
{
    if ( cachedata ) return;

    const float* vals = cachedata->getData();
    const int nrvals = cachedata->info().getTotalSz();

    dataclipper.putData( vals, nrvals );
    dataclipper.calculateRange();
    colortab->scaleTo( dataclipper.getRange() );
}


void visBase::Texture2::reMapData()
{
    if ( !cachedata ) return;

    if ( !cacheddataindexes || cacheddataindexes->info()!=cachedata->info() )
    {
	delete cacheddataindexes;
	cacheddataindexes = new  Array2DImpl<unsigned char>( cachedata->info());
    }

    unsigned char* colmapdata = cacheddataindexes->getData();
    float* data = cachedata->getData();

    const int nrvals = cachedata->info().getTotalSz();

    for ( int idx=0; idx<nrvals; idx++ )
	colmapdata[idx] = colortab->colIndex(data[idx]);

    updateTexture();
}


void visBase::Texture2::updateTexture()
{
    if ( !cacheddataindexes ) return;

    const int nrvals = cachedata->info().getTotalSz();

    const int nrofcomponents = 4;  

    ArrPtrMan<unsigned char> imagedata =
			 new unsigned char[x0sz*x1sz*nrofcomponents];

    unsigned char* colmapdata = cacheddataindexes->getData();
    int pos = 0;
    for ( int idx=0; idx<nrvals; idx++ )
    {
	Color color = colortab->tableColor( colmapdata[idx] );
	imagedata[pos++] = color.r();
	imagedata[idx++] = color.g();
	imagedata[idx++] = color.b();
	if ( nrofcomponents==4 ) imagedata[idx++] = 255 - color.t();
    }

    texture->image.setValue( SbVec2s( x0sz, x1sz ), nrofcomponents, imagedata );
}


