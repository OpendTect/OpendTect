/*
___________________________________________________________________

 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: vistexture.cc,v 1.7 2003-01-23 11:58:17 nanne Exp $";

#include "vistexture.h"

#include "arrayndimpl.h"
#include "dataclipper.h"
#include "simpnumer.h"
#include "viscolortab.h"
#include "visthread.h"
#include "basictask.h"
#include "thread.h"

#include "Inventor/nodes/SoGroup.h"
#include "Inventor/nodes/SoTexture3.h"

#define NRCOLORS 256


visBase::Texture::Texture()
    : datacache( 0 )
    , indexcache( 0 )
    , cachesize( 0 )
    , dataclipper( *new DataClipper( 0.05) )
    , threadworker( 0 )
    , colortab(0)
    , red( new unsigned char[NRCOLORS] )
    , green( new unsigned char[NRCOLORS] )
    , blue( new unsigned char[NRCOLORS] )
    , trans( new unsigned char[NRCOLORS] )
    , autoscale( true )
    , usetrans( true )
    , histogram( NRCOLORS, 0 )
{
    setColorTab( *visBase::VisColorTab::create() );
}


visBase::Texture::~Texture()
{
    delete [] datacache;
    delete [] indexcache;
    delete [] red;
    delete [] green;
    delete [] blue;
    delete &dataclipper;
    setThreadWorker( 0 );
    colortab->unRef();
}


void visBase::Texture::setAutoScale( bool yn )
{
    autoscale = yn;
    if ( autoscale )
	clipData();
}


bool visBase::Texture::autoScale() const
{ return autoscale; }


void visBase::Texture::setColorTab( VisColorTab& newct )
{
    if ( colortab )
    {
	colortab->rangechange.remove(
		mCB( this, visBase::Texture, colorTabChCB ));
	colortab->sequencechange.remove(
		mCB( this, visBase::Texture, colorSeqChCB ));
	colortab->unRef();
    }

    colortab = &newct;
    colortab->rangechange.notify( mCB( this, visBase::Texture, colorTabChCB ));
    colortab->sequencechange.notify( mCB( this, visBase::Texture,colorSeqChCB));
    colortab->ref();
    colortab->setNrSteps(NRCOLORS-1);
    makeColorTables();

    makeColorIndexes();

}


visBase::VisColorTab& visBase::Texture::getColorTab()
{ return *colortab; }


void visBase::Texture::setClipRate( float nv )
{
    dataclipper.setClipRate( nv );
    makeColorIndexes();
    makeTexture();
}


float visBase::Texture::clipRate() const
{ return dataclipper.clipRate(); }


const TypeSet<float>& visBase::Texture::getHistogram() const
{
    return histogram;
}


void visBase::Texture::setUseTransperancy( bool yn )
{
    if ( yn==usetrans ) return;

    usetrans = yn;
    makeTexture();
}


bool visBase::Texture::usesTransperancy() const
{ return usetrans; }


void visBase::Texture::setThreadWorker( ThreadWorker* nw )
{
    if ( threadworker )
	threadworker->unRef();

    threadworker = nw;

    if ( threadworker )
	threadworker->ref();
}


visBase::ThreadWorker* visBase::Texture::getThreadWorker()
{
    return threadworker;
}


void visBase::Texture::setResizedData( float* newdata, int sz )
{
    delete [] datacache;
    if ( !newdata )
    {
	datacache = 0;
	makeColorIndexes();
	makeTexture();
	return;
    }

    datacache = newdata;
    cachesize = sz;

    if ( autoscale )
    {
	clipData();		// Will trigger cbs so everything is updated
    }
    else
    {
	makeColorIndexes();
	makeTexture();
    }
}
    

void visBase::Texture::colorTabChCB(CallBacker*)
{
    makeColorTables();
    makeColorIndexes();
    makeTexture();
}


void visBase::Texture::colorSeqChCB(CallBacker*)
{
    makeColorTables();
    makeTexture();
}


void visBase::Texture::clipData()
{
    if ( !datacache ) return;

    dataclipper.putData( datacache, cachesize );
    dataclipper.calculateRange();
    colortab->scaleTo( dataclipper.getRange() );
}


class visBaseTextureColorIndexMaker : public BasicTask
{
public:
    unsigned char*		indexcache;
    const float*		datacache;
    visBase::VisColorTab*	colortab;
    int				start;
    int				stop;

    TypeSet<int>		histogram;
protected:
    int		nextStep()
		{
		    histogram = TypeSet<int>( NRCOLORS, 0 );
		    for ( int idx=start; idx<stop; idx++ )
		    {
			const int colorindex =
			    colortab->colIndex(datacache[idx]);
			indexcache[idx] = colorindex;
			histogram[colorindex]++;
		    }

		    return 0;
		}
};


void visBase::Texture::makeColorIndexes()
{
    delete [] indexcache;
    indexcache = 0;

    if ( !datacache ) return;

    if ( !colorindexers.size() )
    {
	for ( int idx=0; idx<Threads::getNrProcessors(); idx++ )
	{
	    colorindexers += new visBaseTextureColorIndexMaker;
	}
    }

    indexcache = new unsigned char[cachesize];

    int border=0;
    for ( int idx=0; idx<colorindexers.size(); idx++ )
    {
	visBaseTextureColorIndexMaker* maker = colorindexers[idx];
	maker->start = border;
	border += cachesize/colorindexers.size();
	if ( idx<colorindexers.size()-1 )
	    maker->stop = border;
	else
	    maker->stop = cachesize;

	maker->indexcache = indexcache;
	maker->datacache = datacache;
	maker->colortab = colortab;
    }

    if ( threadworker )
    {
	threadworker->addWork(
		reinterpret_cast<ObjectSet<BasicTask>&>(colorindexers ));
    }
    else
    {
	for ( int idx=0; idx<colorindexers.size(); idx++ )
	{
	    while ( colorindexers[idx]->doStep() )
		;
	}
    }

    int max;
    for ( int idx=0; idx<NRCOLORS; idx++ )
    {
	int sum = 0;
	for ( int idy=0; idy<colorindexers.size(); idy++ )
	    sum += colorindexers[idy]->histogram[idx];

	if ( !idx || sum>max )
	    max = sum;

	histogram[idx] = sum;
    }

    if ( max )
    {
	for ( int idx=0; idx<NRCOLORS; idx++ )
	    histogram[idx] /= max;
    }
}


class visBaseTextureMaker : public BasicTask
{
public:
    unsigned char*		indexcache;
    unsigned char*		texture;
    unsigned char*		red;
    unsigned char*		green;
    unsigned char*		blue;
    unsigned char*		trans;

    int				start;
    int				stop;
    bool			usetrans;
protected:
    int		nextStep()
		{
		    texture += start*(usetrans ? 4 : 3);
		    int pos = 0;
		    for ( int idx=start; idx<stop; idx++ )
		    {
			unsigned char coltabpos = indexcache[idx];
			texture[pos++] = red[coltabpos];
			texture[pos++] = green[coltabpos];
			texture[pos++] = blue[coltabpos];
			if ( usetrans )
			    texture[pos++] = trans[coltabpos];
		    }

		    return 0;
		}
};


void visBase::Texture::makeTexture()
{
    if ( !indexcache )
	return;

    const int nrcomponents = usetrans ? 4 : 3;

    unsigned char* texture = getTexturePtr();

    if ( !texturemakers.size() )
    {
	for ( int idx=0; idx<Threads::getNrProcessors(); idx++ )
	{
	    visBaseTextureMaker* maker = new visBaseTextureMaker;
	    maker->red = red;
	    maker->green = green;
	    maker->blue = blue;
	    maker->trans = trans;

	    texturemakers += maker;
	}
    }

    int border=0;
    for ( int idx=0; idx<texturemakers.size(); idx++ )
    {
	visBaseTextureMaker* maker =
	   reinterpret_cast<visBaseTextureMaker*>(texturemakers[idx]);
	maker->start = border;
	border += cachesize/colorindexers.size();
	if ( idx<colorindexers.size()-1 )
	    maker->stop = border;
	else
	    maker->stop = cachesize;

	maker->indexcache = indexcache;
	maker->texture = texture;
	maker->usetrans = usetrans;
    }


    if ( threadworker )
    {
	threadworker->addWork( texturemakers );
    }
    else
    {
	for ( int idx=0; idx<texturemakers.size(); idx++ )
	{
	    while ( texturemakers[idx]->doStep() )
		;
	}
    }

    finishEditing();
}


void visBase::Texture::makeColorTables()
{
    for ( int idx=0; idx<NRCOLORS; idx++ )
    {
	Color color = colortab->tableColor( idx );
	red[idx] = color.r();
	green[idx] = color.g();
	blue[idx] = color.b();
    trans[idx] = 255-color.t();
    }
}



