/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visthumbwheel.h"

#include <osgGeo/ThumbWheel>

mCreateFactoryEntry( visBase::ThumbWheel );

#define col2f(rgb) float(col.rgb())/255

namespace visBase
{

class ThumbWheelMessenger : public osg::NodeCallback
{
public:
		ThumbWheelMessenger( ThumbWheel* t )
		    : visthumbwheel_( t )
		{}
    void	operator()(osg::Node* node, osg::NodeVisitor* nv ) override
		{
		    if ( visthumbwheel_ && nv )
		    {
			osgGeo::ThumbWheelEventNodeVisitor* thnv =
			    (osgGeo::ThumbWheelEventNodeVisitor*) nv;

			visthumbwheel_->rotation.trigger(
				    thnv->getDeltaAngle(), visthumbwheel_ );
		    }
		}
    void	detach() { visthumbwheel_ = nullptr; }

protected:
		ThumbWheelMessenger()
		{}

    ThumbWheel*	visthumbwheel_;
};



ThumbWheel::ThumbWheel()
    : rotation( this )
    , thumbwheel_( new osgGeo::ThumbWheel )
{
    setOsgNode( thumbwheel_ );
    thumbwheel_->ref();
    messenger_ = new ThumbWheelMessenger( this );
    messenger_->ref();
    thumbwheel_->addRotateCallback( messenger_ );
}


ThumbWheel::~ThumbWheel()
{
    messenger_->detach();
    messenger_->unref();
    thumbwheel_->unref();
}


void ThumbWheel::setPosition(bool horizontal, float x, float y, float len,
			     float width, float zval)
{
    const osg::Vec2 min( horizontal ? x-len/2 : x-width/2,
                        horizontal ? y-width/2 : y-len/2 );
    const osg::Vec2 max( horizontal ? x+len/2 : x+width/2,
			 horizontal ? y+width/2 : y+len/2 );

    thumbwheel_->setShape( horizontal ? 0 : 1, min, max, zval );
}


void ThumbWheel::enableFadeInOut( bool yn )
{ thumbwheel_->enableFadeInOut( yn ); }


bool ThumbWheel::isFadeInOutEnabled() const
{ return thumbwheel_->isFadeInOutEnabled(); }


void ThumbWheel::setAnnotationColor( const OD::Color& col )
{
    osg::Vec4 osgcol( col2f(r),col2f(g),col2f(b), 0.2 );
    thumbwheel_->setBorderColor( osgcol );
}


float ThumbWheel::getAngle() const
{ return thumbwheel_->getAngle(); }

void ThumbWheel::setAngle( float angle, float rotationtime )
{ thumbwheel_->setAngle( angle, rotationtime ); }

} // namespace visBase
