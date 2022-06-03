 /*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : D. Zheng
 * DATE     : April 2013
-*/


#include "vishorizontexturehandler.h"
#include "vishorizonsection.h"
#include "vishorizonsectiondef.h"
#include "vishorizonsectiontile.h"

#include "binidvalset.h"
#include "datapointset.h"
#include "datacoldef.h"
#include "posvecdataset.h"
#include "coltabmapper.h"
#include "threadwork.h"

#include "vistexturechannels.h"
#include "vistexturechannel2rgba.h"
#include "binidsurface.h"

#include <osgGeo/LayeredTexture>


namespace visBase
{

HorizonTextureHandler::HorizonTextureHandler( const HorizonSection* horsection )
    : channels_( TextureChannels::create() )
    , channel2rgba_( ColTabTextureChannel2RGBA::create() )
    , horsection_( horsection )
{
    cache_.allowNull( true );
    channel2rgba_->ref();
    channels_->ref();
    channels_->setChannels2RGBA( channel2rgba_ );
    if ( channels_->nrChannels()<1 )
	addChannel();
    else
	cache_ += 0;
    channels_->getOsgTexture()->setTextureSizePolicy(
	osgGeo::LayeredTexture::AnySize );
}


HorizonTextureHandler::~HorizonTextureHandler()
{
    channel2rgba_->unRef();
    deepErase( cache_ );
    channels_->unRef();
}


osg::Node* HorizonTextureHandler::getOsgNode()
{
    return channels_->osgNode();
}


osgGeo::LayeredTexture*	HorizonTextureHandler::getOsgTexture()
{
    return channels_->getOsgTexture();
}


void HorizonTextureHandler::addChannel()
{
    channels_->addChannel();
    cache_ += 0;
    channel2rgba_->setEnabled( nrChannels()-1, true );
}


void HorizonTextureHandler::setColTabSequence(int channel,
					      const ColTab::Sequence& se)
{
    if ( channel>=0 ) channel2rgba_->setSequence( channel, se );
}


const ColTab::Sequence*
HorizonTextureHandler::getColTabSequence( int channel ) const
{
    return channel<0 ? 0 : channel2rgba_->getSequence( channel );
}


void HorizonTextureHandler::setColTabMapperSetup( int channel,
    const ColTab::MapperSetup& mapper, TaskRunner* tr )
{
    if ( channel>=0 )
    {
	const bool needsclip =
	    channels_->getColTabMapperSetup( channel, 0 ).needsReClip( mapper );
	channels_->setColTabMapperSetup( channel, mapper );
	channels_->reMapData( channel, !needsclip, tr );
    }
}


const ColTab::MapperSetup*
HorizonTextureHandler::getColTabMapperSetup( int ch ) const
{
    return ch<0 ? 0 : &channels_->getColTabMapperSetup( ch,activeVersion(ch) );
}



int HorizonTextureHandler::nrChannels() const
{
    return channels_->nrChannels();
}


void HorizonTextureHandler::setChannels2RGBA( TextureChannel2RGBA* t )
{
    channels_->setChannels2RGBA( t );
    if ( channel2rgba_ )
	channel2rgba_->unRef();

    channel2rgba_ = t;

    if ( channel2rgba_ )
	channel2rgba_->ref();
}


TextureChannel2RGBA* HorizonTextureHandler::getChannels2RGBA()
{
    return channel2rgba_;
}


const TextureChannel2RGBA* HorizonTextureHandler::getChannels2RGBA() const
{
    return channel2rgba_;
}


void HorizonTextureHandler::useChannel( bool yn )
{
    channels_->turnOn( yn );
}


void HorizonTextureHandler::removeChannel( int channelidx )
{
    channels_->removeChannel( channelidx );
    delete cache_.removeSingle( channelidx );
}


void HorizonTextureHandler::swapChannels( int channel0, int channel1 )
{
    channels_->swapChannels( channel0, channel1 );
    cache_.swap( channel0, channel1 );
}


int HorizonTextureHandler::nrVersions( int channel ) const
{
    return channels_->nrVersions( channel );
}


void HorizonTextureHandler::setNrVersions( int channel, int nrvers )
{
    channels_->setNrVersions( channel, nrvers);
}


int HorizonTextureHandler::activeVersion( int channel ) const
{
    return channels_->currentVersion( channel );
}


void HorizonTextureHandler::selectActiveVersion( int channel, int version )
{
    channels_->setCurrentVersion( channel, version );
}


const TypeSet<float>* HorizonTextureHandler::getHistogram( int ch ) const
{
    return channels_->getHistogram( ch );
}


void HorizonTextureHandler::setTransparency( int ch, unsigned char yn )
{
    mDynamicCastGet( ColTabTextureChannel2RGBA*, ct, channel2rgba_ );
    if ( ct && ch>=0 ) ct->setTransparency( ch, yn );
}


unsigned char HorizonTextureHandler::getTransparency( int ch ) const
{
    mDynamicCastGet( ColTabTextureChannel2RGBA*, ct, channel2rgba_ );
    if ( !ct )
	return 0;

    return ct->getTransparency( ch );
}


void HorizonTextureHandler::inValidateCache( int channel )
{
    if ( channel==-1 )
    {
	for ( int idx=0; idx<cache_.size(); idx++ )
	    inValidateCache( idx );
    }
    else
    {
	delete cache_[channel];
	cache_.replace( channel, 0 );
    }
}


const BinIDValueSet* HorizonTextureHandler::getCache( int channel ) const
{
    return cache_.validIdx(channel) ? cache_[channel] : 0;
}


void HorizonTextureHandler::setTextureData( int channel,  int sectionid,
				     const DataPointSet* dtpntset )
{
    const BinIDValueSet* data = dtpntset ? &dtpntset->bivSet() : 0;
    if ( channel<0 || channel>=cache_.size() )
	return;

    if ( cache_[channel] )
    {
	if ( !data )
	{
	    delete cache_[channel];
	    cache_.replace( channel, 0 );
	}
	else
	{
	    (*cache_[channel]) = *data;
	}
    }
    else if ( data )
    {
	cache_.replace( channel, new BinIDValueSet(*data) );
    }

    updateTexture( channel, sectionid, dtpntset );
}


void HorizonTextureHandler::updateTexture(int channel,int sectionid,
				   const DataPointSet* dtpntset)
{
    if ( !horsection_ || !horsection_->geometry_ )
	return;

    const BinIDValueSet* data = cache_.validIdx(channel) ? cache_[channel] : 0;
    if ( !horsection_->geometry_->getArray() || !dtpntset || !data )
	return;

    const int nrfixedcols = dtpntset->nrFixedCols();

    const char* hrsectionid = "Section ID";
    const DataColDef sidcoldef( hrsectionid );
    const int sidcol =
	dtpntset->dataSet().findColDef(sidcoldef,PosVecDataSet::NameExact);
    const int shift = sidcol==-1 ?  nrfixedcols : nrfixedcols+1;

    const int nrversions = data->nrVals()-shift;
    setNrVersions( channel, nrversions );

    const StepInterval<int> rrg = horsection_->displayedRowRange();
    const StepInterval<int> crg = horsection_->displayedColRange();
    const int nrrows = rrg.nrSteps()+1;
    const int nrcols = crg.nrSteps()+1;

    channels_->setSize( channel, 1, nrrows, nrcols );

    ObjectSet<float> versiondata;
    versiondata.allowNull( true );
    const int nrcells = nrrows*nrcols;

    for ( int idx=0; idx<nrversions; idx++ )
    {
	float* vals = new float[nrcells];
	if ( !vals )
	    { deepEraseArr( versiondata ); return; }

	OD::memValueSet( vals, mUdf(float), nrcells );
	versiondata += vals;
    }

    BinIDValueSet::SPos pos;
    const int startsourceidx = nrfixedcols + (nrfixedcols==sidcol ? 1 : 0);
    while ( data->next(pos,true) )
    {
	const float* ptr = data->getVals(pos);
	if ( sidcol!=-1 && sectionid!=mNINT32(ptr[sidcol]) )
	    continue;

	const BinID bid = data->getBinID( pos );
	if ( !rrg.includes(bid.inl(), false) ||
	    !crg.includes(bid.crl(),false) )
	    continue;

	if ( horsection_->userchangedisplayrg_ )
	{
	    if ( ( bid.inl()-rrg.start ) % rrg.step ||
		 ( bid.crl()-crg.start ) % crg.step )
		continue;
	}

	const int inlidx = rrg.nearestIndex( bid.inl() );
	const int crlidx = crg.nearestIndex( bid.crl() );

	const int offset = inlidx*nrcols + crlidx;
	if ( offset>=nrcells )
	    continue;

	for ( int idx=0; idx<nrversions; idx++ )
	    versiondata[idx][offset] = ptr[idx+startsourceidx];
    }

    for ( int idx=0; idx<nrversions; idx++ )
	channels_->setUnMappedData( channel, idx, versiondata[idx],
	OD::TakeOverPtr, 0 );
}


void HorizonTextureHandler::updateTileTextureOrigin()
{
    if ( !horsection_ )
	return;

    const float tilesize = horsection_->nrcoordspertileside_ - 1;

    for ( int idx=0; idx<horsection_->tiles_.info().getSize(0); idx++ )
    {
	for ( int idy=0; idy<horsection_->tiles_.info().getSize(1); idy++ )
	{
	    HorizonSectionTile* tile = horsection_->tiles_.get( idx, idy );
	    if ( !tile )
		continue;

	    Coord origin( tile->origin_.col(), tile->origin_.row() );
	    origin.x -= horsection_->texturecolrg_.start;
	    origin.y -= horsection_->texturerowrg_.start;

	    if ( horsection_->texturecolrg_.step )
		origin.x /= horsection_->texturecolrg_.step;
	    if ( horsection_->texturerowrg_.step )
		origin.y /= horsection_->texturerowrg_.step;

	    tile->setTexture( origin, origin+Coord(tilesize,tilesize) );
	}
    }
}


void HorizonTextureHandler::setHorizonSection( const HorizonSection& horsec )
{
    horsection_ = &horsec;
    updateTileTextureOrigin();
}

} // namespace visBase
