/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2005
___________________________________________________________________

-*/

static const char* rcsID = "$Id: vismultitexture.cc,v 1.3 2006-01-18 22:55:33 cvskris Exp $";

#include "vismultitexture2.h"

#include "basictask.h"
#include "errh.h"
#include "thread.h"
#include "viscolortab.h"


#define mNrColors	256
#define mUndefColIdx	255

namespace visBase
{



class TextureColorIndexer : public ParallelTask
{
public:
    				TextureColorIndexer( const float* inp,
	    					     unsigned char* outp,
						     int sz,
						     const VisColorTab* );

    const unsigned int*		getHistogram() const { return globalhistogram; }

protected:
    bool			doWork( int start, int stop );
    int				nrTimes() const { return sz; }

    unsigned char*		indexcache;
    const float*		datacache;
    const visBase::VisColorTab*	colortab;
    unsigned int		globalhistogram[mNrColors];
    Threads::Mutex		histogrammutex;
    const int			sz;
};


TextureColorIndexer::TextureColorIndexer( const float* inp,
					  unsigned char* outp, int nsz,
					  const VisColorTab* ct )
    : colortab( ct )
    , indexcache( outp )
    , sz( nsz )
    , datacache( inp )
{
    histogrammutex.lock();
    memset( globalhistogram, 0, sizeof(int)*mNrColors );
    histogrammutex.unlock();
}


bool TextureColorIndexer::doWork( int start, int stop )
{
    unsigned int histogram[mNrColors];
    memset( histogram, 0, sizeof(int)*mNrColors );

    for ( int idx=start; idx<=stop; idx++ )
    {
	const int colorindex = colortab->colIndex(datacache[idx]);
	indexcache[idx] = colorindex;
	histogram[colorindex]++;
    }

    histogrammutex.lock();
    for ( int idx=mNrColors-1; idx>=0; idx-- )
	globalhistogram[idx] += histogram[idx];
    histogrammutex.unlock();

    return true;
}


class TextureInfo : public CallBackClass
{
public:
    			TextureInfo( MultiTexture*, const char* name );
    			~TextureInfo();
    int			nrVersions() const;
    void		setNrVersions(int);

    bool		setTextureData( int version, const float*, int sz,
	    				bool man );
    bool		setTextureData( int version, const unsigned char*,
	    				int sz, bool man );

    void		setColorTab( VisColorTab& ct );
    VisColorTab&	getColorTab();

    void		clipData( int version );
    void		createIndexes( int version );
    int			getCurrentVersion() const;
    void		setCurrentVersion( int );
    const unsigned char* getCurrentData() const;

    const TypeSet<float>* getHistogram( int version );

