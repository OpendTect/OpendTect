/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jun 2008
___________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "vistexturechannels.h"

#include "vistexturechannel2rgba.h"
#include "SoTextureChannelSet.h"
#include "SoTextureComposer.h"
#include "Inventor/nodes/SoSwitch.h"
#include "coltabmapper.h"

#include <osg/Image>
#include <osgGeo/LayeredTexture>

#define mNrColors	255




mCreateFactoryEntry( visBase::TextureChannels );
mCreateFactoryEntry( visBase::TextureComposer );


namespace visBase
{


class ChannelInfo : public CallBacker
{
public:
				ChannelInfo( TextureChannels& );
				~ChannelInfo();

    void			setSize(int,int,int);
    int				getSize(unsigned char dim) const;
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
	    					OD::PtrPolicy, TaskRunner*);
    bool			setMappedData(int version,unsigned char*,
	    				      OD::PtrPolicy);

    void			clipData(int version,TaskRunner*);
    				//!<If version==-1, all versions will be clipped
    bool			mapData(int version,TaskRunner*);
    int				getCurrentVersion() const;
    void			setCurrentVersion( int );

    void			updateOsgImages();

    ObjectSet<unsigned char>			mappeddata_;
    BoolTypeSet					ownsmappeddata_;
    ObjectSet<const ValueSeries<float> >	unmappeddata_;
    BoolTypeSet					ownsunmappeddata_;
    ObjectSet<ColTab::Mapper>			mappers_;
    int						currentversion_;
    TextureChannels&				texturechannels_;
    TypeSet<float>				histogram_;
    int						size_[3];

    ObjectSet<osg::Image>			osgimages_;
    TypeSet<int>				osgids_;
};


ChannelInfo::ChannelInfo( TextureChannels& nc )
    : texturechannels_( nc )
    , currentversion_( 0 )
    , histogram_( mNrColors, 0 )
{
    size_[0] = 0;
    size_[1] = 0;
    size_[2] = 0;

    osgimages_.allowNull(true);
    mappeddata_.allowNull(true);
    unmappeddata_.allowNull(true);
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
    {
	pErrMsg("No mappers");
	return;
    }

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
    else
    {
	pErrMsg( "channel >= mappers_.size()" );
	if ( mappers_.isEmpty() )
	{
	    static ColTab::MapperSetup ctms;
	    return ctms;
	}
	return mappers_[mappers_.size()-1]->setup_;
    }
}


void ChannelInfo::clipData( int version, TaskRunner* tr )
{
    const od_int64 nrelements = nrElements( false );

    for ( int idx=0; idx<nrVersions(); idx++ )
    {
	if ( version!=-1 && idx!=version )
	    continue;

	if ( !unmappeddata_[idx] )
	    continue;

	mappers_[idx]->setData( unmappeddata_[idx], nrelements, tr );
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
    mObjectSetApplyToAll( osgimages_,
	    { if ( osgimages_[idx] ) osgimages_[idx]->unref();
	      osgimages_.replace( idx, 0 ); } );
}


void ChannelInfo::removeCaches()
{
    ObjectSet<unsigned char> mappeddata = mappeddata_;
    ObjectSet<const ValueSeries<float> > unmappeddata = unmappeddata_;
    for ( int idx=0; idx<ownsmappeddata_.size(); idx++ )
    {
	mappeddata_.replace( idx, 0 );
	unmappeddata_.replace( idx, 0 );
    }

    removeImages();

    texturechannels_.update( this, true );

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

	mappeddata_.remove( nsz );
	unmappeddata_.remove( nsz );
	ownsmappeddata_.remove( nsz );
	ownsunmappeddata_.remove( nsz );
	delete mappers_.remove( nsz );
    }

    const ColTab::MapperSetup* templ = mappers_.size()
	? &mappers_[0]->setup_
	: 0;

    while ( mappeddata_.size()<nsz )
    {
	mappeddata_ += 0;
	unmappeddata_ += 0;
	ownsmappeddata_ += false;
	ownsunmappeddata_ += false;

	ColTab::Mapper* mapper = new ColTab::Mapper;
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
	osgimages_ += 0;

    while ( osgimages_.size()>nr )
    {
	osg::Image* image = osgimages_.remove( nr );
	if ( image )
	    image->unref();
    }
}


bool ChannelInfo::setUnMappedData(int version, const ValueSeries<float>* data,
	    			  OD::PtrPolicy policy, TaskRunner* tr )
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

	unmappeddata_.replace( version, 0 );
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
	    unmappeddata_.replace( version, 0 );
	}
	    
	ownsunmappeddata_[version] = true;
    }

    if ( mappers_[version]->setup_.type_!=ColTab::MapperSetup::Fixed )
	clipData( version, tr );

    return mapData( version, tr );
}


