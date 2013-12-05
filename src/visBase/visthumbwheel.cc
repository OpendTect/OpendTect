/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          December 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


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
    void	operator() (osg::Node* node, osg::NodeVisitor* nv )
		{
		    if ( visthumbwheel_ && nv )
		    {
			osgGeo::ThumbWheelEventNodeVisitor* thnv =
			    (osgGeo::ThumbWheelEventNodeVisitor*) nv;
					
			visthumbwheel_->rotation.trigger(
				    thnv->getDeltaAngle(), visthumbwheel_ );
		    }
		}
    void	detach() { visthumbwheel_ = 0; }
    
protected:
		ThumbWheelMessenger()
		{
		    
		}
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


void ThumbWheel::setBackgroundColor( const Color& bgcol )
{
    Color col = bgcol;
    if ( col.average()>128 )
        col.lighter( 0.7 );
    else
        col.lighter( 1.5 );

    osg::Vec4 osgcol(col2f(r),col2f(g),col2f(b), 1.0);

    thumbwheel_->setBorderColor( osgcol );
}
    

float ThumbWheel::getAngle() const
{
    return thumbwheel_->getAngle();
}
    
    
void ThumbWheel::setAngle( float a )
{
    thumbwheel_->setAngle( a );
}


}; // namespace visBase
