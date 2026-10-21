/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visvsgthumbwheel.h"

#include "visvsgdataman.h"
#include "vsggeothumbwheel.h"

mCreateDGBVisFactoryEntry( dGBVis::ThumbWheel );

namespace dGBVis
{

ThumbWheel::ThumbWheel()
    : rotation(this)
    , wheel_(vsgGeo::ThumbWheel::create())
{
    wheel_->setRotationCallback( [this]( float delta )
				 { rotation.trigger( delta ); } );
    setVsgNode( wheel_.get() );
}


ThumbWheel::~ThumbWheel()
{
    wheel_->setRotationCallback( nullptr );
}


void ThumbWheel::setPosition( bool horizontal, float x, float y, float len,
			      float width, float zval )
{
    const vsg::vec2 min( horizontal ? x-len/2 : x-width/2,
			 horizontal ? y-width/2 : y-len/2 );
    const vsg::vec2 max( horizontal ? x+len/2 : x+width/2,
			 horizontal ? y+width/2 : y+len/2 );
    wheel_->setShape( horizontal, min, max, zval );
    requestSingleRedraw();
}


void ThumbWheel::setAnnotationColor( const OD::Color& col )
{
    wheel_->setBorderColor( toVec4(col,0.2f) );
    requestSingleRedraw();
}


void ThumbWheel::enableFadeInOut( bool yn )
{
    wheel_->enableFadeInOut( yn );
    requestSingleRedraw();
}


bool ThumbWheel::isFadeInOutEnabled() const
{
    return wheel_->isFadeInOutEnabled();
}


void ThumbWheel::setGrowOnHover( bool yn )
{
    wheel_->setGrowOnHover( yn );
    requestSingleRedraw();
}


bool ThumbWheel::isGrowOnHover() const
{
    return wheel_->isGrowOnHover();
}


void ThumbWheel::setGrowDirection( int dir )
{
    wheel_->setGrowDirection( dir );
    requestSingleRedraw();
}


void ThumbWheel::setGlassLook( bool yn )
{
    wheel_->setStyle( yn ? vsgGeo::ThumbWheel::Style::Glass
			 : vsgGeo::ThumbWheel::Style::Classic );
    requestSingleRedraw();
}


bool ThumbWheel::isGlassLook() const
{
    return wheel_->style() == vsgGeo::ThumbWheel::Style::Glass;
}


float ThumbWheel::getAngle() const
{
    return wheel_->getAngle();
}


void ThumbWheel::setAngle( float angle, float rotationtime )
{
    wheel_->setAngle( angle, rotationtime );
    requestSingleRedraw();
}

} // namespace dGBVis
