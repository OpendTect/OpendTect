/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          August 2002
 RCS:           $Id: visboxdragger.cc,v 1.7 2003-11-07 12:22:02 bert Exp $
________________________________________________________________________

-*/

#include "visboxdragger.h"
#include "ranges.h"
#include "iopar.h"

#include "Inventor/nodes/SoSwitch.h"
#include "Inventor/draggers/SoTabBoxDragger.h"

mCreateFactoryEntry( visBase::BoxDragger );

visBase::BoxDragger::BoxDragger()
    : started( this )
    , motion( this )
    , changed( this )
    , finished( this )
    , onoff( new SoSwitch )
    , boxdragger( new SoTabBoxDragger )
    , xinterval( 0 )
    , yinterval( 0 )
    , zinterval( 0 )
{
    onoff->addChild( boxdragger );
    onoff->ref();
    boxdragger->addStartCallback(
	    visBase::BoxDragger::startCB, this );
    boxdragger->addMotionCallback(
	    visBase::BoxDragger::motionCB, this );
    boxdragger->addValueChangedCallback(
	    visBase::BoxDragger::valueChangedCB, this );
    boxdragger->addFinishCallback(
	    visBase::BoxDragger::finishCB, this );
}


visBase::BoxDragger::~BoxDragger()
{
    boxdragger->removeStartCallback(
	    visBase::BoxDragger::startCB, this );
    boxdragger->removeMotionCallback(
	    visBase::BoxDragger::motionCB, this );
    boxdragger->removeValueChangedCallback(
	    visBase::BoxDragger::valueChangedCB, this );
    boxdragger->removeFinishCallback(
	    visBase::BoxDragger::finishCB, this );

    onoff->unref();
    delete xinterval;
    delete yinterval;
    delete zinterval;
}


void visBase::BoxDragger::setCenter( const Coord3& pos )
{
    boxdragger->translation.setValue( pos.x, pos.y, pos.z );
    prevcenter = pos;
}


Coord3 visBase::BoxDragger::center() const
{
    SbVec3f pos = boxdragger->translation.getValue();
    return Coord3( pos[0], pos[1], pos[2] );
}


void visBase::BoxDragger::setWidth( const Coord3& pos )
{
    boxdragger->scaleFactor.setValue( pos.x/2, pos.y/2, pos.z/2 );
    prevwidth = pos;
}


Coord3 visBase::BoxDragger::width() const
{
    SbVec3f pos = boxdragger->scaleFactor.getValue();
    return Coord3( pos[0]*2, pos[1]*2, pos[2]*2 );
}


void visBase::BoxDragger::setSpaceLimits( const Interval<float>& x,
					  const Interval<float>& y,
					  const Interval<float>& z)
{
    if ( !xinterval )
    {
	xinterval = new Interval<float>(x);
	yinterval = new Interval<float>(y);
	zinterval = new Interval<float>(z);
	return;
    }

    *xinterval = x;
    *yinterval = y;
    *zinterval = z;
}


void visBase::BoxDragger::turnOn( bool yn )
{
    onoff->whichChild = yn ? 0 : SO_SWITCH_NONE;
}


bool visBase::BoxDragger::isOn() const
{
    return !onoff->whichChild.getValue();
}


SoNode* visBase::BoxDragger::getData()
{ return onoff; }


void visBase::BoxDragger::startCB( void* obj, SoDragger* )
{
    ( (visBase::BoxDragger*)obj )->started.trigger();
}


void visBase::BoxDragger::motionCB( void* obj, SoDragger* )
{
    ( (visBase::BoxDragger*)obj )->motion.trigger();
}


void visBase::BoxDragger::valueChangedCB( void* obj, SoDragger* )
{
    visBase::BoxDragger* thisp = (visBase::BoxDragger*) obj;
    const Coord3 center = thisp->center();
    const Coord3 width = thisp->width();

    bool reverse = false;
    if ( thisp->xinterval
	    && (!thisp->xinterval->includes( center.x-width.x/2 ) ||
		!thisp->xinterval->includes( center.x+width.x/2 )) )
	reverse = true;
    else if ( thisp->yinterval
	    && (!thisp->yinterval->includes( center.y-width.y/2 ) ||
		!thisp->yinterval->includes( center.y+width.y/2 )) )
	reverse = true;
    else if ( thisp->zinterval
	    && (!thisp->zinterval->includes( center.z-width.z/2 ) ||
		!thisp->zinterval->includes( center.z+width.z/2 )) )
	reverse = true;

    if ( reverse && thisp->prevwidth.isDefined() &&
	    	    thisp->prevcenter.isDefined() )
    {
	thisp->setCenter( thisp->prevcenter );
	thisp->setWidth( thisp->prevwidth );
	return;
    }

    thisp->prevcenter = center;
    thisp->prevwidth = width;

    ( (visBase::BoxDragger*)obj )->changed.trigger();
}


void visBase::BoxDragger::finishCB( void* obj, SoDragger* )
{
    ( (visBase::BoxDragger*)obj )->finished.trigger();
}

