/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Karthika
 * DATE     : Nov 2009
___________________________________________________________________

-*/

static const char* rcsID = "$Id: vistexturechannel2voldata.cc,v 1.6 2009-12-11 08:34:06 cvskarthika Exp $";

#include "vistexturechannel2voldata.h"
#include "envvars.h"

#include "SoTextureChannelSet.h"

#include <Inventor/nodes/SoGroup.h>
#include <VolumeViz/nodes/SoTransferFunction.h>
#include <VolumeViz/nodes/SoVolumeData.h>

mCreateFactoryEntry( visBase::TextureChannel2VolData );

namespace visBase
{

#define mNrColors 256


class VolumeDataSetImpl : public VolumeDataSet
{
public:

static VolumeDataSetImpl* create()
mCreateDataObj( VolumeDataSetImpl );

int nrChannels() const
{ return 1; }


void setNrChannels( int nr )
{ // to do
}


bool addChannel()
{ return true;  // to do: check 
}


bool enableNotify( bool yn )
{ return voldata_->enableNotify( yn ); }


void touch()
{ voldata_->touch(); }


void setChannelData( int channel,const SbImage& image )
{
    SbVec3s tmpsize;
    int bpp;
    unsigned char* data = image.getValue( tmpsize, bpp );
	    
    if ( data && ( bpp >=1 ) && (bpp <=2) )
    {
        SoVolumeData::DataType dt;
	if ( bpp == 1 )
	    dt = SoVolumeData::UNSIGNED_BYTE;
	else if ( dt == SoVolumeData::UNSIGNED_SHORT )
	    dt = SoVolumeData::UNSIGNED_SHORT;

	voldata_->setVolumeData( tmpsize, data, dt );
    }
}


const SbImage* getChannelData() const
{
    SbVec3s size;
    void* ptr;
    SoVolumeData::DataType dt;
    
    if ( voldata_->getVolumeData(size,ptr,dt) )
    {
        int bpp = 0;

	if ( dt == SoVolumeData::UNSIGNED_BYTE )
	    bpp = 1;
	else if ( dt == SoVolumeData::UNSIGNED_SHORT )
	    bpp = 2;
		
	if ( bpp )
	{
	    SbImage* image = new SbImage( (unsigned char*) ptr, size, bpp );
	    return image;
	}
    }
    
    return 0;
}


SoNode* getInventorNode()
{
    if ( !voldata_ )
    {
	voldata_ = new SoVolumeData;

	setVolumeSize( Interval<float>(-0.5,0.5), Interval<float>(-0.5,0.5),
		       Interval<float>(-0.5,0.5) );
	if ( GetEnvVarYN("DTECT_VOLREN_NO_PALETTED_TEXTURE") )
	    voldata_->usePalettedTexture = FALSE;
    }

    return voldata_;
}

protected:

~VolumeDataSetImpl()
{ 
    voldata_->unref(); 
}
	
};


VolumeDataSet::VolumeDataSet()
     : voldata_( new SoVolumeData )
{ 
    voldata_->ref();
    setVolumeSize( Interval<float>(-0.5,0.5), Interval<float>(-0.5,0.5),
 		   Interval<float>(-0.5,0.5) );
    if ( GetEnvVarYN("DTECT_VOLREN_NO_PALETTED_TEXTURE") )
	    voldata_->usePalettedTexture = FALSE;
}


void VolumeDataSet::setVolumeSize(  const Interval<float>& x,
						  const Interval<float>& y,
						  const Interval<float>& z )
{
    if ( !voldata_ )
	return;

    const SbBox3f size( x.start, y.start, z.start, x.stop, y.stop, z.stop );
    voldata_->setVolumeSize( size );
}


Interval<float> VolumeDataSet::getVolumeSize( int dim ) const
{
     const SbBox3f size = voldata_->getVolumeSize();
     return Interval<float>( size.getMin()[dim], size.getMax()[dim] );
}


mCreateFactoryEntry( VolumeDataSetImpl );

VolumeDataSetImpl::VolumeDataSetImpl()
{}


TextureChannel2VolData::TextureChannel2VolData()
    : transferfunc_( 0 )
    , enabled_ (false)
{
}


TextureChannel2VolData::~TextureChannel2VolData()
{
    if ( transferfunc_ )
	transferfunc_->unref(); 
}


MappedTextureDataSet* TextureChannel2VolData::createMappedDataSet() const
{ return VolumeDataSetImpl::create(); }


SoNode* TextureChannel2VolData::getInventorNode()
{
    enabled_ = true;
    transferfunc_ = new SoTransferFunction;
    transferfunc_->ref();	// to do: check if necessary
    makeColorTables();
    return transferfunc_;
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


void TextureChannel2VolData::setEnabled( int ch, bool yn )
{
    enabled_ = yn;
    update();
}


bool TextureChannel2VolData::isEnabled( int ch ) const
{
    return enabled_;
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

    const bool didnotify = transferfunc_->colorMap.enableNotify( false );

    //transferfunc_->predefColorMap = SoTransferFunction::SEISMIC;
    transferfunc_->predefColorMap = SoTransferFunction::NONE;
    //transferfunc_->colorMapType = SoTransferFunction::RGBA;

    const float redfactor = 1.0/255;
    const float greenfactor = 1.0/255;
    const float bluefactor = 1.0/255;
    const float opacityfactor = 1.0/255;

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
    
    transferfunc_->colorMap.enableNotify(didnotify);
    transferfunc_->colorMap.touch();
}

}; // namespace visBase

