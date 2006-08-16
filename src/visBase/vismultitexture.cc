/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2005
___________________________________________________________________

-*/

static const char* rcsID = "$Id: vismultitexture.cc,v 1.20 2006-08-16 10:51:20 cvsbert Exp $";

#include "vismultitexture2.h"

#include "basictask.h"
#include "errh.h"
#include "thread.h"
#include "viscolortab.h"


#define mNrColors	255
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

    const unsigned int*		getHistogram() const { return globalhistogram_;}

protected:
    bool			doWork(int start,int stop,int threadid);
    int				nrTimes() const { return sz_; }

    unsigned char*		indexcache_;
    const float*		datacache_;
    const visBase::VisColorTab*	colortab_;
    unsigned int		globalhistogram_[mNrColors];
    Threads::Mutex		histogrammutex_;
    const int			sz_;
};


TextureColorIndexer::TextureColorIndexer( const float* inp,
					  unsigned char* outp, int nsz,
					  const VisColorTab* ct )
    : colortab_( ct )
    , indexcache_( outp )
    , sz_( nsz )
    , datacache_( inp )
{
    histogrammutex_.lock();
    memset( globalhistogram_, 0, sizeof(int)*mNrColors );
    histogrammutex_.unlock();
}


bool TextureColorIndexer::doWork( int start, int stop, int threadid )
{
    unsigned int histogram[mNrColors];
    memset( histogram, 0, sizeof(int)*mNrColors );

    for ( int idx=start; idx<=stop; idx++ )
    {
	const int colorindex = colortab_->colIndex(datacache_[idx]);
	indexcache_[idx] = colorindex;
	histogram[colorindex]++;
    }

    histogrammutex_.lock();
    for ( int idx=mNrColors-1; idx>=0; idx-- )
	globalhistogram_[idx] += histogram[idx];
    histogrammutex_.unlock();

    return true;
}


class TextureInfo : public CallBacker
{
public:
    				TextureInfo( MultiTexture*, const char* name );
    				~TextureInfo();
    void			enable( bool yn );
    bool			isEnabled() const { return enabled_; }
    int				nrVersions() const;
    void			setNrVersions(int);

    bool			setTextureData(int version,const float*,
	    				       int sz,bool man);
    bool			setTextureData(int version,const unsigned char*,
					       int sz,bool man);

    void			setColorTab( VisColorTab& ct);
    VisColorTab&		getColorTab();

    void			clipData(int version);
    void			createIndexes(int version);
    int				getCurrentVersion() const;
    void			setCurrentVersion( int );
    const unsigned char*	getCurrentData() const;

    const TypeSet<float>*	getHistogram(int version) const;

    bool			enabled_;
    char			components_;

protected:
    void			setColorTab(int,VisColorTab& ct);
    void			rangeChangeCB(CallBacker*);
    void			sequenceChangeCB(CallBacker*);
    void			autoscaleChangeCB(CallBacker*);

    ObjectSet<const unsigned char> versionindexdata_;
    BoolTypeSet			ownsindexdata_;
    ObjectSet<const float>	versionfloatdata_;
    BoolTypeSet			ownsfloatdata_;
    ObjectSet<VisColorTab>	versioncoltab_;
    ObjectSet<TypeSet<float> >	versionhistogram_;
    int				currentversion_;
    BufferString		name_;
    int				sz_;
    MultiTexture*		texture_;
};


TextureInfo::TextureInfo( MultiTexture* nt, const char* nm )
    : currentversion_( 0 )
    , name_( nm )
    , components_( (MultiTexture::RED | MultiTexture::GREEN |
		    MultiTexture::BLUE | MultiTexture::OPACITY ) )
    , sz_( 0 )
    , texture_( nt )
    , enabled_( true )
{
    versionindexdata_.allowNull(true);
    versionfloatdata_.allowNull(true);
    versionhistogram_.allowNull(true);
    versioncoltab_.allowNull(true);
    setNrVersions(1);
}


