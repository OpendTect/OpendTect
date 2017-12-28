/*
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 AUTHOR   : K. Tingdahl
 DATE     : Oct 2008
___________________________________________________________________

-*/


#include "visrgbatexturechannel2rgba.h"

#include "vistexturechannels.h"
#include "coltabseqmgr.h"
#include "uistrings.h"

#include <osgGeo/LayeredTexture>

mCreateFactoryEntry( visBase::RGBATextureChannel2RGBA );

namespace visBase
{


RGBATextureChannel2RGBA::RGBATextureChannel2RGBA()
    : proc_(0)
    , proctransparency_(0)
{
    for ( int idx=0; idx<=3; idx++ )
	enabled_ += idx!=3;
}


const ColTab::Sequence& RGBATextureChannel2RGBA::getSequence( int ch ) const
{
    return ColTab::SeqMGR().getRGBBlendColSeq( ch );
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
	res = toUiString("%1 %2").arg(layername).arg(uiStrings::sLayer());
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
