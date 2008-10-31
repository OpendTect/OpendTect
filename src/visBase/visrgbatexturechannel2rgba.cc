/*
___________________________________________________________________

 COPYRIGHT: (C) dGB Beheer B.V.
 AUTHOR   : K. Tingdahl
 DATE     : Oct 2008
___________________________________________________________________

-*/

static const char* rcsID = "$Id: visrgbatexturechannel2rgba.cc,v 1.1 2008-10-31 18:03:36 cvskris Exp $";

#include "visrgbatexturechannel2rgba.h"

#include "vistexturechannels.h"
#include "SoRGBATextureChannel2RGBA.h"

mCreateFactoryEntry( visBase::RGBATextureChannel2RGBA );

namespace visBase
{

RGBATextureChannel2RGBA::RGBATextureChannel2RGBA()
    : converter_( new SoRGBATextureChannel2RGBA )
{
    converter_->ref();
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
