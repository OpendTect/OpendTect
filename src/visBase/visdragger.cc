/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          December 2003
 RCS:           $Id: visdragger.cc,v 1.3 2003-12-04 16:00:00 nanne Exp $
________________________________________________________________________

-*/


#include "visdragger.h"
#include "vistransform.h"

#include <Inventor/draggers/SoDragPointDragger.h>
#include <Inventor/draggers/SoTranslate1Dragger.h>
#include <Inventor/nodes/SoSwitch.h>


mCreateFactoryEntry( visBase::Dragger );

namespace visBase
{

Dragger::Dragger()
    : dragger(0)
    , started(this)
    , motion(this)
    , finished(this)
    , onoff( new SoSwitch )
    , group( new SoGroup )
    , transform( visBase::Transformation::create() )
{
    onoff->ref();
    group = new SoGroup;
    onoff->addChild( group );

    transform->ref();
    group->addChild( transform->getData() );
    setDraggerType( Translate );
    setRotation( Coord3(0,1,0), -M_PI/2 );
}


Dragger::~Dragger()
{
    if ( dragger )
    {
	dragger->removeStartCallback( visBase::Dragger::startCB, this );
	dragger->removeMotionCallback( visBase::Dragger::motionCB, this );
	dragger->removeFinishCallback( visBase::Dragger::finishCB, this );
	group->removeChild( dragger );
    }

    group->removeChild( transform->getData() );
    transform->unRef();
    onoff->unref();
}


void Dragger::setDraggerType( Type tp )
{
    if ( dragger )
    {
	dragger->removeStartCallback( visBase::Dragger::startCB, this );
	dragger->removeMotionCallback( visBase::Dragger::motionCB, this );
	dragger->removeFinishCallback( visBase::Dragger::finishCB, this );
	group->removeChild( dragger );
    }
    
    if ( tp == DragPoint )
	dragger = new SoDragPointDragger;
    else
	dragger = new SoTranslate1Dragger;

    group->addChild( dragger );
    dragger->addStartCallback( visBase::Dragger::startCB, this );
    dragger->addMotionCallback( visBase::Dragger::motionCB, this );
    dragger->addFinishCallback( visBase::Dragger::finishCB, this );
}


void Dragger::startCB( void* obj, SoDragger* )
{
    ( (visBase::Dragger*)obj )->started.trigger();
}


void Dragger::motionCB( void* obj, SoDragger* )
{
    ( (visBase::Dragger*)obj )->motion.trigger();
}


void Dragger::finishCB( void* obj, SoDragger* )
{
    ( (visBase::Dragger*)obj )->finished.trigger();
}


void Dragger::turnOn( bool yn )
{
    onoff->whichChild = yn ? 0 : SO_SWITCH_NONE;
}


bool Dragger::isOn() const
{
    return !onoff->whichChild.getValue();
}


void Dragger::setSize( const Coord3& size )
{
    transform->setScale( size );
}


Coord3 Dragger::getSize() const
{
    return transform->getScale();
}


void Dragger::setRotation( const Coord3& vec, float angle )
{
    transform->setRotation( vec, angle );
}


SoNode* Dragger::getData()
{
    return onoff;
}


void Dragger::setPos( const Coord3& pos )
{
    Coord3 newpos = transformation ? transformation->transform( pos ) : pos;
    transform->setTranslation( newpos );

    mDynamicCastGet( SoTranslate1Dragger*, t1d, dragger )
    mDynamicCastGet( SoDragPointDragger*, dpd, dragger )
    if ( t1d ) t1d->translation.setValue( 0, 0, 0 );
    else if ( dpd ) dpd->translation.setValue( 0, 0, 0 );
}


Coord3 Dragger::getPos() const
{
    SbVec3f pos_;
    mDynamicCastGet( SoTranslate1Dragger*, t1d, dragger )
    mDynamicCastGet( SoDragPointDragger*, dpd, dragger )
    if ( t1d ) pos_ = t1d->translation.getValue();
    else pos_ = dpd->translation.getValue();

    Coord3 pos( pos_[1], pos_[2], pos_[0] );
    pos.x *= transform->getScale()[0];
    pos.y *= transform->getScale()[1];
    pos.z *= transform->getScale()[2];

    Coord3 newpos = transform->getTranslation();
    pos += newpos;
    return transformation ? transformation->transformBack( pos ) : pos;
}


void Dragger::setColor( const Color& col )
{
}


}; // namespace visBase
