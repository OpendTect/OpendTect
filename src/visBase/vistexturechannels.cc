/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jun 2008
___________________________________________________________________

-*/

static const char* rcsID = "$Id: vistexturechannels.cc,v 1.3 2008-10-09 21:27:27 cvskris Exp $";

#include "vistexturechannels.h"

#include "vistexturechannel2rgba.h"
#include "SoTextureChannelSet.h"
#include "Inventor/nodes/SoSwitch.h"
#include "coltabmapper.h"

#define mNrColors	255
#define mUndefColIdx	255

namespace visBase
{


class ChannelInfo : public CallBacker
{
public:
    			ChannelInfo( TextureChannels& );
    			~ChannelInfo();

    void		setColTabMapperSetup(const ColTab::MapperSetup&);
    const ColTab::MapperSetup& getColTabMapperSetup() const;
    bool		reMapData();

    void		setNrVersions(int);
    int			nrVersions() const;

    bool		removeCaches();

    bool		setUnMappedData(int version,const float*,
	    				 TextureChannels::CachePolicy);
    bool		setMappedData(int version,unsigned char*,
	    			       TextureChannels::CachePolicy);

    void		clipData();
    bool		mapData(int version);
    int			getCurrentVersion() const;
    void		setCurrentVersion( int );

    bool		mapData( const float*, unsigned char* ) const;

    ObjectSet<unsigned char>			mappeddata_;
    BoolTypeSet					ownsmappeddata_;
    ObjectSet<const float>			unmappeddata_;
    BoolTypeSet					ownsunmappeddata_;
    mutable ColTab::Mapper			mapper_;
    int						currentversion_;
    TextureChannels&				owner_;
};


ChannelInfo::ChannelInfo( TextureChannels& nc )
    : owner_( nc )
    , currentversion_( 0 )
{
    mappeddata_.allowNull(true);
    unmappeddata_.allowNull(true);
    setNrVersions( 1 );
}


ChannelInfo::~ChannelInfo()
{
    setNrVersions( 0 );
}


void ChannelInfo::setColTabMapperSetup( const ColTab::MapperSetup& setup )
{
    if ( mapper_.setup_==setup )
	return;

    mapper_.setup_ = setup;

    if ( mapper_.setup_.type_!=ColTab::MapperSetup::Fixed )
	clipData();

    reMapData();
}


const ColTab::MapperSetup& ChannelInfo::getColTabMapperSetup() const
{
    return mapper_.setup_;
}


void ChannelInfo::clipData()
{
    od_int64 sz = owner_.size_[0];
    sz *= owner_.size_[1];
    sz *= owner_.size_[2];

    for ( int idx=0; idx<nrVersions(); idx++ )
    {
	if ( !unmappeddata_[idx] )
	    continue;

	const ArrayValueSeries<float,float> valseries(
		(float*) unmappeddata_[idx], false, sz );
	mapper_.setData( &valseries, sz );
	break;
    }
}


bool ChannelInfo::reMapData() 
{
    for ( int idx=nrVersions()-1; idx>=0; idx-- )
    {
	if ( !mapData( idx ) )
	    return false;
    }

    return true;
}

void ChannelInfo::setNrVersions( int nsz )
{
    while ( nsz<mappeddata_.size() )
    {
	if ( ownsmappeddata_[nsz] )
	    delete [] mappeddata_[nsz];
	if ( ownsunmappeddata_[nsz] )
	    delete [] unmappeddata_[nsz];

	ownsmappeddata_.remove( nsz );
	ownsmappeddata_.remove( nsz );
    }

    while ( mappeddata_.size()<nsz )
    {
	mappeddata_ += 0;
	unmappeddata_ += 0;
	ownsmappeddata_ += false;
	ownsunmappeddata_ += false;
    }

    if ( currentversion_>=nsz )
	setCurrentVersion( nsz-1 );
}


int ChannelInfo::nrVersions() const
{ return ownsmappeddata_.size(); }


bool ChannelInfo::setUnMappedData(int version, const float* data,
	    			   TextureChannels::CachePolicy policy )
{
    if ( version<0 || version>=nrVersions() )
    {
	if ( policy==TextureChannels::TakeOver ) delete [] data;
	return false;
    }

    od_int64 sz = owner_.size_[0];
    sz *= owner_.size_[1];
    sz *= owner_.size_[2];


    if ( unmappeddata_[version] )
    {
	if ( ownsunmappeddata_[version] )
	    delete [] unmappeddata_[version];

	unmappeddata_.replace( version, 0 );
    }

    if ( policy==TextureChannels::Cache || policy==TextureChannels::TakeOver )
    {
	unmappeddata_.replace( version, data );
	ownsunmappeddata_[version] = policy==TextureChannels::TakeOver;
    }
    else if ( policy==TextureChannels::CacheCopy )
    {
	mDeclareAndTryAlloc(float*, newdata, float[sz] );
	if ( !newdata ) return false;
	memcpy( newdata, data, sz*sizeof(float) );
	unmappeddata_.replace( version, newdata );
	ownsunmappeddata_[version] = true;
    }

    return mapData( version );
}


bool ChannelInfo::mapData( int version )
{
    if ( version<0 || version>=unmappeddata_.size() )
	return true;

    if ( mappeddata_[version] && !ownsmappeddata_[version] )
    {
	delete [] mappeddata_[version];
	mappeddata_.replace( version, 0 );
    }

    od_int64 sz = owner_.size_[0];
    sz *= owner_.size_[1];
    sz *= owner_.size_[2];


    if ( !mappeddata_[version] )
    {
	mDeclareAndTryAlloc(unsigned char*, mappeddata, unsigned char[sz] );
	if ( !mappeddata ) return false;

	mappeddata_.replace( version, mappeddata );
    }

    return mapData( unmappeddata_[version], mappeddata_[version] );
}


bool ChannelInfo::mapData( const float* unmapped, unsigned char* mapped ) const
{
    od_int64 sz = owner_.size_[0];
    sz *= owner_.size_[1];
    sz *= owner_.size_[2];

    unsigned char* stopptr = mapped+sz;
    while ( mapped!=stopptr )
    {
	*mapped = ColTab::Mapper::snappedPosition(&mapper_,*unmapped,
						   mNrColors,mUndefColIdx);
	mapped++; unmapped++;
    }

    return true;
}


bool ChannelInfo::setMappedData( int version, unsigned char* data,
	    			  TextureChannels::CachePolicy policy )
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

