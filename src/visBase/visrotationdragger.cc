/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          August 2002
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: visrotationdragger.cc,v 1.9 2012-08-10 03:50:09 cvsaneesh Exp $";

#include "visrotationdragger.h"

#include "trigonometry.h"

#include <Inventor/draggers/SoRotateDiscDragger.h>
#include <Inventor/draggers/SoRotateSphericalDragger.h>
#include <Inventor/nodes/SoSwitch.h>

mCreateFactoryEntry( visBase::RotationDragger );

namespace visBase
{

RotationDragger::RotationDragger()
    : started( this )
    , motion( this )
    , changed( this )
    , finished( this )
    , onoff_( 0 )
    , cyldragger_( 0 )
    , spheredragger_( 0 )
    , feedback_( 0 )
    , activefeedback_( 0 )
{ }


RotationDragger::~RotationDragger()
{
    if ( getDragger() )
    {
	getDragger()->removeStartCallback(
		RotationDragger::startCB, this );
	getDragger()->removeMotionCallback(
		RotationDragger::motionCB, this );
	getDragger()->removeValueChangedCallback(
		RotationDragger::valueChangedCB, this );
	getDragger()->removeFinishCallback(
		RotationDragger::finishCB, this );
    }

    if ( onoff_ ) onoff_->unref();
    if ( cyldragger_ ) cyldragger_->unref();
    if ( spheredragger_ ) spheredragger_->unref();
    if ( feedback_ ) feedback_->unRef();
    if ( activefeedback_ ) activefeedback_->unRef();
}


Quaternion RotationDragger::get() const
{
    const float* res = 0;
    if ( cyldragger_ )
	res = cyldragger_->rotation.getValue().getValue();
    else if ( spheredragger_ )
	res = spheredragger_->rotation.getValue().getValue();

    return Quaternion( res[3],  res[0], res[1], res[2] );
}


void RotationDragger::set( const Quaternion& q )
{
    NotifyStopper stopper( changed );
    if ( cyldragger_ )
	cyldragger_->rotation.setValue( (float) q.vec_.x, (float) q.vec_.y, 
					    (float) q.vec_.z, (float) q.s_ );
    else if ( spheredragger_ )
	spheredragger_->rotation.setValue( (float) q.vec_.x, (float) q.vec_.y, 
					     (float) q.vec_.z, (float) q.s_ );
}


void RotationDragger::turnOn( bool yn )
{
    if ( !onoff_ ) { onoff_ = new SoSwitch(); onoff_->ref(); }
    onoff_->whichChild = yn ? 0 : SO_SWITCH_NONE;
}


bool RotationDragger::isOn() const
{
    return onoff_ && !onoff_->whichChild.getValue();
}


void RotationDragger::setCallbacks( SoDragger* dragger )
{
    dragger->addStartCallback( RotationDragger::startCB, this );
    dragger->addMotionCallback( RotationDragger::motionCB, this );
    dragger->addValueChangedCallback(
	    RotationDragger::valueChangedCB, this );
    dragger->addFinishCallback( RotationDragger::finishCB, this );
}



SoNode* RotationDragger::gtInvntrNode()
{
    if ( !getDragger() )
    {
	spheredragger_ = new SoRotateSphericalDragger;
	spheredragger_->ref();
	setCallbacks( spheredragger_ );
    }

    if ( onoff_ && !onoff_->getNumChildren()  )
	onoff_->addChild( getDragger() );

    return onoff_ ? (SoNode*)onoff_ : (SoNode*)getDragger();
}


void RotationDragger::doAxisRotate()
{
    if ( getDragger() ) return;

    cyldragger_ = new SoRotateDiscDragger;
    cyldragger_->ref();
    setCallbacks( cyldragger_ );

    if ( onoff_ ) onoff_->addChild( cyldragger_ );
}


void RotationDragger::useSwitch()
{
    onoff_ = new SoSwitch;
    onoff_->ref();
    if ( getDragger() ) onoff_->addChild( getDragger() );
}


void RotationDragger::setOwnFeedback( DataObject* dobj, bool active )
{
    getDragger()->setPart( active ? "rotatorActive" : "rotator",
	   		   dobj->getInventorNode() );
    DataObject*& prev = active ? activefeedback_ : feedback_;
    if ( prev ) prev->unRef();
    prev = dobj;
    dobj->ref();
}


SoDragger* RotationDragger::getDragger()
{
    if ( spheredragger_ ) return spheredragger_;
    return cyldragger_;
}


void RotationDragger::startCB( void* obj, SoDragger* )
{
    ( (RotationDragger*)obj )->started.trigger();
}


void RotationDragger::motionCB( void* obj, SoDragger* )
{
    ( (RotationDragger*)obj )->motion.trigger();
}


void RotationDragger::valueChangedCB( void* obj, SoDragger* )
{
    ( (RotationDragger*)obj )->changed.trigger();
}


void RotationDragger::finishCB( void* obj, SoDragger* )
{
    ( (RotationDragger*)obj )->finished.trigger();
}


}; // namespace visBase