TextureInfo::~TextureInfo()
{

    for ( int idx=0; idx<versionfloatdata_.size(); idx++ )
	if ( ownsfloatdata_[idx] ) delete [] versionfloatdata_[idx];

    for ( int idx=0; idx<versionindexdata_.size(); idx++ )
	if ( ownsindexdata_[idx] ) delete [] versionindexdata_[idx];

    for ( int idx=0; idx<versioncoltab_.size(); idx++ )
    {
	if ( versioncoltab_[idx] )
	{
	    versioncoltab_[idx]->rangechange.remove(
			mCB(this,TextureInfo,rangeChangeCB) );
	    versioncoltab_[idx]->sequencechange.remove(
			mCB(this,TextureInfo,sequenceChangeCB) );
	    versioncoltab_[idx]->autoscalechange.remove(
			mCB(this,TextureInfo,autoscaleChangeCB) );
	    versioncoltab_[idx]->unRef();
	}
    }

    versioncoltab_.erase();
    deepErase( versionhistogram_ );
}


void TextureInfo::enable( bool yn )
{
    if ( yn==enabled_ )
	return;

    enabled_=yn;
    texture_->updateColorTables();
}


void TextureInfo::setNrVersions( int nsz )
{
    if ( !nsz )
    {
	pErrMsg( "Must leave one version" );
	return;
    }

    while ( nsz<versionindexdata_.size() )
    {
	if ( ownsindexdata_[nsz] ) delete [] versionindexdata_[nsz];
	if ( ownsfloatdata_[nsz] ) delete [] versionfloatdata_[nsz];
	delete versionhistogram_[nsz];
	versioncoltab_[nsz]->unRef();

	versionindexdata_.remove(nsz);
	versionfloatdata_.remove(nsz);
	ownsfloatdata_.remove(nsz);
	ownsindexdata_.remove(nsz);
	versionhistogram_.remove(nsz);
	versioncoltab_.remove(nsz);
    }

    while ( versionindexdata_.size()<nsz )
    {
	versionindexdata_ += 0;
	versionfloatdata_ += 0;
	ownsfloatdata_ += false;
	ownsindexdata_ += false;
	versionhistogram_ += 0;
	versioncoltab_ += 0;
	setColorTab( versioncoltab_.size()-1, *VisColorTab::create() );
    }

    if ( currentversion_>=nsz )
	setCurrentVersion( nsz-1 );
}


int TextureInfo::nrVersions() const
{ return versionindexdata_.size(); }


bool TextureInfo::setTextureData( int version, const float* data, int newsz,
				  bool managedata )
{
    if ( version<0 || version>=nrVersions() || ( version && sz_!=newsz ) )
    {
	if ( managedata ) delete [] data;
	return false;
    }

    if ( newsz!=sz_ )
    {
	if ( ownsindexdata_[version] )
	    delete [] versionindexdata_[version];

	versionindexdata_.replace( version, 0 );
    }

    sz_ = newsz;

    if ( ownsfloatdata_[version] )
	delete [] versionfloatdata_[version];

    versionfloatdata_.replace( version, data );
    ownsfloatdata_[version] = managedata;

    if ( versionfloatdata_[version] )
    {
	if ( versioncoltab_[version]->autoScale() )
	    clipData(version);
	else
	    createIndexes( version );
    }
    else
	texture_->textureChange( this );

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

    if ( ownsfloatdata_[version] )
	delete [] versionfloatdata_[version];

    versionfloatdata_.replace( version, 0 );
    ownsfloatdata_[version] = false;

    if ( ownsindexdata_[version] )
	delete [] versionindexdata_[version];

    versionindexdata_.replace( version, data );
    ownsindexdata_[version] = managedata;

    //TODO: Trigger some kind of update.

    return true;
}

void TextureInfo::setColorTab( VisColorTab& ct )
{
    setColorTab( 0, ct );
    ColorSequence& seq = ct.colorSeq();

    for ( int idx=versioncoltab_.size()-1; idx>=0; idx-- )
    {
	if ( !idx ) continue;
	versioncoltab_[idx]->setColorSeq( &seq );
    }
}


