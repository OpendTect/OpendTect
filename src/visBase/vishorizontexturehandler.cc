/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vishorizontexturehandler.h"

#include "binidsurface.h"
#include "binidvalset.h"
#include "coltabmapper.h"
#include "datacoldef.h"
#include "datapointset.h"
#include "posvecdataset.h"
#include "threadwork.h"
#include "vishorizonsectiondef.h"
#include "vishorizonsectiontile.h"

#include <osgGeo/LayeredTexture>


namespace visBase
{

HorizonTextureHandler::HorizonTextureHandler( HorizonSection* horsection )
    : horsection_(horsection)
{
    ref();
    channels_ = TextureChannels::create();
    channel2rgba_ = ColTabTextureChannel2RGBA::create();
    cache_.setNullAllowed();
    channels_->setChannels2RGBA( channel2rgba_ );
    if ( channels_->nrChannels()<1 )
	addChannel();
    else
	cache_ += nullptr;

    channels_->getOsgTexture()->setTextureSizePolicy(
					osgGeo::LayeredTexture::AnySize );
    unRefNoDelete();
}


HorizonTextureHandler::~HorizonTextureHandler()
{
    deepErase( cache_ );
}


osg::Node* HorizonTextureHandler::getOsgNode()
{
    return channels_->osgNode();
}


const osgGeo::LayeredTexture* HorizonTextureHandler::getOsgTexture() const
{
    return channels_->getOsgTexture();
}


osgGeo::LayeredTexture*	HorizonTextureHandler::getOsgTexture()
{
    return channels_->getOsgTexture();
}


void HorizonTextureHandler::addChannel()
{
    channels_->addChannel();
    cache_ += nullptr;
    channel2rgba_->setEnabled( nrChannels()-1, true );
}


void HorizonTextureHandler::setColTabSequence( int channel,
					       const ColTab::Sequence& seq )
{
    if ( channel>=0 )
	channel2rgba_->setSequence( channel, seq );
}


const ColTab::Sequence*
HorizonTextureHandler::getColTabSequence( int channel ) const
{
    return channel<0 ? nullptr : channel2rgba_->getSequence( channel );
}


void HorizonTextureHandler::setColTabMapperSetup( int channel,
			    const ColTab::MapperSetup& mapper, TaskRunner* tr )
{
    if ( !channels_->validIdx((channel)) )
	return;

    const bool needsclip =
	    channels_->getColTabMapperSetup( channel, 0 ).needsReClip( mapper );
    channels_->setColTabMapperSetup( channel, mapper );
    channels_->reMapData( channel, !needsclip, tr );
}


const ColTab::MapperSetup*
HorizonTextureHandler::getColTabMapperSetup( int ch ) const
{
    return channels_->validIdx(ch) ?
	&channels_->getColTabMapperSetup( ch,activeVersion(ch) ) : nullptr;
}



int HorizonTextureHandler::nrChannels() const
{
    return channels_->nrChannels();
}


void HorizonTextureHandler::setChannels2RGBA( TextureChannel2RGBA* t )
{
    channels_->setChannels2RGBA( t );
    channel2rgba_ = t;
}


TextureChannel2RGBA* HorizonTextureHandler::getChannels2RGBA()
{
    return channel2rgba_.ptr();
}


const TextureChannel2RGBA* HorizonTextureHandler::getChannels2RGBA() const
{
    return channel2rgba_.ptr();
}


TextureChannels* HorizonTextureHandler::getChannels()
{
    return channels_.ptr();
}


const TextureChannels* HorizonTextureHandler::getChannels() const
{
    return channels_.ptr();
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
    mDynamicCastGet( ColTabTextureChannel2RGBA*, ct, channel2rgba_.ptr() );
    if ( ct && ch>=0 ) ct->setTransparency( ch, yn );
}


unsigned char HorizonTextureHandler::getTransparency( int ch ) const
{
    mDynamicCastGet( const ColTabTextureChannel2RGBA*, ct, channel2rgba_.ptr());
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
    ConstRefMan<HorizonSection> horsection = horsection_.get();
    if ( !horsection || !horsection->geometry_ )
	return;

    const BinIDValueSet* data = cache_.validIdx(channel) ? cache_[channel]
							 : nullptr;
    if ( !horsection->geometry_->getArray() || !dtpntset || !data )
	return;

    const int nrfixedcols = dtpntset->nrFixedCols();

    const char* hrsectionid = "Section ID";
    const DataColDef sidcoldef( hrsectionid );
    const int sidcol =
	dtpntset->dataSet().findColDef(sidcoldef,PosVecDataSet::NameExact);
    const int shift = sidcol==-1 ?  nrfixedcols : nrfixedcols+1;

    const int nrversions = data->nrVals()-shift;
    setNrVersions( channel, nrversions );

    const StepInterval<int> rrg = horsection->displayedRowRange();
    const StepInterval<int> crg = horsection->displayedColRange();
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

	if ( horsection->userchangedisplayrg_ )
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
				    OD::TakeOverPtr, nullptr );
}


void HorizonTextureHandler::updateTileTextureOrigin()
{
    ConstRefMan<HorizonSection> horsection = horsection_.get();
    if ( !horsection )
	return;

    const float tilesize = horsection->nrcoordspertileside_ - 1;

    for ( int idx=0; idx<horsection->tiles_.info().getSize(0); idx++ )
    {
	for ( int idy=0; idy<horsection->tiles_.info().getSize(1); idy++ )
	{
	    HorizonSectionTile* tile = horsection->tiles_.get( idx, idy );
	    if ( !tile )
		continue;

	    Coord origin( tile->origin_.col(), tile->origin_.row() );
	    origin.x -= horsection->texturecolrg_.start;
	    origin.y -= horsection->texturerowrg_.start;

	    if ( horsection->texturecolrg_.step )
		origin.x /= horsection->texturecolrg_.step;

	    if ( horsection->texturerowrg_.step )
		origin.y /= horsection->texturerowrg_.step;

	    tile->setTexture( origin, origin+Coord(tilesize,tilesize) );
	}
    }
}


void HorizonTextureHandler::setHorizonSection( const HorizonSection& horsec )
{
    horsection_ = &const_cast<HorizonSection&>( horsec );
    updateTileTextureOrigin();
}

} // namespace visBase
