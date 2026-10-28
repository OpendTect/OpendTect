/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"

#include "visvsgtexturechannels.h"
#include "vsggeolayeredtexture.h"

#include "coltabsequence.h"
#include "valseriesimpl.h"

namespace
{

constexpr int cWidth = 4;
constexpr int cHeight = 2;
constexpr unsigned char cUdfIdx = 255;

const float cValues[cWidth*cHeight] =
	{ 0.f, 2.5f, 5.f, 10.f,
	  -4.f, 25.f, mUdf(float), 7.5f };

ColTab::MapperSetup fixedSetup( bool flip=false )
{
    ColTab::MapperSetup setup;
    setup.type_ = ColTab::MapperSetup::Fixed;
    setup.range_ = Interval<float>( 0.f, 10.f );
    setup.flipseq_ = flip;
    return setup;
}


class ChangeCounter : public CallBacker
{
public:
		ChangeCounter( dGBVis::TextureChannels& channels )
		{ mAttachCB( channels.colTabChanged, ChangeCounter::changedCB ); }
		~ChangeCounter()	{ detachAllNotifiers(); }

    void	changedCB( CallBacker* )	{ nrchanges_++; }

    int		nrchanges_ = 0;
};


RefMan<dGBVis::TextureChannels> makeChannels()
{
    RefMan<dGBVis::TextureChannels> channels = dGBVis::TextureChannels::create();
    channels->setSize( 0, 1, cHeight, cWidth );
    channels->setColTabMapperSetup( 0, fixedSetup() );
    channels->setUnMappedData( 0, 0, cValues, OD::UsePtr, nullptr );
    return channels;
}

} // namespace


static bool testSize()
{
    RefMan<dGBVis::TextureChannels> channels = dGBVis::TextureChannels::create();
    mRunStandardTest( channels->nrTiles()==0, "Channels: No size, no tiles" );
    channels->setSize( 0, 1, cHeight, cWidth );
    mRunStandardTest( channels->nrTiles()==1, "Channels: A small image is one "
		      "tile" );
    vsgGeo::LayeredTexture* texture = channels->tile(0).texture_.get();
    mRunStandardTest( texture->width()==cWidth &&
		      texture->height()==cHeight,
		      "Channels: The size gives the image size" );
    mRunStandardTest( channels->getSize(0,0)==1 &&
		      channels->getSize(0,1)==cHeight &&
		      channels->getSize(0,2)==cWidth, "Channels: Sizes kept" );
    mRunStandardTest( texture->isUndefAt(0,0),
		      "Channels: Without data all is undefined" );
    return true;
}


static bool testMapping()
{
    RefMan<dGBVis::TextureChannels> channels = makeChannels();
    vsgGeo::LayeredTexture* texture = channels->tile(0).texture_.get();
    mRunStandardTest( texture->indexAt(0,0)==0 && texture->indexAt(1,0)==63 &&
		      texture->indexAt(2,0)==127 && texture->indexAt(3,0)==254,
		      "Channels: The first row is mapped" );
    mRunStandardTest( texture->indexAt(0,1)==0 && texture->indexAt(1,1)==254 &&
		      texture->isUndefAt(2,1) && !texture->isUndefAt(1,1) &&
		      texture->indexAt(3,1)==191,
		      "Channels: The second row is mapped, clipped and udf" );

    const TypeSet<float>* histogram = channels->getHistogram( 0 );
    mRunStandardTest( histogram && histogram->size()==255 &&
		      mIsEqual((*histogram)[0],1.f,1e-6f) &&
		      mIsEqual((*histogram)[254],1.f,1e-6f) &&
		      mIsEqual((*histogram)[63],0.5f,1e-6f) &&
		      mIsZero((*histogram)[10],1e-6f),
		      "Channels: The histogram is relative to its largest bin" );
    return true;
}