void TextureInfo::setColorTab( int version, VisColorTab& ct )
{
    if ( versioncoltab_[version] )
    {
	versioncoltab_[version]->rangechange.remove(
				mCB(this,TextureInfo,rangeChangeCB ) );
	versioncoltab_[version]->sequencechange.remove(
				mCB(this,TextureInfo,sequenceChangeCB ) );
	versioncoltab_[version]->autoscalechange.remove(
				mCB(this,TextureInfo,autoscaleChangeCB) );
	versioncoltab_[version]->unRef();
    }

    versioncoltab_.replace( version, &ct );
    ct.ref();
    ct.rangechange.notify( mCB(this,TextureInfo,rangeChangeCB) );
    ct.sequencechange.notify( mCB(this,TextureInfo,sequenceChangeCB) );
    ct.autoscalechange.notify( mCB(this,TextureInfo,autoscaleChangeCB) );

    if ( version )
	versioncoltab_[version]->setColorSeq( &versioncoltab_[0]->colorSeq() );
}


VisColorTab& TextureInfo::getColorTab()
{ return *versioncoltab_[currentversion_]; }


void TextureInfo::clipData( int version )
{
    if ( versionfloatdata_[version] )
	versioncoltab_[version]->scaleTo( versionfloatdata_[version], sz_ );
}


void TextureInfo::createIndexes( int version )
{
    if ( !versionfloatdata_[version] )
	return;

    if ( !versionindexdata_[version] )
    {
	versionindexdata_.replace( version, new unsigned char[sz_] );
	ownsindexdata_[version] = true;
    }

    versioncoltab_[version]->setNrSteps(mNrColors);

    TextureColorIndexer indexer( versionfloatdata_[version],
	    			 (unsigned char*) versionindexdata_[version],
				 sz_, versioncoltab_[version] );
    if ( !indexer.execute() )
    {
	delete [] versionindexdata_[version];
	versionindexdata_.replace( version, 0 );
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
	if ( !versionhistogram_[version] )
	    versionhistogram_.replace( version,
		    		       new TypeSet<float>(mNrColors,0) );
	for ( int idx=mNrColors-1; idx>=0; idx-- )
	    (*versionhistogram_[version])[idx] = (float) histogram[idx]/max;
    }

    texture_->textureChange( this );
}


int TextureInfo::getCurrentVersion() const { return currentversion_; }


void TextureInfo::setCurrentVersion( int nidx )
{
    if ( nidx<0 || nidx>=versionindexdata_.size() )
    {
	pErrMsg("Invalid index");
	return;
    }

    currentversion_ = nidx;

    texture_->textureChange( this );
}


const unsigned char* TextureInfo::getCurrentData() const
{ return versionindexdata_[currentversion_]; }


const TypeSet<float>* TextureInfo::getHistogram( int version ) const
{
    if ( version<0 || version>=nrVersions() )
	return 0;

    return versionhistogram_[version];
}


void TextureInfo::rangeChangeCB( CallBacker* cb )
{
    const int version = versioncoltab_.indexOf( (visBase::VisColorTab*)cb );
    createIndexes( version );
}


void TextureInfo::sequenceChangeCB( CallBacker* )
{
    texture_->updateColorTables();
}


void TextureInfo::autoscaleChangeCB( CallBacker* cb )
{
    const int version = versioncoltab_.indexOf( (visBase::VisColorTab*)cb );
    clipData( version );
}


MultiTexture::MultiTexture()
{
    addTexture( "Default" );
}


MultiTexture::~MultiTexture()
{ deepErase( textureinfo_ ); }


int MultiTexture::nrTextures() const
{ return textureinfo_.size(); }


int MultiTexture::addTexture( const char* name )
{
    TextureInfo* newtexture = new TextureInfo( this, name );

    const int res = textureinfo_.size();

    textureinfo_ += newtexture;
    updateSoTextureInternal(res);
    return res;
};


void MultiTexture::enableTexture( int idx, bool yn )
{
    if ( idx<0 || idx>=textureinfo_.size() )
	return;

    textureinfo_[idx]->enable( yn );
}


bool MultiTexture::isTextureEnabled( int idx ) const
{
    if ( idx<0 || idx>=textureinfo_.size() )
	return false;

    return textureinfo_[idx]->isEnabled();
}