bool ChannelInfo::mapData( int version, TaskRunner* tr )
{
    if ( version<0 || version>=unmappeddata_.size() )
	return true;

    //Make sure old data is alive until it's replaced in Coin
    ArrPtrMan<unsigned char> oldptr = 0;

    if ( mappeddata_[version] &&
 	 (!ownsmappeddata_[version] || !unmappeddata_[version] ) )
    {
	if ( ownsmappeddata_[version] )
	    oldptr = mappeddata_[version];

	mappeddata_.replace( version, 0 );
    }

    if ( !unmappeddata_[version] )
    {
	texturechannels_.update( this, true );
	removeImages();
	return true;
    }

    const od_int64 nrelements = nrElements( false );
    const unsigned char spacing = texturechannels_.doOsg() ? 2 : 1;

    if ( !mappeddata_[version] )
    {
	mDeclareAndTryAlloc(unsigned char*, mappeddata,
			    unsigned char[nrelements*spacing] );
	if ( !mappeddata ) return false;

	mappeddata_.replace( version, mappeddata );
	ownsmappeddata_[version] = true;
    }

    ColTab::MapperTask< unsigned char> 	maptask( *mappers_[version], nrelements,
	    mNrColors, *unmappeddata_[version],
	    mappeddata_[version], spacing,
	    texturechannels_.doOsg() ? mappeddata_[version]+1 : 0, spacing  );

    if ( ( tr && tr->execute(maptask) ) || maptask.execute() )
    {
	int max = 0;
	const unsigned int* histogram = maptask.getHistogram();
	for ( int idx=mNrColors-1; idx>=0; idx-- )
	{
	    if ( histogram[idx]>max )
		max = histogram[idx];
	}

	if ( max )
	{
	    for ( int idx=mNrColors-1; idx>=0; idx-- )
		histogram_[idx] = (float) histogram[idx]/max;
	}
	else
	{
	    memset( histogram_.arr(), 0, histogram_.size()*sizeof(float) );
	}

	texturechannels_.update( this, true );
	return true;
    }

    return false;
}


bool ChannelInfo::setMappedData( int version, unsigned char* data,
	    			  OD::PtrPolicy policy )
{
    if ( mappeddata_.size()<version )
    {
	pErrMsg("nrVersions not set" );
	return false;
    }

    if ( mappeddata_[version] )
    {
	if (  ownsmappeddata_[version] )
	    delete mappeddata_[version];
	mappeddata_.replace( version, 0 );
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
	    MemCopier<unsigned char> copier( newdata, data, nrelements );
	    copier.execute();
    	    mappeddata_.replace( version, newdata );
	}
	else
	{
	    mappeddata_.replace( version, 0 );
	}
	
	ownsmappeddata_[version] = true;
    }

    texturechannels_.update( this, true );
    return true;
}


int ChannelInfo::getCurrentVersion() const { return currentversion_; }


void ChannelInfo::setCurrentVersion( int nidx )
{
    if ( mappeddata_.size() && (nidx<0 || nidx>=mappeddata_.size() ) )
    {
	pErrMsg("Invalid index");
	return;
    }

    currentversion_ = nidx;
    texturechannels_.update( this, true );
}


void ChannelInfo::updateOsgImages()
{
    if ( !mappeddata_[currentversion_] )
    {
	removeImages();
	return;
    }

    const od_int64 componentsize = nrElements( true );
    for ( int idx=0; idx<osgimages_.size(); idx++ )
    {
	if ( !osgimages_[idx] )
	{
	    osg::Image* image = new osg::Image;
	    image->ref();
	    osgimages_.replace( idx, image );
	}

	const od_int64 offset = idx * componentsize*2;

	osgimages_[idx]->setImage( size_[2], size_[1],
			    size_[0], GL_LUMINANCE_ALPHA,
			    GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE,
			    mappeddata_[currentversion_]+offset,
			    osg::Image::NO_DELETE, 1 );
    }
}


TextureChannels::TextureChannels()
    : tc_( 0 )
    , onoff_ ( new SoSwitch )
    , tc2rgba_( 0 )
    , osgtexture_( 0 )
{
    onoff_->ref();
    turnOn( true );

    if ( doOsg() )
    {
	osgtexture_ = new osgGeo::LayeredTexture;
	osgtexture_->invertUndefLayers();
	osgtexture_->ref();
    }

    addChannel();
}


