/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          August 2002
 RCS:           $Id: visboxdragger.cc,v 1.2 2002-10-14 14:24:39 niclas Exp $
________________________________________________________________________

-*/

#include "visboxdragger.h"
#include "iopar.h"

#include "Inventor/nodes/SoSeparator.h"
#include "Inventor/draggers/SoTabBoxDragger.h"

mCreateFactoryEntry( visBase::BoxDragger );

visBase::BoxDragger::BoxDragger()
    : started( this )
    , motion( this )
    , changed( this )
    , finished( this )
    , root( new SoSeparator )
    , boxdragger( new SoTabBoxDragger )
{
    root->addChild( boxdragger );
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
}


void visBase::BoxDragger::setCenter( const Coord3& pos_ )
{
    Coord3 pos( pos_ );
    boxdragger->translation.setValue( pos.x, pos.y, pos.z );
}


Coord3 visBase::BoxDragger::center() const
{
    SbVec3f pos = boxdragger->translation.getValue();
    Coord3 res( pos[0], pos[1], pos[2] );
    return res;
}


void visBase::BoxDragger::setScale( const Coord3& pos )
{
    boxdragger->scaleFactor.setValue( pos.x/2, pos.y/2, pos.z/2 );
}


Coord3 visBase::BoxDragger::scale() const
{
    SbVec3f pos = boxdragger->scaleFactor.getValue();
    Coord3 res( pos[0]*2, pos[1]*2, pos[2]*2 );
    return res;
}


SoNode* visBase::BoxDragger::getData()
{ return root; }


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
    ( (visBase::BoxDragger*)obj )->changed.trigger();
}


void visBase::BoxDragger::finishCB( void* obj, SoDragger* )
{
    ( (visBase::BoxDragger*)obj )->finished.trigger();
}