    if ( policy==TextureChannels::Cache || policy==TextureChannels::TakeOver )
    {
	mappeddata_.replace( version, data );
	ownsmappeddata_[version] = policy==TextureChannels::TakeOver;
    }
    else if ( policy==TextureChannels::CacheCopy )
    {
	od_int64 sz = owner_.size_[0];
	sz *= owner_.size_[1];
	sz *= owner_.size_[2];

	mDeclareAndTryAlloc(unsigned char*, newdata, unsigned char[sz] );
	if ( !newdata ) return false;
	memcpy( newdata, data, sz*sizeof(unsigned char) );
	mappeddata_.replace( version, newdata );
	ownsmappeddata_[version] = true;
    }
}


int ChannelInfo::getCurrentVersion() const { return currentversion_; }


void ChannelInfo::setCurrentVersion( int nidx )
{
    if ( nidx<0 || nidx>=mappeddata_.size() )
    {
	pErrMsg("Invalid index");
	return;
    }

    currentversion_ = nidx;
}


TextureChannels::TextureChannels()
    : tc_( new SoTextureChannelSet )
    , onoff_ ( new SoSwitch )
    , tc2rgba_( 0 )
{
    onoff_->ref();
    onoff_->addChild( tc_ );
    addChannel();
    turnOn( true );
}


TextureChannels::~TextureChannels()
{
    deepErase( channelinfo_ );
    setChannels2RGBA( 0 );
    onoff_->unref();
}


bool TextureChannels::turnOn( bool yn )
{
    const bool res = isOn();
    onoff_->whichChild = yn ? SO_SWITCH_ALL : SO_SWITCH_NONE;
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
    ChannelInfo* newchannel = new ChannelInfo( *this );

    const int res = channelinfo_.size();
    channelinfo_ += newchannel;
    update ( res );
    return res;
};