TextureChannels::~TextureChannels()
{
    deepErase( channelinfo_ );
    setChannels2RGBA( 0 );
    onoff_->unref();

    if ( osgtexture_ ) osgtexture_->unref();
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


void TextureChannels::setSize( int s0, int s1, int s2 )
{
    for ( int channel=0; channel<channelinfo_.size(); channel++ )
	setSize( channel, s0, s1, s2 );
}


int TextureChannels::getSize( unsigned char dim ) const
{
    if ( channelinfo_.size() )
	return getSize( 0, dim );

    return -1;
}


bool TextureChannels::turnOn( bool yn )
{
    const bool res = isOn();
    const int newval = yn ? SO_SWITCH_ALL : SO_SWITCH_NONE;
    if ( newval!=onoff_->whichChild.getValue() )
	onoff_->whichChild = newval;
    return res;
}


bool TextureChannels::isOn() const
{
    return onoff_->whichChild.getValue()!=SO_SWITCH_NONE;
}


int TextureChannels::nrChannels() const
{ return channelinfo_.size(); }


int TextureChannels::addChannel()
{
    TypeSet<int> osgids;

    int osgid = -1;
    if ( osgtexture_ )
    {
	osgid = osgtexture_->addDataLayer();
	osgtexture_->setDataLayerUndefLayerID( osgid, osgid );
	osgtexture_->setDataLayerUndefChannel( osgid, 3 );
	const osg::Vec4f imageudfcolor( 1.0, 1.0, 1.0, 0.0 );
	osgtexture_->setDataLayerImageUndefColor( osgid, imageudfcolor );
    }

    osgids += osgid;

    ChannelInfo* newchannel = new ChannelInfo( *this );

    const int res = channelinfo_.size();
    if ( res ) //&& !doOsg() ) For later
	newchannel->setSize( channelinfo_[0]->getSize(0),
			     channelinfo_[0]->getSize(1),
			     channelinfo_[0]->getSize(2) );

    channelinfo_ += newchannel;
    newchannel->setOsgIDs( osgids );

    update ( res, false );

    if ( tc2rgba_ )
	tc2rgba_->notifyChannelInsert( res );

    return res;
}


void TextureChannels::swapChannels( int t0, int t1 )
{
    if ( t0<0 || t1<0 || t0>=channelinfo_.size() || t1>=channelinfo_.size() )
	return;

    channelinfo_.swap( t0, t1 );


    update( t0, false );
    update( t1, false );

    if ( tc2rgba_ )
	tc2rgba_->swapChannels( t0, t1 );
}


int TextureChannels::insertChannel( int channel )
{
    if ( channel>=channelinfo_.size() )
	return addChannel();

    if ( channel<0 )
    {
	pErrMsg("Negative index");
	channel=0;
    }

    ChannelInfo* newchannel = new ChannelInfo( *this );
    channelinfo_.insertAt( newchannel, channel );
    for ( int idy=channel; idy<nrChannels(); idy++ )
	update( idy, false );

    if ( tc2rgba_ )
	tc2rgba_->notifyChannelInsert( channel );
    
    return channel;
}


void TextureChannels::removeChannel( int channel )
{
    if ( !channelinfo_.validIdx(channel) )
	return;

    PtrMan<ChannelInfo> info = channelinfo_[channel];

    if ( osgtexture_ )
    {
	for ( int idx=info->getOsgIDs().size()-1; idx>=0; idx-- )
	    osgtexture_->removeDataLayer( info->getOsgIDs()[idx] );
    }

    channelinfo_.remove(channel);

    bool oldenable = tc_->enableNotify( false );
    for ( int idy=channel; idy<nrChannels(); idy++ )
	update( idy, false );

    tc_->setNrChannels( nrChannels() );
    tc_->enableNotify( oldenable );
    tc_->touch();

    if ( tc2rgba_ )
	tc2rgba_->notifyChannelRemove( channel );
}


void TextureChannels::setColTabMapperSetup( int channel,
					    const ColTab::MapperSetup& setup )
{
    if ( channel<0 || channel>=channelinfo_.size() )
	pErrMsg("Index out of bounds");

    channelinfo_[channel]->setColTabMapperSetup( setup );
}


void TextureChannels::reMapData( int channel, bool dontreclip, TaskRunner* tr )
{
    if ( channel<0 || channel>=channelinfo_.size() )
	pErrMsg("Index out of bounds");

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
	osgids += osgtexture_->addDataLayer();

    while ( osgids.size()>newsz )
    {
	osgtexture_->removeDataLayer( osgids[newsz] );
	osgids.remove( newsz );
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


#define mErrRet \
{ \
    if ( managedata ) delete [] data; \
    return false; \
}

bool TextureChannels::setUnMappedVSData( int channel, int version,
	const ValueSeries<float>* data, OD::PtrPolicy cp, TaskRunner* tr )
{
    if ( channel<0 || channel>=channelinfo_.size() )
    {
	if ( cp==OD::TakeOverPtr ) delete data;
	return false;
    }

    return channelinfo_[channel]->setUnMappedData( version, data, cp, tr );
}


bool TextureChannels::setUnMappedData( int channel, int version,
	const float* data, OD::PtrPolicy cp, TaskRunner* tr )
{
    if ( !channelinfo_.validIdx(channel) )
    {
	if ( cp==OD::TakeOverPtr ) delete [] data;
	return false;
    }

    const float* useddata = data;
    if ( useddata && cp==OD::CopyPtr )
    {
	const od_int64 nrelements = channelinfo_[channel]->nrElements( false );
	mDeclareAndTryAlloc( float*, newdata, float[nrelements] );
	if ( !useddata )
	    return false;

	MemCopier<float> copier( newdata, data, nrelements );
	if ( !copier.execute() )
	{
	    delete [] newdata;
	    return false;
	}

	cp = OD::TakeOverPtr;
	useddata = newdata;
    }

    ValueSeries<float>* vs = useddata
	? new ArrayValueSeries<float,float>( 
	    const_cast<float*>(useddata), cp==OD::TakeOverPtr )
	: 0;

    return channelinfo_[channel]->setUnMappedData( version, vs,
						   OD::TakeOverPtr, tr );
}


bool TextureChannels::setMappedData( int channel, int version,
				     unsigned char* data, 
				     OD::PtrPolicy cp )
{
    if ( channel<0 || channel>=channelinfo_.size() )
    {
	if ( cp==OD::TakeOverPtr ) delete [] data;
	return false;
    }

    return channelinfo_[channel]->setMappedData( version, data, cp );
}


bool TextureChannels::setChannels2RGBA( TextureChannel2RGBA* nt )
{
    if ( tc2rgba_ )
    {
	onoff_->removeChild( tc2rgba_->getInventorNode() );
	tc2rgba_->setChannels( 0 );
	tc2rgba_->unRef();

	onoff_->removeChild( tc_->getInventorNode() );
	tc_->unRef();
	tc_ = 0;
    }

    tc2rgba_ = nt;

    if ( tc2rgba_ )
    {
	tc_ = tc2rgba_->createMappedDataSet();
	if ( !tc_ )
	{
	    tc2rgba_ = 0;
	    return false;
	}

	tc_->ref();
	onoff_->addChild( tc_->getInventorNode() );

	onoff_->addChild( tc2rgba_->getInventorNode() );
	tc2rgba_->setChannels( this );
	tc2rgba_->ref();

	for ( int channel=0; channel<nrChannels(); channel++ )
	    update( channel, false );
    }

    return true;
}


const TextureChannel2RGBA* TextureChannels::getChannels2RGBA() const
{ return tc2rgba_; }


TextureChannel2RGBA* TextureChannels::getChannels2RGBA()
{ return tc2rgba_; }


SoNode* TextureChannels::gtInvntrNode()
{ return onoff_; }


const SbImagei32* TextureChannels::getChannels() const
{
    return tc_->getChannelData();
}


void TextureChannels::update( ChannelInfo* ti, bool tc2rgba )
{
    const int channel= channelinfo_.indexOf( ti );
    if ( channel==-1 )
	return;

    update( channel, tc2rgba );
}


void TextureChannels::update( int channel, bool tc2rgba )
{
    if ( osgtexture_ )
    {
	channelinfo_[channel]->updateOsgImages();
	for ( int component=channelinfo_[channel]->nrComponents()-1;
	      component>=0; component-- )
	{
	    osgtexture_->setDataLayerImage(
		    channelinfo_[channel]->osgids_[component],
		    channelinfo_[channel]->osgimages_[component] );
	}

	return;
    }

    if ( !tc_ )
	return;

    SbImagei32 image;
    const int curversion = channelinfo_[channel]->getCurrentVersion();
    image.setValuePtr( SbVec3i32( channelinfo_[channel]->getSize(0),
				  channelinfo_[channel]->getSize(1),
				  channelinfo_[channel]->getSize(2) ), 1,
	    	       channelinfo_[channel]->mappeddata_[curversion] );
    tc_->setChannelData( channel, image );

    if ( tc2rgba && tc2rgba_ )
	tc2rgba_->notifyChannelChange();
}


void TextureChannels::touchMappedData()
{
    tc_->touch();
}


const TypeSet<int>* TextureChannels::getOsgIDs( int channel ) const
{
    if ( channel<0 || channel>=channelinfo_.size() )
	return 0;

    return &channelinfo_[channel]->getOsgIDs();
}


// Texture Composer

TextureComposer::TextureComposer()
    : texturecomposer_(new SoTextureComposer())
{
    texturecomposer_->ref();
}


TextureComposer::~TextureComposer()
{
    texturecomposer_->unref();
}


SoNode* TextureComposer::gtInvntrNode()
{
    return texturecomposer_;
}


void TextureComposer::setOrigin( int val1, int val2, int val3 )
{
    texturecomposer_->origin.setValue( val1, val2, val3 );
}


void TextureComposer::setSize( int xsz, int ysz, int zsz )
{
    texturecomposer_->size.setValue( xsz, ysz, zsz );
}

}; // namespace visBase
