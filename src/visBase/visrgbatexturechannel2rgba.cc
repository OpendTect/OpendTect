/*
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 AUTHOR   : K. Tingdahl
 DATE     : Oct 2008
___________________________________________________________________

-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "visrgbatexturechannel2rgba.h"

#include "vistexturechannels.h"
#include "coltabsequence.h"
#include "SoRGBATextureChannel2RGBA.h"

mCreateFactoryEntry( visBase::RGBATextureChannel2RGBA );

namespace visBase
{

ArrPtrMan<ColTab::Sequence> RGBATextureChannel2RGBA::sequences_ = 0;


RGBATextureChannel2RGBA::RGBATextureChannel2RGBA()
    : converter_( new SoRGBATextureChannel2RGBA )
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
	sequences_[3].setColor( 1, 0, 0, 0 );
	sequences_[3].setTransparency( Geom::Point2D<float>(0,0) );
	sequences_[3].setTransparency( Geom::Point2D<float>(1,255) );
	sequences_[3].setName( "Transparency" );
    }

    converter_->ref();
}


const ColTab::Sequence* RGBATextureChannel2RGBA::getSequence( int ch ) const
{
    return &sequences_[ch];
}


RGBATextureChannel2RGBA::~RGBATextureChannel2RGBA()
{ converter_->unref(); }


SoNode* RGBATextureChannel2RGBA::gtInvntrNode()
{ return converter_; }



void RGBATextureChannel2RGBA::setEnabled( int ch, bool yn )
{
    converter_->enabled.set1Value( ch, yn );
}


bool RGBATextureChannel2RGBA::isEnabled( int ch ) const
{
    return ch>=converter_->enabled.getNum() ? false : converter_->enabled[ch];
}


bool RGBATextureChannel2RGBA::createRGBA( SbImagei32& res ) const
{
    return false;
}



}; // namespace visBase
