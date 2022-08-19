/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visrgbatexturechannel2rgba.h"

#include "vistexturechannels.h"
#include "coltabsequence.h"
#include "uistrings.h"

#include <osgGeo/LayeredTexture>

mCreateFactoryEntry( visBase::RGBATextureChannel2RGBA );

namespace visBase
{

ArrPtrMan<ColTab::Sequence> RGBATextureChannel2RGBA::sequences_ = 0;

RGBATextureChannel2RGBA::RGBATextureChannel2RGBA()
    : proc_(0)
    , proctransparency_(0)
{
    if ( !sequences_ )
    {
	sequences_ = new ColTab::Sequence[4];
	sequences_[0].setType(ColTab::Sequence::User);
	sequences_[0].setColor( 0, 0, 0, 0 );
	sequences_[0].setColor( 1, 255, 0, 0 );
	sequences_[0].setName( "Red" );

	sequences_[1].setType(ColTab::Sequence::User);
	sequences_[1].setColor( 0, 0, 0, 0 );
	sequences_[1].setColor( 1, 0, 255, 0 );
	sequences_[1].setName( "Green" );

	sequences_[2].setType(ColTab::Sequence::User);
	sequences_[2].setColor( 0, 0, 0, 0 );
	sequences_[2].setColor( 1, 0, 0, 255 );
	sequences_[2].setName( "Blue" );

	sequences_[3].setType(ColTab::Sequence::User);
	sequences_[3].setColor( 0, 0, 0, 0 );
	sequences_[3].setColor( 1, 255,255,255 );
	sequences_[3].setTransparency( Geom::Point2D<float>(0,0) );
	sequences_[3].setTransparency( Geom::Point2D<float>(1,255) );
	sequences_[3].setName( "Alpha" );
    }

    for ( int idx=0; idx<=3; idx++ )
	enabled_ += idx!=3;
}


const ColTab::Sequence* RGBATextureChannel2RGBA::getSequence( int ch ) const
{
    return (sequences_ && ch>=0 && ch<=3) ? &sequences_[ch] : 0;
}


void RGBATextureChannel2RGBA::getChannelName( int channel,
					      uiString& res ) const
{
    uiString layername;
    switch ( channel ) {
	case 0:
	    layername = uiStrings::sRed();
	    break;
	case 1:
	    layername = uiStrings::sGreen();
	    break;
	case 2:
	    layername = uiStrings::sBlue();
	    break;
	case 3:
	    layername = uiStrings::sAlpha();
	    break;
	default:
	    res.setEmpty();
	    break;
    }

    if ( !layername.isEmpty() )
	res = uiStrings::phrJoinStrings( layername, uiStrings::sLayer() );
}


RGBATextureChannel2RGBA::~RGBATextureChannel2RGBA()
{
    if ( proc_ ) proc_->unref();
}


void RGBATextureChannel2RGBA::notifyChannelInsert( int channel )
{
    if ( !laytex_ || proc_ || channel!=3 )
	return;

    proc_ = new osgGeo::RGBALayerProcess( *laytex_ );
    proc_->ref();
    proc_->applyUndefPerChannel( true );

    laytex_->addProcess( proc_ );

    for ( int idx=0; idx<=3; idx++ )
    {
	const int layerid = (*channels_->getOsgIDs(idx))[0];
	proc_->setDataLayerID( idx, layerid );
	setEnabled( idx, enabled_[idx] );
    }

    setTransparency( proctransparency_ );
}


void RGBATextureChannel2RGBA::swapChannels( int ch0, int ch1 )
{
    if ( ch0>=0 && ch0<=3 && ch1>=0 && ch1<=3 )
    {
	const bool wason0 = isEnabled( ch0 );
	setEnabled( ch0, isEnabled(ch1) );
	setEnabled( ch1, wason0 );

	if ( proc_ )
	{
	    const int oldid0 = proc_->getDataLayerID( ch0 );
	    proc_->setDataLayerID( ch0, proc_->getDataLayerID(ch1) );
	    proc_->setDataLayerID( ch1, oldid0 );
	}
    }
}


void RGBATextureChannel2RGBA::setEnabled( int channel, bool yn )
{
    if ( enabled_.validIdx(channel) )
    {
	enabled_[channel] = yn;

	if ( proc_ )
	    proc_->turnOn( channel, yn );
    }
}


bool RGBATextureChannel2RGBA::isEnabled( int channel ) const
{
    if ( enabled_.validIdx(channel) )
	return enabled_[channel];

    return false;
}


void RGBATextureChannel2RGBA::setTransparency( unsigned char transparency )
{
    proctransparency_ = transparency;

    if ( proc_ )
	proc_->setOpacity( 1.0f - mCast(float,transparency)/255.0f );
}


unsigned char RGBATextureChannel2RGBA::getTransparency() const
{
    return proctransparency_;
}


void RGBATextureChannel2RGBA::applyUndefPerChannel( bool yn )
{
    if ( proc_ && isUndefPerChannel()!=yn )
	proc_->applyUndefPerChannel( yn );
}


bool RGBATextureChannel2RGBA::isUndefPerChannel() const
{
    return proc_ ? proc_->isUndefPerChannel() : true;
}


} // namespace visBase
