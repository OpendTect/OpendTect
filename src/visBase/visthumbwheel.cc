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
		    RefMan<ThumbWheel> visthumbwheel = visthumbwheel_.get();
		    if ( visthumbwheel && nv )
		    {
			osgGeo::ThumbWheelEventNodeVisitor* thnv =
			    (osgGeo::ThumbWheelEventNodeVisitor*) nv;

			visthumbwheel->rotation.trigger(
				thnv->getDeltaAngle(), visthumbwheel.ptr() ) ;
		    }
		}

    void	detach() { visthumbwheel_ = nullptr; }

protected:
		ThumbWheelMessenger()
		{}

    WeakPtr<ThumbWheel> visthumbwheel_;
};



ThumbWheel::ThumbWheel()
    : rotation(this)
    , thumbwheel_(new osgGeo::ThumbWheel)
{
    ref();
    refOsgPtr( thumbwheel_ );;
    setOsgNode( thumbwheel_ );
    messenger_ = new ThumbWheelMessenger( this );
    refOsgPtr( messenger_ );
    thumbwheel_->addRotateCallback( messenger_ );
    unRefNoDelete();
}


ThumbWheel::~ThumbWheel()
{
    unRefOsgPtr( messenger_ );
    unRefOsgPtr( thumbwheel_ );
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