void TextureChannels::swapChannels( int t0, int t1 )
{
    if ( t0<0 || t1<0 || t0>=channelinfo_.size() || t1>=channelinfo_.size() )
	return;

    channelinfo_.swap( t0, t1 );

    update( t0 );
    update( t1 );
}


int TextureChannels::insertChannel( int idx )
{
    if ( idx>=channelinfo_.size() )
	return addChannel();

    if ( idx<0 )
    {
	pErrMsg("Negative index");
	idx=0;
    }

    ChannelInfo* newchannel = new ChannelInfo( *this );
    channelinfo_.insertAt( newchannel, idx );
    for ( int idy=idx; idy<nrChannels(); idy++ )
	update( idy );

    return idx;
}


void TextureChannels::removeChannel( int idx )
{
    if ( idx<0 || idx>=channelinfo_.size() )
	return;

    delete channelinfo_[idx];
    channelinfo_.remove(idx);

    bool oldenable = tc_->channels.enableNotify( false );
    for ( int idy=idx; idy<nrChannels(); idy++ )
	update( idy );

    tc_->channels.setNum( nrChannels() );
    tc_->channels.enableNotify( oldenable );
    tc_->channels.touch();
}


void TextureChannels::setColTabMapperSetup( int channel,
					    const ColTab::MapperSetup& setup )
{
    if ( channel<0 || channel>=channelinfo_.size() )
	pErrMsg("Index out of bounds");

    channelinfo_[channel]->setColTabMapperSetup( setup );
}


const ColTab::MapperSetup&
TextureChannels::getColTabMapperSetup(int channel) const
{ return channelinfo_[channel]->getColTabMapperSetup(); }



int TextureChannels::nrVersions( int idx ) const 
{ return channelinfo_[idx]->nrVersions(); }


void TextureChannels::setNrVersions( int idx, int newsz )
{
    if ( idx<0 || idx>=channelinfo_.size() )
	return;

    channelinfo_[idx]->setNrVersions( newsz );
}


int TextureChannels::currentVersion( int idx ) const
{
    if ( idx<0 || idx>=channelinfo_.size() )
	idx = 0;

    return channelinfo_[idx]->getCurrentVersion();
}


void TextureChannels::setCurrentVersion( int idx, int version )
{
    if ( idx<0 || idx>=channelinfo_.size() )
	idx = 0;

    channelinfo_[idx]->setCurrentVersion( version );
}


#define mErrRet \
{ \
    if ( managedata ) delete [] data; \
    return false; \
}

bool TextureChannels::setUnMappedData( int channel, int version,
	const float* data, CachePolicy cp )
{
    if ( channel<0 || channel>=channelinfo_.size() )
    {
	if ( cp==TakeOver ) delete [] data;
	return false;
    }

    return channelinfo_[channel]->setUnMappedData( version, data, cp );
}


bool TextureChannels::setMappedData( int channel, int version,
				     unsigned char* data, 
				     CachePolicy cp )
{
    if ( channel<0 || channel>=channelinfo_.size() )
    {
	if ( cp==TakeOver ) delete [] data;
	return false;
    }

    return channelinfo_[channel]->setMappedData( version, data, cp );
}


bool TextureChannels::setChannels2RGBA( TextureChannel2RGBA* nt )
{
    if ( tc2rgba_ )
    {
	onoff_->removeChild( tc2rgba_->getInventorNode() );
	tc2rgba_->unRef();
    }

    tc2rgba_ = nt;

    if ( tc2rgba_ )
    {
	onoff_->addChild( tc2rgba_->getInventorNode() );
	tc2rgba_->ref();
    }

    return true;
}


SoNode* TextureChannels::getInventorNode()
{ return onoff_; }


const SbImage* TextureChannels::getChannels() const
{
    return tc_->channels.getValues( 0 );
}


void TextureChannels::update( int channel )
{
    SbImage image;
    const int curversion = channelinfo_[channel]->getCurrentVersion();
    image.setValuePtr( SbVec3s( size_[0], size_[1], size_[2] ), 1,
	    	       channelinfo_[channel]->mappeddata_[curversion] );
    tc_->channels.set1Value( channel, image );
}


}; // namespace visBase
