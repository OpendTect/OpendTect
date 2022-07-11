/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jun 2008
___________________________________________________________________

-*/


#include "vistexturechannels.h"

#include "coord.h"
#include "coltabmapper.h"
#include "mousecursor.h"
#include "settingsaccess.h"
#include "valseriesimpl.h"
#include "visosg.h"
#include "vistexturechannel2rgba.h"

#include <osgGeo/LayeredTexture>
#include <osg/Image>

#define mNrColors	255




mCreateFactoryEntry( visBase::TextureChannels );


namespace visBase
{


class ChannelInfo : public CallBacker
{
public:
				ChannelInfo(TextureChannels&);
				~ChannelInfo();

    void			setSize(int,int,int);
    int				getSize(unsigned char dim) const;

    void			setOrigin(const Coord& origin);
    const Coord&		getOrigin() const;
    void			setScale(const Coord& scale);
    const Coord&		getScale() const;

    od_int64			nrElements(bool percomponent) const;

    void			setOsgIDs(const TypeSet<int>& );
    const TypeSet<int>&		getOsgIDs() const { return osgids_; }
    int				nrComponents() const { return osgids_.size(); }

    void			setColTabMapperSetup(
						const ColTab::MapperSetup&);
    const ColTab::MapperSetup&	getColTabMapperSetup(int version) const;
    const ColTab::Mapper&	getColTabMapper(int version) const;
    bool			reMapData(bool dontreclip,TaskRunner*);
    const TypeSet<float>&	getHistogram() const	{ return histogram_; }

    void			setNrVersions(int);
    int				nrVersions() const;

    void			removeCaches();
    void			removeImages();

    bool			setUnMappedData(int version,
					        const ValueSeries<float>*,
						OD::PtrPolicy, TaskRunner*,
						bool skipclip);
    bool			setMappedData(int version,unsigned char*,
					      OD::PtrPolicy);

    void			clipData(int version,TaskRunner*);
				//!<If version==-1, all versions will be clipped
    bool			mapData(int version,TaskRunner*);
    int				getCurrentVersion() const;
    void			setCurrentVersion( int );

    void			updateOsgImages();

    bool			isCurrentDataPremapped() const;

    ObjectSet<unsigned char>			mappeddata_;
    BoolTypeSet					ownsmappeddata_;
    ObjectSet<const ValueSeries<float> >	unmappeddata_;
    BoolTypeSet					ownsunmappeddata_;
    ObjectSet<ColTab::Mapper>			mappers_;
    int						currentversion_ = 0;
    TextureChannels&				texturechannels_;
    TypeSet<float>				histogram_;
    int						size_[3];
    Coord					origin_;
    Coord					scale_;