    MultiTexture::Operation	operation;
    char			components;

protected:
    void			setColorTab( int, VisColorTab& ct );
    void			rangeChangeCB( CallBacker* );
    void			sequenceChangeCB( CallBacker* );
    void			clipStatusChangeCB( CallBacker* );
    ObjectSet<const unsigned char> versionindexdata;
    BoolTypeSet			ownsindexdata;
    ObjectSet<const float>	versionfloatdata;
    BoolTypeSet			ownsfloatdata;
    ObjectSet<VisColorTab>	versioncoltab;
    ObjectSet<TypeSet<float> >	versionhistogram;
    int				currentversion;
    BufferString		name;
    int				sz;
    MultiTexture*		texture;
};


TextureInfo::TextureInfo( MultiTexture* nt, const char* nm )
    : currentversion( 0 )
    , operation( MultiTexture::BLEND )
    , components( (MultiTexture::RED | MultiTexture::GREEN |
		   MultiTexture::BLUE | MultiTexture::OPACITY ) )
    , name( nm )
    , sz( 0 )
    , texture( nt )
{
    versionindexdata.allowNull(true);
    versionfloatdata.allowNull(true);
    versionhistogram.allowNull(true);
    versioncoltab.allowNull(true);
    setNrVersions(1);
}


TextureInfo::~TextureInfo()
{

    for ( int idx=0; idx<versionfloatdata.size(); idx++ )
	if ( ownsfloatdata[idx] ) delete [] versionfloatdata[idx];

    for ( int idx=0; idx<versionindexdata.size(); idx++ )
	if ( ownsindexdata[idx] ) delete [] versionindexdata[idx];

    deepUnRef( versioncoltab );
}

void TextureInfo::setNrVersions( int nsz )
{
    if ( !nsz )
    {
	pErrMsg( "Must leave one version" );
	return;
    }

    while ( nsz<versionindexdata.size() )
    {
	if ( ownsindexdata[nsz] ) delete [] versionindexdata[nsz];
	if ( ownsfloatdata[nsz] ) delete [] versionfloatdata[nsz];
	delete versionhistogram[nsz];
	versioncoltab[nsz]->unRef();

	versionindexdata.remove(nsz);
	versionfloatdata.remove(nsz);
	ownsfloatdata.remove(nsz);
	ownsindexdata.remove(nsz);
	versionhistogram.remove(nsz);
	versioncoltab.remove(nsz);
    }

    while ( versionindexdata.size()<nsz )
    {
	versionindexdata += 0;
	versionfloatdata += 0;
	ownsfloatdata += false;
	ownsindexdata += false;
	versionhistogram += 0;
	versioncoltab += 0;
	setColorTab( versioncoltab.size()-1, *VisColorTab::create() );
    }

    if ( currentversion>=nsz )
	setCurrentVersion( nsz-1 );
}


int TextureInfo::nrVersions() const
{ return versionindexdata.size(); }


bool TextureInfo::setTextureData( int version, const float* data, int newsz,
				  bool managedata )
{
    if ( version<0 || version>=nrVersions() || ( version && sz!=newsz ) )
    {
	if ( managedata ) delete [] data;
	return false;
    }

    sz = newsz;

    if ( ownsfloatdata[version] )
	delete [] versionfloatdata[version];

    versionfloatdata.replace( version, data );
    ownsfloatdata[version] = managedata;

    if ( versioncoltab[version]->autoScale() )
	clipData(version);
    else
	createIndexes( version );

    return true;
}


bool TextureInfo::setTextureData( int version, const unsigned char* data,
				  int sz, bool managedata )
{
    if ( version<0 || version>=nrVersions() )
    {
	if ( managedata ) delete [] data;
	return false;
    }

    if ( ownsfloatdata[version] )
	delete [] versionfloatdata[version];

    versionfloatdata.replace( version, 0 );
    ownsfloatdata[version] = false;

    if ( ownsindexdata[version] )
	delete [] versionindexdata[version];

    versionindexdata.replace( version, data );
    ownsindexdata[version] = managedata;

    //TODO: Trigger some kind of update.

    return true;
}

void TextureInfo::setColorTab( VisColorTab& ct )
{
    versioncoltab[currentversion]->unRef();
    versioncoltab.replace( currentversion, &ct );
    ct.ref();

    ColorSequence& seq = ct.colorSeq();

    for ( int idx=versioncoltab.size()-1; idx>=0; idx-- )
    {
	if ( idx==currentversion )
	    continue;

	versioncoltab[idx]->setColorSeq( &seq );
    }
}


void TextureInfo::setColorTab( int version, VisColorTab& ct )
{
    if ( versioncoltab[version] )
    {
	versioncoltab[version]->rangechange.remove(
		mCB( this, TextureInfo, rangeChangeCB ) );
	versioncoltab[version]->sequencechange.remove(
		mCB( this, TextureInfo, sequenceChangeCB ) );
	versioncoltab[version]->unRef();
    }

    versioncoltab.replace( version, &ct );
    ct.ref();
    ct.rangechange.notify( mCB( this, TextureInfo, rangeChangeCB ) );
    ct.sequencechange.notify( mCB( this, TextureInfo, sequenceChangeCB ) );

    if ( version )
	versioncoltab[version]->setColorSeq( &versioncoltab[0]->colorSeq() );
}


VisColorTab& TextureInfo::getColorTab()
{ return *versioncoltab[currentversion]; }


void TextureInfo::clipData( int version )
{
    if ( versionfloatdata[version] )
	versioncoltab[version]->scaleTo( versionfloatdata[version], sz );
}


void TextureInfo::createIndexes( int version )
{
    if ( !versionfloatdata[version] )
	return;

    if ( !versionindexdata[version] )
    {
	versionindexdata.replace( version, new unsigned char[sz] );
	ownsindexdata[version] = true;
    }

    versioncoltab[version]->setNrSteps(mNrColors-1);

    TextureColorIndexer indexer( versionfloatdata[version],
	    			 (unsigned char*) versionindexdata[version],
				 sz, versioncoltab[version] );
    if ( !indexer.execute() )
    {
	delete [] versionindexdata[version];
	versionindexdata.replace( version, 0 );
	return;
    }

    int max = 0;
    const unsigned int* histogram = indexer.getHistogram();
    for ( int idx=mNrColors-1; idx>=0; idx-- )
    {
	if ( histogram[idx]>max && idx!=mUndefColIdx )
	    max = histogram[idx];
    }

    if ( max )
    {
	if ( !versionhistogram[version] )
	    versionhistogram.replace( version,new TypeSet<float>(mNrColors,0) );
	for ( int idx=mNrColors-1; idx>=0; idx-- )
	    (*versionhistogram[version])[idx] = (float) histogram[idx]/max;
    }

    texture->textureChange( this );
}


int TextureInfo::getCurrentVersion() const { return currentversion; }


void TextureInfo::setCurrentVersion( int nidx )
{
    if ( nidx<0 || nidx>=versionindexdata.size() )
    {
	pErrMsg("Invalid index");
	return;
    }

    currentversion = nidx;

    //TODO Trigger something
}


const unsigned char* TextureInfo::getCurrentData() const
{ return versionindexdata[currentversion]; }


const TypeSet<float>* TextureInfo::getHistogram( int version )
{
    if ( version<0 || version>=nrVersions() )
	return 0;

    return versionhistogram[version];
}


void TextureInfo::rangeChangeCB( CallBacker* cb )
{
    const int version = versioncoltab.indexOf((visBase::VisColorTab*) cb);
    createIndexes( version );
}


void TextureInfo::sequenceChangeCB( CallBacker* )
{
    texture->updateColorTables();
}



//const char* Texture::colortabstr = "ColorTable ID";
//const char* Texture::usestexturestr = "Uses texture";
//const char* Texture::texturequalitystr = "Texture quality";
//const char* Texture::resolutionstr = "Resolution";
//const char* Texture::coltabmodstr = "ColorTableModifier ID";
//


MultiTexture::MultiTexture()
{
    addTexture( "Default" );
}


int MultiTexture::nrTextures() const
{ return textureinfo.size(); }


int MultiTexture::addTexture( const char* name )
{
    TextureInfo* newtexture = new TextureInfo( this, name );

    const int res = textureinfo.size();

    textureinfo += newtexture;
    updateSoTextureInternal(res);
    return res;
};


void MultiTexture::swapTextures( int t0, int t1 )
{
    if ( t0<0 || t1<0 || t0>=textureinfo.size() || t1>=textureinfo.size() )
	return;

    textureinfo.swap( t0, t1 );

    updateSoTextureInternal( t0 );
    updateSoTextureInternal( t1 );
}
    

int MultiTexture::insertTexture( int idx, const char* name )
{
    if ( idx>=textureinfo.size() )
	return addTexture( name );

    if ( idx<0 )
    {
	pErrMsg("Negative index");
	idx=0;
    }

    TextureInfo* newtexture = new TextureInfo( this, name );
    textureinfo.insertAt( newtexture, idx );
    insertTextureInternal(idx);
    return idx;
}


void MultiTexture::removeTexture( int idx )
{
    if ( idx<0 || idx>=textureinfo.size() )
	return;

    delete textureinfo[idx];
    textureinfo.remove(idx);
    removeTextureInternal( idx );
}


void MultiTexture::setOperation( int idx, Operation op )
{
    if ( idx<0 || idx>=textureinfo.size() )
	return;

    textureinfo[idx]->operation = op;
}


void MultiTexture::setComponents( int idx, char bits )
{
    if ( idx<0 || idx>=textureinfo.size() )
	return;

    textureinfo[idx]->components = bits;
}


void MultiTexture::setColorTab( int idx, VisColorTab& ct )
{
    ct.ref();
    if ( idx<=0 && idx<textureinfo.size() )
	textureinfo[idx]->setColorTab( ct );

    ct.unRef();
}


VisColorTab& MultiTexture::getColorTab( int idx )
{
    if ( idx<0 || idx>=textureinfo.size() )
	pErrMsg("Index out of bounds");

    return textureinfo[idx]->getColorTab();
}


int MultiTexture::nrVersions( int idx ) const 
{ return textureinfo[idx]->nrVersions(); }


void MultiTexture::setNrVersions( int idx, int newsz )
{
    if ( idx<0 || idx>=textureinfo.size() )
	return;

    textureinfo[idx]->setNrVersions( newsz );
}


int MultiTexture::currentVersion( int idx ) const
{
    if ( idx<0 || idx>=textureinfo.size() )
	idx = 0;

    return textureinfo[idx]->getCurrentVersion();
}


void MultiTexture::setCurrentVersion( int idx, int version )
{
    if ( idx<0 || idx>=textureinfo.size() )
	idx = 0;

    textureinfo[idx]->setCurrentVersion( version );
}


const TypeSet<float>* MultiTexture::getHistogram( int texture, int version ) const
{
    if ( texture<0 || texture>=textureinfo.size() )
	return 0;

    return textureinfo[texture]->getHistogram( version );
}


#define mErrRet \
{ \
    if ( managedata ) delete [] data; \
    return false; \
}

bool MultiTexture::setTextureData( int texture, int version, const float* data,
				   int sz, bool managedata )
{
    if ( texture<0 || texture>=textureinfo.size() )
    {
	if ( managedata ) delete [] data;
	return false;
    }

    return textureinfo[texture]->setTextureData( version, data, sz, managedata);
}


bool MultiTexture::setTextureIndexData( int texture, int version,
					const unsigned char* data, int sz,
					bool managedata )
{
    if ( texture<0 || texture>=textureinfo.size() )
    {
	if ( managedata ) delete [] data;
	return false;
    }

    return textureinfo[texture]->setTextureData( version, data, sz, managedata);
}


const unsigned char*
MultiTexture::getCurrentTextureIndexData( int texture ) const
{ 
    if ( texture<0 || texture>=textureinfo.size() )
	return 0;

    return textureinfo[texture]->getCurrentData();
}


void MultiTexture::textureChange( TextureInfo* ti )
{
    const int texture = textureinfo.indexOf( ti );
    if ( ti<0 )
    {
	pErrMsg("Hugh");
	return;
    }

    updateSoTextureInternal( texture );
}



}; // namespace visBase
