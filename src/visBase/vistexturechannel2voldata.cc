/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Karthika
 * DATE     : Nov 2009
___________________________________________________________________

-*/

static const char* rcsID = "$Id: vistexturechannel2voldata.cc,v 1.1 2009-11-10 07:58:34 cvskarthika Exp $";

#include "vistexturechannel2voldata.h"
#include "envvars.h"

#include <Inventor/nodes/SoGroup.h>
#include <VolumeViz/nodes/SoTransferFunction.h>
#include <VolumeViz/nodes/SoVolumeData.h>

mCreateFactoryEntry( visBase::TextureChannel2VolData );

namespace visBase
{

#define mNrColors 256

TextureChannel2VolData::TextureChannel2VolData()
    : transferfunc_( 0 )
    , voldata_( 0 )
    , root_( new SoGroup )
    , dummytexture_(255)
{
    root_->ref();
}


TextureChannel2VolData::~TextureChannel2VolData()
{
    if ( voldata_ )
	voldata_->unref();
    
    if ( transferfunc_ )
	transferfunc_->unref();

    root_->unref();
}


SoNode* TextureChannel2VolData::getInventorNode()
{
    if ( !voldata_ )
    {
	voldata_ = new SoVolumeData;
	voldata_->ref();	// to do: check if necessary
	root_->addChild( voldata_ );

	setVolumeSize( Interval<float>(-0.5,0.5), Interval<float>(-0.5,0.5),
		Interval<float>(-0.5,0.5) );
	voldata_->setVolumeData( SbVec3s(1,1,1), &dummytexture_, 
		SoVolumeData::UNSIGNED_BYTE );
	if ( GetEnvVarYN("DTECT_VOLREN_NO_PALETTED_TEXTURE") )
	    voldata_->usePalettedTexture = FALSE;

	transferfunc_ = new SoTransferFunction;
	transferfunc_->ref();	// to do: check if necessary
	makeColorTables();

	root_->addChild( transferfunc_ );
    }

    return root_;
}


void TextureChannel2VolData::setVolumeSize( const Interval<float>& x,
	const Interval<float>& y, const Interval<float>& z )
{
    if ( !voldata_ )
	return;

    const SbBox3f size( x.start, y.start, z.start, x.stop, y.stop, z.stop );
    voldata_->setVolumeSize( size );
}


Interval<float> TextureChannel2VolData::getVolumeSize( int dim ) const
{
    const SbBox3f size = voldata_->getVolumeSize();
    return Interval<float>( size.getMin()[dim], size.getMax()[dim] );
}


void TextureChannel2VolData::setChannels( TextureChannels* texch )
{
    TextureChannel2RGBA::setChannels( texch );
}


void TextureChannel2VolData::setSequence( int channel,
					  const ColTab::Sequence& seq )
{
    // Only 1 channel supported now.
    if ( ( channel < 0 ) || ( channel >= maxNrChannels() ) )
	return;

    if ( sequence_ == seq )
	return;
    
    sequence_ = seq;
    update();
}


const ColTab::Sequence* TextureChannel2VolData::getSequence( int channel ) const
{
    // Only 1 channel supported now.
    if ( ( channel < 0 ) || ( channel >= maxNrChannels() ) )
	return 0;

    return &sequence_;  // to do: use PtrMan?
}


void TextureChannel2VolData::setEnabled( bool yn )
{
    if ( !voldata_ ) return;
    if ( !yn )
	voldata_->setVolumeData( SbVec3s(1,1,1),
		&dummytexture_, SoVolumeData::UNSIGNED_BYTE );
    /*else if ( indexcache_ )
	voldata_->setVolumeData( SbVec3s(sz2_,sz1_,sz0_),
		indexcache_, SoVolumeData::UNSIGNED_BYTE );*/
}


bool TextureChannel2VolData::isEnabled() const
{
    if ( !voldata_ )
	return false;

    SbVec3s size;
    void* ptr;
    SoVolumeData::DataType dt;

    return voldata_->getVolumeData(size,ptr,dt) /*&& ptr==indexcache_*/;
}


void TextureChannel2VolData::notifyChannelChange()
{
    update();
}


void TextureChannel2VolData::update()
{
    makeColorTables();
}


void TextureChannel2VolData::makeColorTables()
{
    if ( !transferfunc_ )
	return;

    /*const float redfactor = (float) blendcolor_.r()/(255*255);
    const float greenfactor = (float) blendcolor_.g()/(255*255);
    const float bluefactor = (float) blendcolor_.b()/(255*255);
    const float opacityfactor = (float) (255-blendcolor_.t())/(255*255);
*/
    const float redfactor = 1.0;
    const float greenfactor = 1.0;
    const float bluefactor = 1.0;
    const float opacityfactor = 0;

    const bool didnotify = transferfunc_->colorMap.enableNotify( false );
    int cti = 0;
    for ( int idx=0; idx<mNrColors; idx++ )
    {
	const float relval = ((float) idx)/(mNrColors-2);
	const ::Color col = sequence_.color( relval );
	transferfunc_->colorMap.set1Value( cti++, col.r()*redfactor );
	transferfunc_->colorMap.set1Value( cti++, col.g()*greenfactor );
	transferfunc_->colorMap.set1Value( cti++, col.b()*bluefactor );
	transferfunc_->colorMap.set1Value( cti++, 1.0-col.t()*opacityfactor );
    }

    transferfunc_->predefColorMap = SoTransferFunction::NONE;

    transferfunc_->colorMap.enableNotify(didnotify);
    transferfunc_->colorMap.touch();
}

}; // namespace visBase