    ObjectSet<osg::Image>			osgimages_;
    TypeSet<int>				osgids_;
};


ChannelInfo::ChannelInfo( TextureChannels& nc )
    : texturechannels_(nc)
    , histogram_( mNrColors, 0 )
    , origin_( 0.0, 0.0 )
    , scale_( 1.0, 1.0 )
{
    size_[0] = 0;
    size_[1] = 0;
    size_[2] = 0;

    osgimages_.setNullAllowed();
    mappeddata_.setNullAllowed();
    unmappeddata_.setNullAllowed();
    setNrVersions( 1 );
}


ChannelInfo::~ChannelInfo()
{
    setNrVersions( 0 );
}


void ChannelInfo::setSize( int s0, int s1, int s2 )
{
    if ( size_[0]==s0 && size_[1]==s1 && size_[2]==s2 )
	return;

    size_[0]=s0;
    size_[1]=s1;
    size_[2]=s2;

    removeCaches();
}


int ChannelInfo::getSize( unsigned char dim ) const
{ return size_[dim]; }


void ChannelInfo::setOrigin( const Coord& origin )
{ origin_ = origin; }


const Coord& ChannelInfo::getOrigin() const
{ return origin_; }


void ChannelInfo::setScale( const Coord& scale )
{ scale_ = scale; }


const Coord& ChannelInfo::getScale() const
{ return scale_; }


od_int64 ChannelInfo::nrElements( bool percomponent ) const
{
    od_int64 nrelements = size_[0];
    nrelements *= size_[1];
    nrelements *= size_[2];
    if ( !percomponent )
	nrelements *= nrComponents();

    return nrelements;
}


const ColTab::Mapper& ChannelInfo::getColTabMapper( int version ) const
{ return *mappers_[version]; }


void ChannelInfo::setColTabMapperSetup( const ColTab::MapperSetup& setup )
{
    if ( !mappers_.size() )
	{ pErrMsg("No mappers"); return; }

    if ( mappers_[0]->setup_==setup )
	return;

    for ( int idx=0; idx<mappers_.size(); idx++ )
    {
	const bool autoscalechange = mappers_[idx]->setup_.type_ != setup.type_;
	mappers_[idx]->setup_ = setup;
	if ( autoscalechange )
	    mappers_[idx]->setup_.triggerAutoscaleChange();
	else
	    mappers_[idx]->setup_.triggerRangeChange();
    }

}


const ColTab::MapperSetup& ChannelInfo::getColTabMapperSetup(int channel) const
{
    if ( channel < mappers_.size() )
	return mappers_[channel]->setup_;

	pErrMsg( "channel >= mappers_.size()" );
	if ( mappers_.isEmpty() )
	{
	    mDefineStaticLocalObject( ColTab::MapperSetup, ctms, );
	    return ctms;
	}
	return mappers_[mappers_.size()-1]->setup_;
}


void ChannelInfo::clipData( int version, TaskRunner* tr )
{
    for ( int idx=0; idx<nrVersions(); idx++ )
    {
	if ( version!=-1 && idx!=version )
	    continue;

	if ( !unmappeddata_[idx] )
	    continue;

	mappers_[idx]->setData( *unmappeddata_[idx], tr );
	mappers_[idx]->setup_.triggerRangeChange();
    }
}


bool ChannelInfo::reMapData(bool dontreclip,TaskRunner* tr )
{
    for ( int idx=nrVersions()-1; idx>=0; idx-- )
    {
	if ( !dontreclip &&
	    mappers_[idx]->setup_.type_!=ColTab::MapperSetup::Fixed )
	    clipData( idx, tr );

	if ( !mapData( idx, tr ) )
	    return false;
    }

    return true;
}


void ChannelInfo::removeImages()
{
    for ( int idx=0; idx<osgimages_.size(); idx++ )
    {
	if ( osgimages_.get(idx) )
	    osgimages_.get(idx)->unref();
	osgimages_.replace( idx, nullptr );
    }
}


void ChannelInfo::removeCaches()
{
    ObjectSet<unsigned char> mappeddata = mappeddata_;
    ObjectSet<const ValueSeries<float> > unmappeddata = unmappeddata_;
    for ( int idx=0; idx<ownsmappeddata_.size(); idx++ )
    {
	mappeddata_.replace( idx, nullptr );
	unmappeddata_.replace( idx, nullptr );
    }

    removeImages();

    texturechannels_.update( this );

    for ( int idx=0; idx<ownsmappeddata_.size(); idx++ )
    {
	if ( ownsmappeddata_[idx] )
	    delete [] mappeddata[idx];
	if ( ownsunmappeddata_[idx] )
	    delete unmappeddata[idx];
    }
}


void ChannelInfo::setNrVersions( int nsz )
{
    if ( nsz && currentversion_>=nsz )
	setCurrentVersion( nsz-1 );

    while ( nsz<mappeddata_.size() )
    {
	if ( ownsmappeddata_[nsz] )
	    delete [] mappeddata_[nsz];
	if ( ownsunmappeddata_[nsz] )
	    delete unmappeddata_[nsz];

	mappeddata_.removeSingle( nsz );
	unmappeddata_.removeSingle( nsz );
	ownsmappeddata_.removeSingle( nsz );
	ownsunmappeddata_.removeSingle( nsz );
	delete mappers_.removeSingle( nsz );
    }

    const ColTab::MapperSetup* templ = mappers_.size()
	? &mappers_[0]->setup_
	: nullptr;

    while ( mappeddata_.size()<nsz )
    {
	mappeddata_ += nullptr;
	unmappeddata_ += nullptr;
	ownsmappeddata_ += false;
	ownsunmappeddata_ += false;

	auto* mapper = new ColTab::Mapper;
	if ( templ )
	    mapper->setup_ = *templ;
	mappers_ += mapper;
    }
}


int ChannelInfo::nrVersions() const
{ return ownsmappeddata_.size(); }


void ChannelInfo::setOsgIDs( const TypeSet<int>& osgids )
{
    osgids_ = osgids;
    const int nr = osgids.size();

    while ( nr>osgimages_.size() )
	osgimages_ += nullptr;

    while ( osgimages_.size()>nr )
    {
	osg::Image* image = osgimages_.removeSingle( nr );
	if ( image )
	    image->unref();
    }
}


bool ChannelInfo::setUnMappedData( int version, const ValueSeries<float>* data,
			  OD::PtrPolicy policy, TaskRunner* tr, bool skipclip )
{
    if ( version<0 || version>=nrVersions() )
    {
	if ( policy==OD::TakeOverPtr ) delete [] data;
	return false;
    }

    const od_int64 nrelements = nrElements( false );

    if ( unmappeddata_[version] )
    {
	if ( ownsunmappeddata_[version] )
	    delete unmappeddata_[version];

	unmappeddata_.replace( version, nullptr );
    }

    if ( policy==OD::UsePtr || policy==OD::TakeOverPtr )
    {
	unmappeddata_.replace( version, data );
	ownsunmappeddata_[version] = policy==OD::TakeOverPtr;
    }
    else if ( policy==OD::CopyPtr )
    {
	if ( data )
	{
	    ValueSeries<float>* newdata =
		new MultiArrayValueSeries<float,float>(nrelements);
	    if ( !newdata || !newdata->isOK() ) return false;
	    data->getValues( *newdata, nrelements );
	    unmappeddata_.replace( version, newdata );
	}
	else
	{
	    unmappeddata_.replace( version, nullptr );
	}

	ownsunmappeddata_[version] = true;
    }

    if ( !skipclip &&
	 mappers_[version]->setup_.type_!=ColTab::MapperSetup::Fixed )
	clipData( version, tr );

    return mapData( version, tr );
}


bool ChannelInfo::mapData( int version, TaskRunner* tr )
{
    if ( version<0 || version>=unmappeddata_.size() )
	return true;

    //Make sure old data is alive until it's replaced in Coin
    ArrPtrMan<unsigned char> oldptr;

    if ( mappeddata_[version] &&
	 (!ownsmappeddata_[version] || !unmappeddata_[version] ) )
    {
	if ( ownsmappeddata_[version] )
	    oldptr = mappeddata_[version];

	mappeddata_.replace( version, nullptr );
    }

    if ( !unmappeddata_[version] )
    {
	texturechannels_.update( this );
	removeImages();
	return true;
    }

    const od_int64 nrelements = nrElements( false );
    const unsigned char spacing = texturechannels_.nrTextureBands();

    if ( !mappeddata_[version] )
    {
	mDeclareAndTryAlloc(unsigned char*, mappeddata,
			    unsigned char[nrelements*spacing] );
	if ( !mappeddata ) return false;

	mappeddata_.replace( version, mappeddata );
	ownsmappeddata_[version] = true;
    }

    unsigned char* mappedudfs = nullptr;
    if ( texturechannels_.nrUdfBands() )
	mappedudfs = mappeddata_[version] + texturechannels_.nrDataBands();

    ColTab::MapperTask< unsigned char>	maptask( *mappers_[version], nrelements,
	    mNrColors, *unmappeddata_[version],
	    mappeddata_[version], spacing,
	    mappedudfs, spacing );

    if ( TaskRunner::execute( tr, maptask ) )
    {
	int max = 0;
	const unsigned int* histogram = maptask.getHistogram();
	for ( int idx=mNrColors-1; idx>=0; idx-- )
	{
	    if ( histogram[idx]>max )
		max = histogram[idx];
	}

	if ( max == 0 )
	    OD::memZero( histogram_.arr(), histogram_.size()*sizeof(float) );
	else
	{
	    for ( int idx=mNrColors-1; idx>=0; idx-- )
		histogram_[idx] = (float) histogram[idx]/max;
	}

	texturechannels_.update( this );
	return true;
    }

    return false;
}


bool ChannelInfo::setMappedData( int version, unsigned char* data,
				  OD::PtrPolicy policy )
{
    if ( mappeddata_.size()<version )
	{ pErrMsg("nrVersions not set" ); return false; }

    if ( mappeddata_[version] )
    {
	if (  ownsmappeddata_[version] )
	    delete mappeddata_[version];
	mappeddata_.replace( version, nullptr );
    }

    if ( policy==OD::UsePtr || policy==OD::TakeOverPtr )
    {
	mappeddata_.replace( version, data );
	ownsmappeddata_[version] = policy==OD::TakeOverPtr;
    }
    else if ( policy==OD::CopyPtr )
    {
	const od_int64 nrelements = nrElements( false );

	if ( data )
	{
	    mDeclareAndTryAlloc(unsigned char*, newdata,
				unsigned char[nrelements] );
	    if ( !newdata ) return false;
	    OD::memCopy( newdata, data, nrelements*sizeof(unsigned char) );
	    mappeddata_.replace( version, newdata );
	}
	else
	{
	    mappeddata_.replace( version, nullptr );
	}

	ownsmappeddata_[version] = true;
    }

    texturechannels_.update( this );
    return true;
}


int ChannelInfo::getCurrentVersion() const { return currentversion_; }


void ChannelInfo::setCurrentVersion( int nidx )
{
    if ( mappeddata_.size() && (nidx<0 || nidx>=mappeddata_.size() ) )
	{ pErrMsg("Invalid index"); return; }

    currentversion_ = nidx;
    texturechannels_.update( this );
}


void ChannelInfo::updateOsgImages()
{
    if ( !mappeddata_[currentversion_] )
    {
	removeImages();
	return;
    }

    const unsigned char nrbands = isCurrentDataPremapped() ? 1 :
					    texturechannels_.nrTextureBands();

    const GLenum imageformat = nrbands>=4 ? GL_RGBA :
			       nrbands==3 ? GL_RGB :
			       nrbands==2 ? GL_LUMINANCE_ALPHA : GL_LUMINANCE;

    const od_int64 componentsize = nrElements( true );
    for ( int idx=0; idx<osgimages_.size(); idx++ )
    {
	if ( !osgimages_[idx] )
	{
	    osg::Image* image = new osg::Image;
	    image->ref();
	    osgimages_.replace( idx, image );
	}

	const od_int64 offset = idx * componentsize * nrbands;

	osgimages_[idx]->setImage( size_[2], size_[1], size_[0],
			    imageformat, imageformat, GL_UNSIGNED_BYTE,
			    mappeddata_[currentversion_]+offset,
			    osg::Image::NO_DELETE, 1 );
    }
}


bool ChannelInfo::isCurrentDataPremapped() const
{
    if ( !unmappeddata_.validIdx(currentversion_) )
	return false;

    return !unmappeddata_[currentversion_];
}



class TextureChannels::TextureCallbackHandler :
					public osgGeo::LayeredTexture::Callback
{
public:
    TextureCallbackHandler()						{}
    virtual void startWorkInProgress() const
			{ MouseCursorManager::setOverride(MouseCursor::Wait); }
    virtual void stopWorkInProgress() const
			{ MouseCursorManager::restoreOverride(); }
};


#define mGetFilterType (interpolatetexture_ ? osgGeo::Linear : osgGeo::Nearest)

TextureChannels::TextureChannels()
    : osgtexture_(new osgGeo::LayeredTexture)
    , texturecallbackhandler_(new TextureCallbackHandler())
{
    turnOn( true );

    osgtexture_->invertUndefLayers();
    osgtexture_->setDataLayerFilterType( osgtexture_->compositeLayerId(),
					 mGetFilterType );
    osgtexture_->setSeamPower( 1 );

    bool mipmapping = true;
    Settings::common().getYN( SettingsAccess::sKeyEnableMipmapping(),
			      mipmapping );
    osgtexture_->enableMipmapping( mipmapping );

    int anisotropicpower = 4;
    Settings::common().get( SettingsAccess::sKeyAnisotropicPower(),
			    anisotropicpower );
    osgtexture_->setAnisotropicPower( anisotropicpower );

    osgtexture_->ref();
    texturecallbackhandler_->ref();
    osgtexture_->addCallback( texturecallbackhandler_ );

    addChannel();
}


TextureChannels::~TextureChannels()
{
    deepErase( channelinfo_ );
    setChannels2RGBA( nullptr );

    osgtexture_->removeCallback( texturecallbackhandler_ );
    texturecallbackhandler_->unref();
    osgtexture_->unref();
}


void TextureChannels::setSize( int channel, int s0, int s1, int s2 )
{
    if ( !channelinfo_.validIdx(channel) )
	return;

    channelinfo_[channel]->setSize( s0, s1, s2 );
}


int TextureChannels::getSize( int channel, unsigned char dim ) const
{
    if ( !channelinfo_.validIdx(channel) )
	return -1;

    return channelinfo_[channel]->getSize( dim );
}


void TextureChannels::setOrigin( int channel, const Coord& origin )
{
    if ( channelinfo_.validIdx(channel) )
	channelinfo_[channel]->setOrigin( origin );
}


const Coord& TextureChannels::getOrigin( int channel ) const
{
    if ( !channelinfo_.validIdx(channel) )
	return Coord::udf();

    return channelinfo_[channel]->getOrigin();
}


void TextureChannels::setScale( int channel, const Coord& scale )
{
    if ( channelinfo_.validIdx(channel) )
	channelinfo_[channel]->setScale( scale );
}


const Coord& TextureChannels::getScale( int channel ) const
{
    if ( !channelinfo_.validIdx(channel) )
	return Coord::udf();

    return channelinfo_[channel]->getScale();
}


StepInterval<float> TextureChannels::getEnvelopeRange( unsigned char dim ) const
{
    if ( !osgtexture_->isEnvelopeDefined() )
	return StepInterval<float>::udf();

    if ( dim > 1 ) dim = 1;

    osg::Vec2f center = osgtexture_->envelopeCenter();
    osg::Vec2f halfsz = osgtexture_->textureEnvelopeSize() * 0.5f;

    return StepInterval<float>( center[dim]-halfsz[dim],
				center[dim]+halfsz[dim],
				1.0/osgtexture_->tilingPlanResolution()[dim] );
}


bool TextureChannels::turnOn( bool yn )
{
    bool res = isOn();
    osgtexture_->turnOn( yn );

    return res;
}


bool TextureChannels::isOn() const
{
    return osgtexture_->isOn();
}


int TextureChannels::nrChannels() const
{ return channelinfo_.size(); }


int TextureChannels::addChannel()
{
    TypeSet<int> osgids;

    const int osgid = osgtexture_->addDataLayer();
    const osg::Vec4f imageudfcolor( 1.0, 1.0, 1.0, 0.0 );
    osgtexture_->setDataLayerImageUndefColor( osgid, imageudfcolor );
    osgtexture_->setDataLayerFilterType( osgid, mGetFilterType );

    osgids += osgid;

    auto* newchannel = new ChannelInfo( *this );

    const int res = channelinfo_.size();
    if ( res )
    {
	newchannel->setSize( channelinfo_[0]->getSize(0),
			     channelinfo_[0]->getSize(1),
			     channelinfo_[0]->getSize(2) );
	newchannel->setOrigin( channelinfo_[0]->getOrigin() );
	newchannel->setScale( channelinfo_[0]->getScale() );
    }

    channelinfo_.add( newchannel );
    newchannel->setOsgIDs( osgids );

    update( res );

    if ( tc2rgba_ )
	tc2rgba_->notifyChannelInsert( res );

    return res;
}


void TextureChannels::swapChannels( int t0, int t1 )
{
    if ( t0<0 || t1<0 || t0>=channelinfo_.size() || t1>=channelinfo_.size() )
	return;

    channelinfo_.swap( t0, t1 );


    update( t0 );
    update( t1 );

    if ( tc2rgba_ )
	tc2rgba_->swapChannels( t0, t1 );
}


int TextureChannels::insertChannel( int channel )
{
    if ( channel>=channelinfo_.size() )
	return addChannel();

    if ( channel<0 )
	{ pErrMsg("Negative index"); channel=0; }

    auto* newchannel = new ChannelInfo( *this );
    channelinfo_.insertAt( newchannel, channel );
    for ( int idy=channel; idy<nrChannels(); idy++ )
	update( idy );

    if ( tc2rgba_ )
	tc2rgba_->notifyChannelInsert( channel );

    return channel;
}


void TextureChannels::removeChannel( int channel )
{
    if ( !channelinfo_.validIdx(channel) )
	return;

    const TypeSet<int>& osgidxs = channelinfo_.get(channel)->getOsgIDs();
    for ( int idx=osgidxs.size()-1; idx>=0; idx-- )
	osgtexture_->removeDataLayer( osgidxs[idx] );

    delete channelinfo_.removeSingle(channel);

    if ( tc2rgba_ )
	tc2rgba_->notifyChannelRemove( channel );
}


void TextureChannels::setColTabMapperSetup( int channel,
					    const ColTab::MapperSetup& setup )
{
    if ( channel<0 || channel>=channelinfo_.size() )
	{ pErrMsg("Index out of bounds"); return; }

    channelinfo_[channel]->setColTabMapperSetup( setup );
}


void TextureChannels::reMapData( int channel, bool dontreclip, TaskRunner* tr )
{
    if ( channel<0 || channel>=channelinfo_.size() )
	{ pErrMsg("Index out of bounds"); return; }

    channelinfo_[channel]->reMapData( dontreclip, tr );
}


const TypeSet<float>* TextureChannels::getHistogram( int channel ) const
{
    if ( !channelinfo_.validIdx(channel) )
	return 0;

    return &channelinfo_[channel]->getHistogram();
}


const ColTab::MapperSetup&
TextureChannels::getColTabMapperSetup( int channel, int version ) const
{ return channelinfo_[channel]->getColTabMapperSetup( version ); }


const ColTab::Mapper&
TextureChannels::getColTabMapper( int channel, int version ) const
{ return channelinfo_[channel]->getColTabMapper( version ); }


int TextureChannels::getNrComponents( int channel ) const
{ return channelinfo_[channel]->nrComponents(); }


void TextureChannels::setNrComponents( int channel, int newsz )
{
    if ( !channelinfo_.validIdx(channel) )
	return;

    if ( newsz==channelinfo_[channel]->nrComponents() )
	return;

    TypeSet<int> osgids = channelinfo_[channel]->getOsgIDs();
    while ( osgids.size()<newsz )
    {
	const int osgid = osgtexture_->addDataLayer();
	osgtexture_->setDataLayerFilterType( osgid, mGetFilterType );
	osgids += osgid;
    }

    while ( osgids.size()>newsz )
    {
	osgtexture_->removeDataLayer( osgids[newsz] );
	osgids.removeSingle( newsz );
    }

    channelinfo_[channel]->setOsgIDs( osgids );
}


int TextureChannels::nrVersions( int channel ) const
{ return channelinfo_[channel]->nrVersions(); }


void TextureChannels::setNrVersions( int channel, int newsz )
{
    if ( !channelinfo_.validIdx(channel) )
	return;

    channelinfo_[channel]->setNrVersions( newsz );
}


int TextureChannels::currentVersion( int channel ) const
{
    if ( !channelinfo_.validIdx(channel) )
	channel = 0;

    return channelinfo_[channel]->getCurrentVersion();
}


void TextureChannels::setCurrentVersion( int channel, int version )
{
    if ( !channelinfo_.validIdx(channel) )
	channel = 0;

    channelinfo_[channel]->setCurrentVersion( version );
}


bool TextureChannels::isCurrentDataPremapped( int channel ) const
{
    if ( !channelinfo_.validIdx(channel) )
	return false;

    return channelinfo_[channel]->isCurrentDataPremapped();
}


#define mErrRet \
{ \
    if ( managedata ) delete [] data; \
    return false; \
}

bool TextureChannels::setUnMappedVSData( int channel, int version,
			    const ValueSeries<float>* data, OD::PtrPolicy cp,
			    TaskRunner* tr, bool skipclip )
{
    if ( channel<0 || channel>=channelinfo_.size() )
    {
	if ( cp==OD::TakeOverPtr ) delete data;
	return false;
    }

    return channelinfo_[channel]->setUnMappedData( version, data, cp,
						   tr, skipclip );
}


bool TextureChannels::setUnMappedData( int channel, int version,
	const float* data, OD::PtrPolicy cp, TaskRunner* tr, bool skipclip )
{
    if ( !channelinfo_.validIdx(channel) )
    {
	if ( cp==OD::TakeOverPtr ) delete [] data;
	return false;
    }

    const od_int64 nrelements = channelinfo_[channel]->nrElements( false );
    const float* useddata = data;
    if ( useddata && cp==OD::CopyPtr )
    {
	mDeclareAndTryAlloc( float*, newdata, float[nrelements] );
	if ( !useddata )
	    return false;

	OD::memCopy( newdata, data, nrelements * sizeof(float) );
	cp = OD::TakeOverPtr;
	useddata = newdata;
    }

    ValueSeries<float>* vs = useddata
	? new ArrayValueSeries<float,float>(
	    const_cast<float*>(useddata), cp==OD::TakeOverPtr, nrelements )
	: nullptr;

    return channelinfo_[channel]->setUnMappedData( version, vs, OD::TakeOverPtr,
						   tr, skipclip );
}


bool TextureChannels::setMappedData( int channel, int version,
				     unsigned char* data,
				     OD::PtrPolicy cp )
{
    setUnMappedData( channel, version, 0, OD::UsePtr, nullptr );

    if ( channel<0 || channel>=channelinfo_.size() )
    {
	if ( cp==OD::TakeOverPtr ) delete [] data;
	return false;
    }

    return channelinfo_[channel]->setMappedData( version, data, cp );
}


bool TextureChannels::setChannels2RGBA( TextureChannel2RGBA* nt )
{
    const int oldnrtexturebands = nrTextureBands();

    if ( tc2rgba_ )
    {
	tc2rgba_->setChannels( 0 );
	tc2rgba_->unRef();
    }

    tc2rgba_ = nt;

    if ( tc2rgba_ )
    {
	tc2rgba_->setChannels( this );
	tc2rgba_->ref();

	for ( int channel=0; channel<nrChannels(); channel++ )
	{
	    if ( oldnrtexturebands != nrTextureBands() )
		reMapData( channel, true, 0 );

	    update( channel );
	}
    }

    return true;
}


const TextureChannel2RGBA* TextureChannels::getChannels2RGBA() const
{ return tc2rgba_; }


TextureChannel2RGBA* TextureChannels::getChannels2RGBA()
{ return tc2rgba_; }


const SbImagei32* TextureChannels::getChannels() const
{
    return nullptr;
}


void TextureChannels::update( ChannelInfo* ti )
{
    const int channel= channelinfo_.indexOf( ti );
    if ( channel==-1 )
	return;

    update( channel );
}


unsigned char TextureChannels::nrDataBands() const
{
    const bool uselowressignalpower = true;    // tuneable

    unsigned char nrdatabands = 1;
    // Improve color table mapping quality of mipmapped fragment shader
    if ( tc2rgba_ && tc2rgba_->canSetSequence() && tc2rgba_->canUseShading() )
	nrdatabands += uselowressignalpower ? 1 : 2;

    return nrdatabands;
}


void TextureChannels::update( int channel, bool freezeifnodata )
{
    channelinfo_[channel]->updateOsgImages();
    for ( int component=channelinfo_[channel]->nrComponents()-1;
	  component>=0; component-- )
    {
	const int osgid = channelinfo_[channel]->osgids_[component];

	if ( isCurrentDataPremapped(channel) )
	{
	    osgtexture_->setDataLayerImage( osgid,
		channelinfo_[channel]->osgimages_[component], freezeifnodata );
	    osgtexture_->setDataLayerUndefLayerID( osgid, -1 );
	}
	else
	{
	    osgtexture_->setDataLayerImage( osgid,
		    channelinfo_[channel]->osgimages_[component],
		    freezeifnodata, nrDataBands()-1 );

	    const int udflayerid = nrUdfBands() ? osgid : -1;
	    osgtexture_->setDataLayerUndefLayerID( osgid, udflayerid );
	    const int udfchannel = nrTextureBands()==3 ? 2 : 3;
	    osgtexture_->setDataLayerUndefChannel( osgid, udfchannel );

	    osg::Vec4f bordercolor( 1.0, 1.0, 1.0, 1.0 );
	    bordercolor[udfchannel] = 0.0;
	    osgtexture_->setDataLayerBorderColor( osgid, bordercolor );
	}

	osgtexture_->setDataLayerOrigin( osgid,
		Conv::to<osg::Vec2f>(channelinfo_[channel]->getOrigin()) );
	osgtexture_->setDataLayerScale( osgid,
		Conv::to<osg::Vec2f>(channelinfo_[channel]->getScale()) );
    }

    if ( getChannels2RGBA() )
	getChannels2RGBA()->setChannels( this );
}


void TextureChannels::touchMappedData()
{
    pErrMsg("Is this function needed?");
}


const TypeSet<int>* TextureChannels::getOsgIDs( int channel ) const
{
    if ( channel<0 || channel>=channelinfo_.size() )
	return nullptr;

    return &channelinfo_[channel]->getOsgIDs();
}


void TextureChannels::enableTextureInterpolation( bool yn )
{
    interpolatetexture_ = yn;

    for ( int idx=0; idx<osgtexture_->nrDataLayers(); idx++ )
    {
	const int layerid = osgtexture_->getDataLayerID( idx );
	if ( osgtexture_->getDataLayerFilterType(layerid) != mGetFilterType )
	    osgtexture_->setDataLayerFilterType( layerid, mGetFilterType );
    }
}


bool TextureChannels::textureInterpolationEnabled() const
{
    return interpolatetexture_;
}


void TextureChannels::setNonShaderResolution( int resolution )
{
    osgtexture_->setCompositeSubsampleSteps( resolution+1 );
}


int TextureChannels::getNonShaderResolution() const
{
    return osgtexture_->getCompositeSubsampleSteps()-1;
}


void TextureChannels::unfreezeOldData( int channel )
{
    if ( channelinfo_.validIdx(channel) )
	update( channel, false );
}


}; // namespace visBase
