/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: vistexture.cc,v 1.47 2012/07/10 13:06:10 cvskris Exp $";

#include "vistexture.h"

#include "arrayndimpl.h"
#include "task.h"
#include "iopar.h"
#include "simpnumer.h"
#include "thread.h"
#include "envvars.h"
#include "viscolortab.h"
#include "viscoltabmod.h"
#include "visdataman.h"

#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/nodes/SoComplexity.h>
#include <Inventor/nodes/SoTextureScalePolicy.h>

#define NRCOLORS 256


class visBaseTextureColorIndexMaker : public SequentialTask
{
public:
    unsigned char*		indexcache;
    const float*		datacache;
    visBase::VisColorTab*	colortab;
    int				start;
    int				stop;

    TypeSet<int>		histogram;
protected:
    int				nextStep();
};


int visBaseTextureColorIndexMaker::nextStep()
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


namespace visBase
{

const char* Texture::colortabstr()	{ return "ColorTable ID"; }
const char* Texture::usestexturestr()	{ return "Uses texture"; }
const char* Texture::texturequalitystr()  { return "Texture quality"; }
const char* Texture::resolutionstr()	  { return "Resolution"; }
const char* Texture::coltabmodstr()	  { return "ColorTableModifier ID"; }


Texture::Texture()
    : indexcache( 0 )
    , cachesize( 0 )
    , indexcachesize( 0 )
    , colortab(0)
    , colortabcolors( new ::Color[NRCOLORS] )
    , usetrans( true )
    , resolution(0)
    , histogram( NRCOLORS, 0 )
    , onoff( new SoSwitch )
    , texturegrp( new SoGroup )
    , quality( new SoComplexity )
    , datacache(0)
    , colordatacache(0)
    , curtype(Texture::Color)
{
    onoff->ref();
    onoff->addChild( texturegrp );
    texturegrp->insertChild( quality, 0 );
    quality->textureQuality.setValue( 1 );


//  TODO: Evaluate this. Can most probably be removed.
    if ( GetEnvVarYN("DTECT_USE_SCALE_POLICY") )
    {
	SoTextureScalePolicy* scalepolicy = new SoTextureScalePolicy;
	texturegrp->insertChild( scalepolicy, 1 );
	scalepolicy->policy = SoTextureScalePolicy::FRACTURE;
	float newquality = 1;
	scalepolicy->quality.setValue( newquality );
    }

    setColorTab( *VisColorTab::create() );

    coltabmod = VisColTabMod::create();
    coltabmod->ref();
}


Texture::~Texture()
{
    delete [] datacache;
    delete [] colordatacache;
    delete [] indexcache;
    delete [] colortabcolors;
    if ( colortab )
    {
	colortab->rangechange.remove(
		mCB( this, Texture, colorTabChCB ));
	colortab->sequencechange.remove(
		mCB( this, Texture, colorSeqChCB ));
	colortab->autoscalechange.remove(
		mCB( this, Texture, autoscaleChCB ));
	colortab->unRef();
    }

    onoff->unref();
    coltabmod->unRef();

    deepErase( texturemakers );
    deepErase( colorindexers );
}


bool Texture::turnOn( bool yn )
{
    bool res = isOn();
    onoff->whichChild = yn ? 0 : SO_SWITCH_NONE;
    return res;
}


bool Texture::isOn() const
{ return !onoff->whichChild.getValue(); }


void Texture::setTextureQuality( float q )
{
    quality->textureQuality.setValue( q );
}


float Texture::getTextureQuality() const
{
    return quality->textureQuality.getValue();
}


void Texture::setAutoScale( bool yn )
{
    colortab->setAutoScale( yn );
}


bool Texture::autoScale() const
{ return colortab->autoScale(); }


void Texture::setColorTab( VisColorTab& newct )
{
    if ( colortab==&newct )
	return;

    if ( colortab )
    {
	colortab->rangechange.remove(
		mCB( this, Texture, colorTabChCB ));
	colortab->sequencechange.remove(
		mCB( this, Texture, colorSeqChCB ));
	colortab->autoscalechange.remove(
		mCB( this, Texture, autoscaleChCB ));
	colortab->unRef();
    }

    colortab = &newct;
    colortab->rangechange.notify( mCB( this, Texture, colorTabChCB ));
    colortab->sequencechange.notify( mCB( this, Texture,colorSeqChCB));
    colortab->autoscalechange.notify(mCB( this,Texture,autoscaleChCB));
    colortab->ref();
    colortab->setNrSteps(NRCOLORS-1);

    makeColorTables();
    makeColorIndexes();
}


VisColorTab& Texture::getColorTab()
{ return *colortab; }


void Texture::setClipRate( Interval<float> nv )
{
    colortab->setClipRate( nv );
}


Interval<float> Texture::clipRate() const
{ return colortab->clipRate(); }


const TypeSet<float>& Texture::getHistogram() const
{
    return histogram;
}


void Texture::setUseTransperancy( bool yn )
{
    if ( yn==usetrans ) return;

    usetrans = yn;
    makeTexture();
}


bool Texture::usesTransperancy() const
{ return usetrans; }


SoNode* Texture::gtInvntrNode()
{ return onoff; }


void Texture::clearDataCache( bool all )
{
    delete [] colordatacache;
    colordatacache = 0;

    if ( all )
    {
	delete [] datacache;
	datacache = 0;
    }
}


void Texture::setResizedData( float* newdata, int sz, DataType dt_ )
{
    const int dt = (int)dt_;
    clearDataCache( newdata && !dt );
    curtype = dt_;

    if ( !newdata )
    {
	makeColorIndexes();
	makeTexture();
	return;
    }

    if ( !dt )
	datacache = newdata;
    else
	colordatacache = newdata;
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
    

void Texture::colorTabChCB(CallBacker*)
{
    makeColorTables();
    makeColorIndexes();
    makeTexture();
}


void Texture::colorSeqChCB(CallBacker*)
{
    makeColorTables();
    makeTexture();
}


void Texture::autoscaleChCB(CallBacker*)
{
    clipData();
}


void Texture::clipData()
{
    if ( colordatacache )
	coltabmod->setScale( colordatacache, cachesize );

    if ( datacache )
	colortab->scaleTo( datacache, cachesize );
}


void Texture::makeColorIndexes()
{
    if ( !datacache ) return;

    if ( colorindexers.isEmpty() )
    {
	for ( int idx=0; idx<Threads::getNrProcessors(); idx++ )
	{
	    colorindexers += new visBaseTextureColorIndexMaker;
	}
    }

    if ( indexcachesize!=cachesize )
    {
	delete [] indexcache;
	indexcache = new unsigned char[cachesize];
	indexcachesize = cachesize;
    }


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

    for ( int idx=0; idx<colorindexers.size(); idx++ )
	colorindexers[idx]->execute();

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


class visBaseTextureMaker : public SequentialTask
{
public:
    unsigned char*		indexcache;
    unsigned char*		texture;
    ::Color*			colortabcolors;
    VisColTabMod*	ctm;
    const float*		colordata;
    Texture::DataType	datatype;

    int				start;
    int				stop;
    bool			usetrans;
protected:

#define mScaleData(val,type,top,chgrgb) \
    if ( datatype==Texture::type ) \
    { \
	float fac = ctm->getScale().scale(colordata[idx]); \
	if ( ctm->isReverse() ) fac = 1 - fac; \
	float newval = (float)val * fac; \
	if ( chgrgb ) newval += (float)val; \
	if ( newval<0 ) newval=0; \
	else if ( newval>top ) newval=top; \
	val = mNINT32(newval); \
    }

    int	nextStep()
	{
	    texture += start*(usetrans ? 4 : 3);
	    int pos = 0;
	    for ( int idx=start; idx<stop; idx++ )
	    {
		unsigned char coltabpos = indexcache[idx];
		Color col = colortabcolors[coltabpos];

		if ( colordata )
		{
		    if ( (int)datatype>1 && (int)datatype<5 )
		    {
			unsigned char h, s, v;
			col.getHSV(h,s,v);
			mScaleData( h, Hue, 360, false );
			mScaleData( s, Saturation, 255, false );
			mScaleData( v, Brightness, 255, false );
			col.setHSV(h,s,v);
		    }
		    else if ( (int)datatype > 4 )
		    {
			int r = col.r(); int g = col.g(); int b = col.b();
			mScaleData( r, Red, 255, true );
			mScaleData( g, Green, 255, true );
			mScaleData( b, Blue, 255, true );
			col.set( r, g, b );
		    }
		}

		texture[pos++] = col.r();
		texture[pos++] = col.g();
		texture[pos++] = col.b();

		if ( usetrans )
		{
		    if ( colordata && datatype==1 )
		    {
			int trans = mNINT32( 255 * 
			    ctm->getScale().scale(colordata[idx]) );
			if ( trans<0 ) trans=0;
			else if ( trans>255 ) trans=255;
			if ( !ctm->isReverse() ) trans = 255-trans;
			texture[pos++] = trans;
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


void Texture::makeTexture()
{
    if ( !indexcache )
	return;

    const int nrcomponents = usetrans ? 4 : 3;

    unsigned char* texture = getTexturePtr();

    if ( texturemakers.isEmpty() )
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
	maker->colordata = colordatacache;
	maker->ctm = coltabmod;
	maker->datatype = curtype;
    }

    if ( !texture )
    {
	finishEditing();
	return;
    }


    for ( int idx=0; idx<texturemakers.size(); idx++ )
	texturemakers[idx]->execute();

    finishEditing();
}


void Texture::makeColorTables()
{
    for ( int idx=0; idx<NRCOLORS; idx++ )
    {
	colortabcolors[idx] = colortab->tableColor( idx );
    }
}


int Texture::nextPower2( int nr, int minnr, int maxnr ) const
{
    if ( nr > maxnr )
	return maxnr;

    int newnr = minnr;
    while ( nr > newnr )
	newnr *= 2;

    return newnr;
}


void Texture::setColorPars( bool rev, bool useclip,
				     const Interval<float>& intv )
{
    coltabmod->doReverse( rev );
    coltabmod->useClipping( useclip );
    useclip ? coltabmod->setClipRate( intv.start, intv.stop )
	    : coltabmod->setRange( intv );
}


const Interval<float>& Texture::getColorDataRange() const
{
    return coltabmod->getRange();
}


void Texture::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    DataObject::fillPar( par, saveids );

    int ctid = colortab->id();
    par.set( colortabstr(), ctid );

    par.set( texturequalitystr(), getTextureQuality() );
    par.setYN( usestexturestr(), isOn() );
    par.set( resolutionstr(), resolution );

    int ctmid = coltabmod->id();
    par.set( coltabmodstr(), ctmid );

    if ( saveids.indexOf(ctid) == -1 ) saveids += ctid;
    if ( saveids.indexOf(ctmid) == -1 ) saveids += ctmid;
}


int Texture::usePar( const IOPar& par )
{
    int res = DataObject::usePar( par );
    if ( res != 1 ) return res;

    int coltabid;
    if ( !par.get( colortabstr(), coltabid ) ) return -1;
    DataObject* dataobj = DM().getObject( coltabid );
    if ( !dataobj ) return 0;
    mDynamicCastGet(VisColorTab*,coltab,dataobj)
    if ( !coltab ) return -1;
    setColorTab( *coltab );

    int ctmid = -1;
    if ( par.get( coltabmodstr(), ctmid ) )
    {
	dataobj = DM().getObject( ctmid );
	if ( !dataobj ) return 0;
	mDynamicCastGet(VisColTabMod*,ctm,dataobj)
	if ( !ctm ) return -1;
	coltabmod->unRef();
	coltabmod = ctm;
	coltabmod->ref();
    }

    int newres = 0;
    par.get( resolutionstr(), newres );
    setResolution( newres );

    float texturequality = 1;
    par.get( texturequalitystr(), texturequality );
    setTextureQuality( texturequality );

    bool usetext = true;
    par.getYN( usestexturestr(), usetext );
    turnOn( usetext );

    return 1;
}

}; // namespace visBase
