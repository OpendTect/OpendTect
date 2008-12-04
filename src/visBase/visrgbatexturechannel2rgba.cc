/*
___________________________________________________________________

 COPYRIGHT: (C) dGB Beheer B.V.
 AUTHOR   : K. Tingdahl
 DATE     : Oct 2008
___________________________________________________________________

-*/

static const char* rcsID = "$Id: visrgbatexturechannel2rgba.cc,v 1.2 2008-12-04 13:45:59 cvskris Exp $";

#include "visrgbatexturechannel2rgba.h"

#include "vistexturechannels.h"
#include "SoRGBATextureChannel2RGBA.h"

mCreateFactoryEntry( visBase::RGBATextureChannel2RGBA );

namespace visBase
{

RGBATextureChannel2RGBA::RGBATextureChannel2RGBA()
    : converter_( new SoRGBATextureChannel2RGBA )
{
    sequences_[0].setType(ColTab::Sequence::User);
    sequences_[0].setColor( 0, 255, 0, 0 );
    sequences_[0].setColor( 1, 255, 0, 0 );

    sequences_[1].setType(ColTab::Sequence::User);
    sequences_[1].setColor( 0, 0, 255, 0 );
    sequences_[1].setColor( 1, 0, 255, 0 );

    sequences_[2].setType(ColTab::Sequence::User);
    sequences_[2].setColor( 0, 0, 0, 255 );
    sequences_[2].setColor( 1, 0, 0, 255 );

    sequences_[3].setType(ColTab::Sequence::User);
    sequences_[3].setColor( 0, 0, 0, 0 );
    sequences_[3].setColor( 1, 0, 0, 0 );
    sequences_[3].setTransparency( Geom::Point2D<float>(0,1) );
    sequences_[3].setTransparency( Geom::Point2D<float>(1,1) );

    converter_->ref();
}


const ColTab::Sequence* RGBATextureChannel2RGBA::getSequence( int ch ) const
{
    return &sequences_[ch];
}


RGBATextureChannel2RGBA::~RGBATextureChannel2RGBA()
{ converter_->unref(); }


SoNode* RGBATextureChannel2RGBA::getInventorNode()
{ return converter_; }



void RGBATextureChannel2RGBA::setEnabled( int ch, bool yn )
{
    converter_->enabled.set1Value( ch, yn );
}


bool RGBATextureChannel2RGBA::isEnabled( int ch ) const
{
    return ch>=converter_->enabled.getNum() ? false : converter_->enabled[ch];
}


bool RGBATextureChannel2RGBA::createRGBA( SbImage& res ) const
{
    return false;
}



}; // namespace visBase