static bool testRemap()
{
    RefMan<dGBVis::TextureChannels> channels = makeChannels();
    channels->setColTabMapperSetup( 0, fixedSetup(true) );
    vsgGeo::LayeredTexture* texture = channels->tile(0).texture_.get();
    mRunStandardTest( texture->indexAt(0,0)==0,
		      "Channels: A new setup waits for the remap" );
    channels->reMapData( 0, true, nullptr );
    mRunStandardTest( texture->indexAt(0,0)==254 &&
		      texture->indexAt(3,0)==0 &&
		      texture->isUndefAt(2,1),
		      "Channels: Flipping reverses the indices" );
    return true;
}


static bool testSequence()
{
    RefMan<dGBVis::TextureChannels> channels = makeChannels();
    ColTab::Sequence seq;
    seq.setColor( 0.f, 255, 0, 0 );
    seq.setColor( 1.f, 0, 0, 255 );
    channels->setColTabSequence( seq );
    mRunStandardTest( channels->getColTabSequence() == seq,
		      "Channels: The sequence is kept" );
    mRunStandardTest( channels->tile(0).texture_->indexAt(3,0)==254,
		      "Channels: A new sequence leaves the indices alone" );
    return true;
}


static bool testColTabChanges()
{
    RefMan<dGBVis::TextureChannels> channels = makeChannels();
    ChangeCounter counter( *channels );
    ColTab::Sequence seq;
    seq.setColor( 0.f, 255, 0, 0 );
    seq.setColor( 1.f, 0, 0, 255 );
    channels->setColTabSequence( seq );
    mRunStandardTest( counter.nrchanges_==1,
		      "Channels: A new sequence is signalled" );

    channels->setColTabMapperSetup( 0, fixedSetup(true) );
    mRunStandardTest( counter.nrchanges_==1,
		      "Channels: A new setup is signalled with the remap" );
    channels->reMapData( 0, true, nullptr );
    mRunStandardTest( counter.nrchanges_==2,
		      "Channels: A remap signals the new range" );
    return true;
}


static bool testTiles()
{
    const int oldmax = vsgGeo::LayeredTexture::maxTextureSize();
    vsgGeo::LayeredTexture::setMaxTextureSize( 5 );
    constexpr int width = 8;
    constexpr int height = 7;
    TypeSet<float> values;
    for ( int idy=0; idy<height; idy++ )
    {
	for ( int idx=0; idx<width; idx++ )
	    values += float( idy*width + idx );
    }

    RefMan<dGBVis::TextureChannels> channels = dGBVis::TextureChannels::create();
    channels->setSize( 0, 1, height, width );
    ColTab::MapperSetup setup;
    setup.type_ = ColTab::MapperSetup::Fixed;
    setup.range_ = Interval<float>( 0.f, float(width*height-1) );
    channels->setColTabMapperSetup( 0, setup );
    channels->setUnMappedData( 0, 0, values.arr(), OD::UsePtr, nullptr );
    vsgGeo::LayeredTexture::setMaxTextureSize( oldmax );

    mRunStandardTest( channels->nrTiles()==9, "Tiles: 3 by 3 tiles" );
    const dGBVis::TextureChannels::Tile& last = channels->tile( 8 );
    mRunStandardTest( last.x0_==6 && last.y0_==6 &&
		      last.nrx_==2 && last.nry_==1 &&
		      last.overx_==1 && last.overy_==1 &&
		      last.texture_->width()==3 && last.texture_->height()==2,
		      "Tiles: The last tile is the corner that is left, with "
		      "the cells before it" );

    const dGBVis::TextureChannels::Tile& second = channels->tile( 1 );
    mRunStandardTest( second.x0_==3 && second.y0_==0 &&
		      second.nrx_==3 && second.nry_==3 &&
		      second.overx_==1 && second.overy_==0 &&
		      second.texture_->width()==5 &&
		      second.texture_->height()==4, "Tiles: The second tile "
		      "is the next one in the first row, with cells of its "
		      "neighbours on both sides" );

    bool allok = true;
    const ColTab::Mapper& mapper = channels->getColTabMapper( 0, 0 );
    for ( int itile=0; itile<channels->nrTiles(); itile++ )
    {
	const dGBVis::TextureChannels::Tile& tile = channels->tile( itile );
	for ( int idy=0; idy<tile.texture_->height(); idy++ )
	{
	    for ( int idx=0; idx<tile.texture_->width(); idx++ )
	    {
		const float val = values[ (tile.y0_-tile.overy_+idy)*width +
					  tile.x0_-tile.overx_+idx ];
		const int expected = ColTab::Mapper::snappedPosition(
			&mapper, val, dGBVis::ColTabMapping::nrSteps(),
			dGBVis::ColTabMapping::nrSteps() );
		if ( std::abs(int(tile.texture_->indexAt(idx,idy))-expected)>1 )
		    allok = false;
	    }
	}
    }

    mRunStandardTest( allok, "Tiles: Every cell is in the right tile, also "
			     "those of the neighbours" );
    return true;
}


