/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          December 2003
 RCS:           $Id: visdragger.cc,v 1.10 2004-11-16 09:28:33 kristofer Exp $
________________________________________________________________________

-*/


#include "visdragger.h"

#include "visevent.h"
#include "vistransform.h"

#include <Inventor/draggers/SoDragPointDragger.h>
#include <Inventor/draggers/SoTranslate1Dragger.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoSeparator.h>


mCreateFactoryEntry( visBase::Dragger );

namespace visBase
{

Dragger::Dragger()
    : dragger(0)
    , started(this)
    , motion(this)
    , finished(this)
    , onoff( new SoSwitch )
    , separator( new SoSeparator )
    , positiontransform( visBase::Transformation::create() )
    , displaytrans( 0 )
    , rightclicknotifier(this)
{
    onoff->ref();
    onoff->addChild( separator );

    positiontransform->ref();
    separator->addChild( positiontransform->getInventorNode() );
    setDraggerType( Translate );
    setDefaultRotation();
}


void Dragger::setDefaultRotation()
{
    setRotation( Coord3(0,1,0), -M_PI_2 );
}

Dragger::~Dragger()
{
    if ( dragger )
    {
	dragger->removeStartCallback( visBase::Dragger::startCB, this );
	dragger->removeMotionCallback( visBase::Dragger::motionCB, this );
	dragger->removeFinishCallback( visBase::Dragger::finishCB, this );
	separator->removeChild( dragger );
    }

    separator->removeChild( positiontransform->getInventorNode() );
    positiontransform->unRef();
    onoff->unref();
    if ( displaytrans ) displaytrans->unRef();
}


bool Dragger::selectable() const { return true; }


void Dragger::setDraggerType( Type tp )
{
    if ( dragger )
    {
	dragger->removeStartCallback( visBase::Dragger::startCB, this );
	dragger->removeMotionCallback( visBase::Dragger::motionCB, this );
	dragger->removeFinishCallback( visBase::Dragger::finishCB, this );
	separator->removeChild( dragger );
    }
    
    if ( tp == DragPoint )
	dragger = new SoDragPointDragger;
    else
	dragger = new SoTranslate1Dragger;

    separator->addChild( dragger );
    dragger->addStartCallback( visBase::Dragger::startCB, this );
    dragger->addMotionCallback( visBase::Dragger::motionCB, this );
    dragger->addFinishCallback( visBase::Dragger::finishCB, this );
}


void Dragger::setDisplayTransformation( Transformation* nt )
{
    Coord3 pos = getPos();
    if ( displaytrans )
    {
	displaytrans->unRef();
	displaytrans = 0;
    }

    displaytrans = nt;
    if ( displaytrans )
    {
	displaytrans->ref();
    }

    setPos( pos );
}


Transformation* Dragger::getDisplayTransformation()
{
    return displaytrans;
}


void Dragger::setOwnShape(visBase::DataObject* newshape, const char* partname )
{
    if ( newshape && newshape->getInventorNode() )
    {
	dragger->setPart(partname,newshape->getInventorNode());
    }
}


void Dragger::triggerRightClick(const EventInfo* eventinfo)
{
    rightclickpath = eventinfo ? &eventinfo->pickedobjids : 0;
    rightclicknotifier.trigger();
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
    positiontransform->setScale( size );
}


Coord3 Dragger::getSize() const
{
    return positiontransform->getScale();
}


void Dragger::setRotation( const Coord3& vec, float angle )
{
    positiontransform->setRotation( vec, angle );
}


SoNode* Dragger::getInventorNode()
{
    return onoff;
}


void Dragger::setPos( const Coord3& pos )
{
    Coord3 newpos = displaytrans ? displaytrans->transform( pos ) : pos;
    positiontransform->setTranslation( newpos );

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


    const Coord3 pos = positiontransform->transform(
	    Coord3(pos_[0],pos_[1],pos_[2]) );

    return displaytrans ? displaytrans->transformBack( pos ) : pos;
}


void Dragger::setColor( const Color& col )
{
}


}; // namespace visBase
