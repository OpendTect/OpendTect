/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          August 2002
 RCS:           $Id: visboxdragger.cc,v 1.12 2005-02-04 14:31:34 kristofer Exp $
________________________________________________________________________

-*/

#include "visboxdragger.h"
#include "ranges.h"
#include "iopar.h"
#include "survinfo.h"

#include <Inventor/draggers/SoTabBoxDragger.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoShapeHints.h>


namespace visBase
{

mCreateFactoryEntry( BoxDragger );
BoxDragger::BoxDragger()
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
	    BoxDragger::startCB, this );
    boxdragger->addMotionCallback(
	    BoxDragger::motionCB, this );
    boxdragger->addValueChangedCallback(
	    BoxDragger::valueChangedCB, this );
    boxdragger->addFinishCallback(
	    BoxDragger::finishCB, this );

    setOwnShapeHints();

}


BoxDragger::~BoxDragger()
{
    boxdragger->removeStartCallback(
	    BoxDragger::startCB, this );
    boxdragger->removeMotionCallback(
	    BoxDragger::motionCB, this );
    boxdragger->removeValueChangedCallback(
	    BoxDragger::valueChangedCB, this );
    boxdragger->removeFinishCallback(
	    BoxDragger::finishCB, this );

    onoff->unref();
    delete xinterval;
    delete yinterval;
    delete zinterval;
}


void BoxDragger::setOwnShapeHints()
{
    SoShapeHints* myHints = new SoShapeHints;
    myHints->shapeType = SoShapeHints::SOLID;
    myHints->vertexOrdering = SI().isClockWise()
	? SoShapeHints::COUNTERCLOCKWISE : SoShapeHints::CLOCKWISE;
    
    SoDragger* child;
    const char* tabstr( "tabPlane" );
    for ( int i = 1; i <= 6; i++ )
    {
	BufferString str( tabstr ); str += i;
	child = (SoDragger*)boxdragger->getPart( str.buf(), false );
	child->setPart( "scaleTabHints", myHints );
    }
}


void BoxDragger::setCenter( const Coord3& pos )
{
    boxdragger->translation.setValue( pos.x, pos.y, pos.z );
    prevcenter = pos;
}


Coord3 BoxDragger::center() const
{
    SbVec3f pos = boxdragger->translation.getValue();
    return Coord3( pos[0], pos[1], pos[2] );
}


void BoxDragger::setWidth( const Coord3& pos )
{
    boxdragger->scaleFactor.setValue( pos.x/2, pos.y/2, pos.z/2 );
    prevwidth = pos;
}


Coord3 BoxDragger::width() const
{
    SbVec3f pos = boxdragger->scaleFactor.getValue();
    return Coord3( pos[0]*2, pos[1]*2, pos[2]*2 );
}


void BoxDragger::setSpaceLimits( const Interval<float>& x,
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


void BoxDragger::turnOn( bool yn )
{
    onoff->whichChild = yn ? 0 : SO_SWITCH_NONE;
}


bool BoxDragger::isOn() const
{
    return !onoff->whichChild.getValue();
}


SoNode* BoxDragger::getInventorNode()
{ return onoff; }


void BoxDragger::startCB( void* obj, SoDragger* )
{
    BoxDragger* thisp = (BoxDragger*)obj;
    thisp->prevcenter = thisp->center();
    thisp->prevwidth = thisp->width();
    thisp->started.trigger();
}


void BoxDragger::motionCB( void* obj, SoDragger* )
{
    ( (BoxDragger*)obj )->motion.trigger();
}

#define mCheckDim(dim)\
if ( thisp->dim##interval )\
{\
    if ( !thisp->dim##interval->includes(center.dim-width.dim/2) || \
	 !thisp->dim##interval->includes(center.dim+width.dim/2))\
    {\
	if ( constantwidth ) center.dim = thisp->prevcenter.dim; \
	else \
	{ \
	    width.dim = thisp->prevwidth.dim; \
	    center.dim = thisp->prevcenter.dim; \
	} \
	change = true; \
    }\
}

void BoxDragger::valueChangedCB( void* obj, SoDragger* )
{
    BoxDragger* thisp = (BoxDragger*)obj;

    Coord3 center = thisp->center();
    Coord3 width = thisp->width();

    const bool constantwidth =
	mIsEqualRel(width.x,thisp->prevwidth.x,1e-6) &&
	mIsEqualRel(width.y,thisp->prevwidth.y,1e-6) &&
	mIsEqualRel(width.z,thisp->prevwidth.z,1e-6);

    if  ( constantwidth && center==thisp->prevcenter )
	return;

    bool change = false;
    mCheckDim(x);
    mCheckDim(y);
    mCheckDim(z);

    if ( change )
    {
	thisp->setCenter( center );
	thisp->setWidth(  width );
    }

    thisp->prevcenter = center;
    thisp->prevwidth = width;

    ( (BoxDragger*)obj )->changed.trigger();
}


void BoxDragger::finishCB( void* obj, SoDragger* )
{
    ( (BoxDragger*)obj )->finished.trigger();
}


}; // namespace visBase
