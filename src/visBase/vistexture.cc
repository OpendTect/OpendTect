/*
___________________________________________________________________

 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: vistexture.cc,v 1.19 2003-06-02 08:08:26 nanne Exp $";

#include "vistexture.h"

#include "arrayndimpl.h"
#include "basictask.h"
#include "dataclipper.h"
#include "iopar.h"
#include "scaler.h"
#include "simpnumer.h"
#include "thread.h"
#include "viscolortab.h"
#include "visdataman.h"
#include "visthread.h"

#include "Inventor/nodes/SoSwitch.h"
#include "Inventor/nodes/SoGroup.h"
#include "Inventor/nodes/SoComplexity.h"

#define NRCOLORS 256


const char* visBase::Texture::colortabstr = "ColorTable ID";
const char* visBase::Texture::usestexturestr = "Uses texture";
const char* visBase::Texture::texturequalitystr = "Texture quality";
const char* visBase::Texture::resolutionstr = "Resolution";

visBase::Texture::Texture()
    : indexcache( 0 )
    , cachesize( 0 )
    , threadworker( 0 )
    , colortab(0)
    , colortabcolors( new ::Color[NRCOLORS] )
    , usetrans( true )
    , resolution(0)
    , histogram( NRCOLORS, 0 )
    , onoff( new SoSwitch )
    , texturegrp( new SoGroup )
    , quality( new SoComplexity )
{
    datacache.allowNull(true);
    for ( int idx=0; idx<5; idx++ )
    {
	datacache += 0;
	datascales += LinScaler( 0, 1 );
    }

    onoff->ref();
    onoff->addChild( texturegrp );
    texturegrp->insertChild( quality, 0 );
    quality->textureQuality.setValue( 1 );

    setColorTab( *visBase::VisColorTab::create() );
}


visBase::Texture::~Texture()
{
    deepEraseArr( datacache );
    delete [] indexcache;
    delete [] colortabcolors;
    setThreadWorker( 0 );
    if ( colortab )
    {
	colortab->rangechange.remove(
		mCB( this, visBase::Texture, colorTabChCB ));
	colortab->sequencechange.remove(
		mCB( this, visBase::Texture, colorSeqChCB ));
	colortab->autoscalechange.remove(
		mCB( this, visBase::Texture, autoscaleChCB ));
	colortab->unRef();
    }

    onoff->unref();
}


bool visBase::Texture::turnOn( bool yn )
{
    bool res = isOn();
    onoff->whichChild = yn ? 0 : SO_SWITCH_NONE;
    return res;
}


bool visBase::Texture::isOn() const
{ return !onoff->whichChild.getValue(); }


void visBase::Texture::setTextureQuality( float q )
{
    quality->textureQuality.setValue( q );
}


float visBase::Texture::getTextureQuality() const
{
    return quality->textureQuality.getValue();
}


void visBase::Texture::setAutoScale( bool yn )
{
    colortab->setAutoScale( yn );
}


bool visBase::Texture::autoScale() const
{ return colortab->autoScale(); }


void visBase::Texture::setColorTab( VisColorTab& newct )
{
    if ( colortab )
    {
	colortab->rangechange.remove(
		mCB( this, visBase::Texture, colorTabChCB ));
	colortab->sequencechange.remove(
		mCB( this, visBase::Texture, colorSeqChCB ));
	colortab->autoscalechange.remove(
		mCB( this, visBase::Texture, autoscaleChCB ));
	colortab->unRef();
    }

    colortab = &newct;
    colortab->rangechange.notify( mCB( this, visBase::Texture, colorTabChCB ));
    colortab->sequencechange.notify( mCB( this, visBase::Texture,colorSeqChCB));
    colortab->autoscalechange.notify(mCB( this,visBase::Texture,autoscaleChCB));
    colortab->ref();
    colortab->setNrSteps(NRCOLORS-1);
    makeColorTables();

    makeColorIndexes();

}


visBase::VisColorTab& visBase::Texture::getColorTab()
{ return *colortab; }


void visBase::Texture::setClipRate( float nv )
{
    colortab->setClipRate( nv );
}


float visBase::Texture::clipRate() const
{ return colortab->clipRate(); }


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
{ return threadworker; }


SoNode* visBase::Texture::getData()
{ return onoff; }


void visBase::Texture::clearDataCache( bool onlycolor )
{
    int start = onlycolor ? 1 : 0;
    for ( int idx=start; idx<5; idx++ )
    {
	if ( datacache[idx] )
	{
	    delete [] datacache[idx];
	    datacache.replace( 0, idx );
	}
    }
}


void visBase::Texture::setResizedData( float* newdata, int sz, DataType dt_ )
{
    const int dt = (int)dt_;
    if ( !newdata || dt )
	clearDataCache( true );

    if ( !newdata )
    {
	makeColorIndexes();
	makeTexture();
	return;
    }

    datacache.replace( newdata, dt );
    cachesize = sz;

    if ( colortab->autoScale() )
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


void visBase::Texture::autoscaleChCB(CallBacker*)
{
    clipData();
}


void visBase::Texture::clipData()
{
    DataClipper clipper( colortab->clipRate() );
    for ( int idx=Texture::Transparency; idx<=Texture::Brightness; idx++ )
    {
	if ( !datacache[idx] )
	    continue;

	clipper.putData(datacache[idx], cachesize );
	clipper.calculateRange();
	datascales[idx].factor = 1.0/clipper.getRange().width();
	datascales[idx].constant = -clipper.getRange().start * 
	    						datascales[idx].factor;
    }

    if ( datacache[Texture::Color] )
	colortab->scaleTo( datacache[Texture::Color], cachesize );
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

    if ( !datacache[Texture::Color] ) return;

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
	maker->datacache = datacache[Texture::Color];
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

    int max = 0;
    for ( int idx=0; idx<NRCOLORS; idx++ )
    {
	int sum = 0;
	for ( int idy=0; idy<colorindexers.size(); idy++ )
	    sum += colorindexers[idy]->histogram[idx];

	if ( !idx || idx>NRCOLORS-3 )
	    sum = 0;

	if ( sum > max )
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
    ::Color*			colortabcolors;
    TypeSet<LinScaler>*		datascales;
    const float*		transdata;
    const float*		huedata;
    const float*		saturationdata;
    const float*		brightnessdata;

    int				start;
    int				stop;
    bool			usetrans;
protected:
    int	nextStep()
	{
	    texture += start*(usetrans ? 4 : 3);
	    int pos = 0;
	    for ( int idx=start; idx<stop; idx++ )
	    {
		unsigned char coltabpos = indexcache[idx];
		Color col = colortabcolors[coltabpos];

		if ( huedata || saturationdata || brightnessdata )
		{
		    unsigned char h, s, v;
		    col.getHSV(h,s,v);
		    if ( huedata )
		    {
			float th = 255 * (*datascales)[visBase::Texture::Hue]
					.scale(huedata[idx]);

			if ( th<0 ) th=0;
			else if ( th>255 ) th=255;

			h = mNINT(th);
		    }
		    if ( saturationdata )
		    {
			float ts = 255 *
			    	(*datascales)[visBase::Texture::Saturation]
				    .scale(saturationdata[idx]);
			if ( ts<0 ) ts=0;
			else if ( ts>255 ) ts=255;
			s = mNINT(ts);
		    }
		    if ( brightnessdata )
		    {
			float tv = 255 *
			    (*datascales)[visBase::Texture::Brightness]
				    .scale(brightnessdata[idx]);
			if ( tv<0 ) tv=0;
			else if ( tv>255 ) tv=255;
			v = mNINT(tv);
		    }

		    col.setHSV(h,s,v);
		}

		texture[pos++] = col.r();
		texture[pos++] = col.g();
		texture[pos++] = col.b();

		if ( usetrans )
		{
		    if ( transdata )
		    {
			int trans = 255 * 
			    mNINT((*datascales)[visBase::Texture::Transparency]
			    .scale(transdata[idx] ));
			if ( trans<0 ) trans=0;
			else if ( trans>255 ) trans=255;
			texture[pos++] = 255-trans;
		    }
		    else
		    {
			texture[pos++] = 255-col.t();
		    }
		}
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
	    maker->colortabcolors = colortabcolors;

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
	maker->datascales = &datascales;
	maker->transdata = datacache[Transparency];
	maker->huedata = datacache[Hue];
	maker->saturationdata = datacache[Saturation];
	maker->brightnessdata = datacache[Brightness];
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
	colortabcolors[idx] = colortab->tableColor( idx );
}


int visBase::Texture::nextPower2( int nr, int minnr, int maxnr ) const
{
    if ( nr > maxnr )
	return maxnr;

    int newnr = minnr;
    while ( nr > newnr )
	newnr *= 2;

    return newnr;
}


void visBase::Texture::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    SceneObject::fillPar( par, saveids );

    int ctid = colortab->id();
    par.set( colortabstr, ctid );

    par.set( texturequalitystr, getTextureQuality() );
    par.setYN( usestexturestr, isOn() );
    par.set( resolutionstr, resolution );

    if ( saveids.indexOf(ctid) == -1 ) saveids += ctid;
}


int visBase::Texture::usePar( const IOPar& par )
{
    int res = SceneObject::usePar( par );
    if ( res != 1 ) return res;

    int coltabid;
    if ( !par.get( colortabstr, coltabid ) ) return -1;
    DataObject* dataobj = DM().getObj( coltabid );
    if ( !dataobj ) return 0;
    
    mDynamicCastGet(VisColorTab*,coltab,dataobj);
    if ( !coltab ) return -1;

    setColorTab( *coltab );

    int newres = 0;
    par.get( resolutionstr, newres );
    setResolution( newres );

    float texturequality = 1;
    par.get( texturequalitystr, texturequality );
    setTextureQuality( texturequality );

    bool usetext = true;
    par.getYN( usestexturestr, usetext );
    turnOn( usetext );

    return 1;
}