static bool testValueSeries()
{
    RefMan<dGBVis::TextureChannels> channels = dGBVis::TextureChannels::create();
    channels->setSize( 0, 1, cHeight, cWidth );
    channels->setColTabMapperSetup( 0, fixedSetup() );

    ArrayValueSeries<float,float> series( cWidth*cHeight );
    for ( int idx=0; idx<cWidth*cHeight; idx++ )
	series.setValue( idx, cValues[idx] );

    mRunStandardTest( channels->setUnMappedVSData(0,0,&series,OD::UsePtr,
						  nullptr),
		      "Channels: A value series is accepted" );
    vsgGeo::LayeredTexture* texture = channels->tile(0).texture_.get();
    mRunStandardTest( texture->indexAt(2,0)==127 && texture->isUndefAt(2,1),
		      "Channels: A value series is mapped like an array" );
    return true;
}


static bool testMappedData()
{
    RefMan<dGBVis::TextureChannels> channels = dGBVis::TextureChannels::create();
    channels->setSize( 0, 1, cHeight, cWidth );
    mRunStandardTest( !channels->isCurrentDataPremapped(0),
		      "Channels: Data is not premapped at first" );

    TypeSet<unsigned char> bands;
    for ( int idx=0; idx<cWidth*cHeight; idx++ )
    {
	const bool udf = idx==5;
	bands += udf ? cUdfIdx : (unsigned char)(idx*10);
	bands += udf ? 0 : 255;
    }

    mRunStandardTest( channels->setMappedData(0,0,bands.arr(),OD::UsePtr),
		      "Channels: Mapped data is accepted" );
    vsgGeo::LayeredTexture* texture = channels->tile(0).texture_.get();
    mRunStandardTest( channels->isCurrentDataPremapped(0) &&
		      texture->indexAt(3,0)==30 && texture->indexAt(0,1)==40 &&
		      texture->indexAt(2,1)==60 &&
		      texture->isUndefAt(1,1) && !texture->isUndefAt(0,1),
		      "Channels: Mapped data is shown as it is" );
    mRunStandardTest( !channels->getHistogram(0),
		      "Channels: Mapped data has no histogram" );

    channels->setUnMappedData( 0, 0, cValues, OD::UsePtr, nullptr );
    mRunStandardTest( !channels->isCurrentDataPremapped(0),
		      "Channels: New data to map replaces mapped data" );
    return true;
}


static bool testInterpolation()
{
    RefMan<dGBVis::TextureChannels> channels = makeChannels();
    mRunStandardTest( channels->textureInterpolationEnabled() &&
		      channels->tile(0).texture_->isInterpolated(),
		      "Channels: The textures are interpolated by default" );

    ChangeCounter counter( *channels );
    channels->enableTextureInterpolation( false );
    vsgGeo::LayeredTexture* texture = channels->tile(0).texture_.get();
    mRunStandardTest( !channels->textureInterpolationEnabled() &&
		      !texture->isInterpolated(),
		      "Channels: Interpolation is switched off" );
    mRunStandardTest( texture->indexAt(1,0)==63 && texture->isUndefAt(2,1),
		      "Channels: The data is kept when the textures are "
		      "made again" );
    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testSize() ||
	 !testMapping() ||
	 !testRemap() ||
	 !testSequence() ||
	 !testColTabChanges() ||
	 !testTiles() ||
	 !testValueSeries() ||
	 !testMappedData() ||
	 !testInterpolation() )
	return 1;

    return 0;
}