void MultiTexture::swapTextures( int t0, int t1 )
{
    if ( t0<0 || t1<0 || t0>=textureinfo_.size() || t1>=textureinfo_.size() )
	return;

    textureinfo_.swap( t0, t1 );

    updateSoTextureInternal( t0 );
    updateSoTextureInternal( t1 );
}
    

int MultiTexture::insertTexture( int idx, const char* name )
{
    if ( idx>=textureinfo_.size() )
	return addTexture( name );

    if ( idx<0 )
    {
	pErrMsg("Negative index");
	idx=0;
    }

    TextureInfo* newtexture = new TextureInfo( this, name );
    textureinfo_.insertAt( newtexture, idx );
    insertTextureInternal(idx);
    return idx;
}


void MultiTexture::removeTexture( int idx )
{
    if ( idx<0 || idx>=textureinfo_.size() )
	return;

    delete textureinfo_[idx];
    textureinfo_.remove(idx);
    removeTextureInternal( idx );
}


void MultiTexture::setComponents( int idx, char bits )
{
    if ( idx<0 || idx>=textureinfo_.size() )
	return;

    textureinfo_[idx]->components_ = bits;
}


char MultiTexture::getComponents( int idx ) const
{
    return idx<0 || idx>=textureinfo_.size()
	? RED | GREEN | BLUE | OPACITY
	: textureinfo_[idx]->components_;
}


void MultiTexture::setColorTab( int idx, VisColorTab& ct )
{
    ct.ref();
    if ( idx>=0 && idx<textureinfo_.size() )
	textureinfo_[idx]->setColorTab( ct );

    ct.unRef();
}


VisColorTab& MultiTexture::getColorTab( int idx )
{
    if ( idx<0 || idx>=textureinfo_.size() )
	pErrMsg("Index out of bounds");

    return textureinfo_[idx]->getColorTab();
}


int MultiTexture::nrVersions( int idx ) const 
{ return textureinfo_[idx]->nrVersions(); }


void MultiTexture::setNrVersions( int idx, int newsz )
{
    if ( idx<0 || idx>=textureinfo_.size() )
	return;

    textureinfo_[idx]->setNrVersions( newsz );
}


int MultiTexture::currentVersion( int idx ) const
{
    if ( idx<0 || idx>=textureinfo_.size() )
	idx = 0;

    return textureinfo_[idx]->getCurrentVersion();
}


void MultiTexture::setCurrentVersion( int idx, int version )
{
    if ( idx<0 || idx>=textureinfo_.size() )
	idx = 0;

    textureinfo_[idx]->setCurrentVersion( version );
}


const TypeSet<float>* MultiTexture::getHistogram( int texture, int version ) const
{
    if ( texture<0 || texture>=textureinfo_.size() )
	return 0;

    return textureinfo_[texture]->getHistogram( version );
}


#define mErrRet \
{ \
    if ( managedata ) delete [] data; \
    return false; \
}

bool MultiTexture::setTextureData( int texture, int version, const float* data,
				   int sz, bool managedata )
{
    if ( texture<0 || texture>=textureinfo_.size() )
    {
	if ( managedata ) delete [] data;
	return false;
    }

    return textureinfo_[texture]->setTextureData( version, data, sz, managedata);
}


bool MultiTexture::setTextureIndexData( int texture, int version,
					const unsigned char* data, int sz,
					bool managedata )
{
    if ( texture<0 || texture>=textureinfo_.size() )
    {
	if ( managedata ) delete [] data;
	return false;
    }

    return textureinfo_[texture]->setTextureData( version, data, sz, managedata);
}


const unsigned char*
MultiTexture::getCurrentTextureIndexData( int texture ) const
{ 
    if ( texture<0 || texture>=textureinfo_.size() )
	return 0;

    return textureinfo_[texture]->getCurrentData();
}


void MultiTexture::textureChange( TextureInfo* ti )
{
    const int texture = textureinfo_.indexOf( ti );
    if ( ti<0 )
    {
	pErrMsg("Hugh");
	return;
    }

    updateSoTextureInternal( texture );
}



}; // namespace visBase
