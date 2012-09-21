/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Karthika
 * DATE     : Nov 2009
___________________________________________________________________

-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "vistexturechannel2voldata.h"
#include "envvars.h"

#include "SoTextureChannelSet.h"

#include <Inventor/nodes/SoGroup.h>
#include <VolumeViz/nodes/SoTransferFunction.h>
#include <VolumeViz/nodes/SoVolumeData.h>

#include <limits.h>

mCreateFactoryEntry( visBase::TextureChannel2VolData );

namespace visBase
{

#define mNrColors 256


/*!A destination where the texturechannels can put the mapped data. The class
   instanciation is provided by the TextureChannel2VolData. */

class VolumeDataSet : public MappedTextureDataSet
{
public:

VolumeDataSet() : voldata_( new SoVolumeData )
		, dummytexture_( 255 )
{ 
    voldata_->ref();
    setVolumeSize( Interval<float>(-0.5,0.5), Interval<float>(-0.5,0.5),
 		   Interval<float>(-0.5,0.5) );
    voldata_->setVolumeData( SbVec3s(1,1,1), &dummytexture_, 
		    SoVolumeData::UNSIGNED_BYTE );
    if ( GetEnvVarYN("DTECT_VOLREN_NO_PALETTED_TEXTURE") )
	voldata_->usePalettedTexture = FALSE;
}


void setVolumeSize( const Interval<float>& x,
		    const Interval<float>& y,
		    const Interval<float>& z )
{
    if ( !voldata_ )
	return;

    const SbBox3f size( x.start, y.start, z.start, x.stop, y.stop, z.stop );
    voldata_->setVolumeSize( size );
}


Interval<float> getVolumeSize( int dim ) const
{
     const SbBox3f size = voldata_->getVolumeSize();
     return Interval<float>( size.getMin()[dim], size.getMax()[dim] );
}

protected:

~VolumeDataSet()
{
    voldata_->unref();
}

	SoVolumeData*		voldata_;
	unsigned char* 		datacache_;
	unsigned char		dummytexture_;
    
};


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


void setChannelData( int channel,const SbImagei32& image )
{
    SbVec3i32 size;
    SbVec3s tmpsize;
    int bpp;
    unsigned char* data = image.getValue( size, bpp );

    if ( size[0] > SHRT_MAX || size[1] > SHRT_MAX || size[2] > SHRT_MAX )
    {
	pErrMsg( "Image is too large to fit the volume!" );
	return;
    }

    tmpsize = SbVec3s( size[0], size[1], size[2] );

    if ( data && ( bpp >=1 ) && (bpp <=2) )
    {
	SoVolumeData::DataType dt;
	dt = ( bpp == 1 ) ? SoVolumeData::UNSIGNED_BYTE :
		SoVolumeData::UNSIGNED_SHORT;
	voldata_->setVolumeData( tmpsize, data, dt );
    }
}


const SbImagei32* getChannelData() const
{
    SbVec3s size;
    SbVec3i32 tmpsize;
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
	    tmpsize[0] = size[0];
	    tmpsize[1] = size[1];
	    tmpsize[2] = size[2];
	    return new SbImagei32( (unsigned char*) ptr, tmpsize, bpp );
	}
    }
    
    return 0;
}


SoNode* gtInvntrNode()
{
    if ( !voldata_ )
    {
	voldata_ = new SoVolumeData;

	setVolumeSize( Interval<float>(-0.5,0.5), Interval<float>(-0.5,0.5),
		       Interval<float>(-0.5,0.5) );
	voldata_->setVolumeData( SbVec3s(1,1,1),
	    		    &dummytexture_, SoVolumeData::UNSIGNED_BYTE );
	if ( GetEnvVarYN("DTECT_VOLREN_NO_PALETTED_TEXTURE") )
	    voldata_->usePalettedTexture = FALSE;
    }

    return voldata_;
}

protected:

~VolumeDataSetImpl()
{ 
}
	
};



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


SoNode* TextureChannel2VolData::gtInvntrNode()
{
    enabled_ = true;
    transferfunc_ = new SoTransferFunction;
    transferfunc_->ref();
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

    return &sequence_;
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

    transferfunc_->predefColorMap = SoTransferFunction::NONE;
    
    const float redfactor = 1.0/255;
    const float greenfactor = 1.0/255;
    const float bluefactor = 1.0/255;
    const float opacityfactor = 1.0/255;

    int cti = 0;
    for ( int idx=0; idx<mNrColors-1; idx++ )
    {
	const float relval = ((float) idx)/(mNrColors-2);
	const ::Color col = sequence_.color( relval );
	transferfunc_->colorMap.set1Value( cti++, col.r()*redfactor );
	transferfunc_->colorMap.set1Value( cti++, col.g()*greenfactor );
	transferfunc_->colorMap.set1Value( cti++, col.b()*bluefactor );
	transferfunc_->colorMap.set1Value( cti++, 1.0f-col.t()*opacityfactor );
    }
    
    const ::Color col = sequence_.undefColor();
    transferfunc_->colorMap.set1Value( cti++, col.r()*redfactor );
    transferfunc_->colorMap.set1Value( cti++, col.g()*greenfactor );
    transferfunc_->colorMap.set1Value( cti++, col.b()*bluefactor );
    transferfunc_->colorMap.set1Value( cti++, 1.0f-col.t()*opacityfactor );
    
    transferfunc_->colorMap.enableNotify(didnotify);
    transferfunc_->colorMap.touch();
}

}; // namespace visBase

