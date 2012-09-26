/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          December 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "visdragger.h"

#include "visevent.h"
#include "vistransform.h"

#include <Inventor/draggers/SoDragPointDragger.h>
#include <Inventor/draggers/SoTranslate1Dragger.h>
#include <Inventor/draggers/SoTranslate2Dragger.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoSeparator.h>
#include "SoScale3Dragger.h"

mCreateFactoryEntry( visBase::Dragger );

namespace visBase
{

Dragger::Dragger()
    : dragger_(0)
    , started(this)
    , motion(this)
    , finished(this)
    , onoff_( new SoSwitch )
    , root_( new SoSeparator )
    , positiontransform_( Transformation::create() )
    , displaytrans_( 0 )
    , rightclicknotifier_(this)
    , rightclickeventinfo_( 0 )
{
    onoff_->ref();
    onoff_->addChild( root_ );
    onoff_->whichChild = 0;

    positiontransform_->ref();
    root_->addChild( positiontransform_->getInventorNode() );
    setDraggerType( Translate1D );
    setDefaultRotation();
}


void Dragger::setDefaultRotation()
{
    setRotation( Coord3(0,1,0), -M_PI_2 );
}

Dragger::~Dragger()
{
    if ( dragger_ )
    {
	dragger_->removeStartCallback( Dragger::startCB, this );
	dragger_->removeMotionCallback( Dragger::motionCB, this );
	dragger_->removeFinishCallback( Dragger::finishCB, this );
	root_->removeChild( dragger_ );
    }

    root_->removeChild( positiontransform_->getInventorNode() );
    positiontransform_->unRef();
    onoff_->unref();
    if ( displaytrans_ ) displaytrans_->unRef();
}


bool Dragger::selectable() const { return true; }


void Dragger::setDraggerType( Type tp )
{
    if ( dragger_ )
    {
	dragger_->removeStartCallback( Dragger::startCB, this );
	dragger_->removeMotionCallback( Dragger::motionCB, this );
	dragger_->removeFinishCallback( Dragger::finishCB, this );
	root_->removeChild( dragger_ );
    }
    
    if ( tp == Translate3D )
	dragger_ = new SoDragPointDragger;
    else if ( tp==Translate2D )
	dragger_ = new SoTranslate2Dragger;
    else if ( tp==Translate1D )
	dragger_ = new SoTranslate1Dragger;
    else
	dragger_ = new SoScale3Dragger;
    

    root_->addChild( dragger_ );
    dragger_->addStartCallback( Dragger::startCB, this );
    dragger_->addMotionCallback( Dragger::motionCB, this );
    dragger_->addFinishCallback( Dragger::finishCB, this );
}


void Dragger::setDisplayTransformation( const mVisTrans* nt )
{
    Coord3 pos = getPos();
    if ( displaytrans_ )
    {
	displaytrans_->unRef();
	displaytrans_ = 0;
    }

    displaytrans_ = nt;
    if ( displaytrans_ )
    {
	displaytrans_->ref();
    }

    setPos( pos );
}


const mVisTrans* Dragger::getDisplayTransformation() const
{
    return displaytrans_;
}


void Dragger::setOwnShape( DataObject* newshape, const char* partname )
{
    if ( newshape && newshape->getInventorNode() )
	dragger_->setPart( partname, newshape->getInventorNode() );
}


SoNode* Dragger::getShape( const char* nm )
{
    return dragger_->getPart( nm, false );
}


void Dragger::triggerRightClick( const EventInfo* eventinfo )
{
    rightclickeventinfo_ = eventinfo;
    rightclicknotifier_.trigger();
}


const TypeSet<int>* Dragger::rightClickedPath() const
{ return rightclickeventinfo_ ? &rightclickeventinfo_->pickedobjids : 0; }


const EventInfo* Dragger::rightClickedEventInfo() const
{ return rightclickeventinfo_; }


void Dragger::startCB( void* obj, SoDragger* )
{
    ( (Dragger*)obj )->started.trigger();
}


void Dragger::motionCB( void* obj, SoDragger* )
{
    ( (Dragger*)obj )->motion.trigger();
}


void Dragger::finishCB( void* obj, SoDragger* )
{
    ( (Dragger*)obj )->finished.trigger();
}


void Dragger::turnOn( bool yn )
{
    onoff_->whichChild = yn ? 0 : SO_SWITCH_NONE;
}


bool Dragger::isOn() const
{
    return !onoff_->whichChild.getValue();
}


void Dragger::setSize( const Coord3& size )
{
    positiontransform_->setScale( size/2 );
    mDynamicCastGet( SoScale3Dragger*, s3d, dragger_ );
    if ( s3d ) s3d->scale.setValue( SbVec3f(1,1,1) );
}


Coord3 Dragger::getSize() const
{
    SbVec3f scale( 1, 1, 1 );
    mDynamicCastGet( SoScale3Dragger*, s3d, dragger_ );
    if ( s3d ) scale = s3d->scale.getValue();

    const Coord3 transscale = positiontransform_->transform(
	    Coord3(scale[0],scale[1],scale[2]) );
    const Coord3 transorigin = positiontransform_->transform( Coord3(0,0,0) );

    return (transscale - transorigin)*2;
}


void Dragger::setRotation( const Coord3& vec, float angle )
{
    positiontransform_->setRotation( vec, angle );
}


SoNode* Dragger::gtInvntrNode()
{
    return onoff_ ? (SoNode*) onoff_ : (SoNode*) root_;
}


void Dragger::setPos( const Coord3& pos )
{
    Coord3 newpos = displaytrans_ ? displaytrans_->transform( pos ) : pos;
    positiontransform_->setTranslation( newpos );

    mDynamicCastGet( SoTranslate1Dragger*, t1d, dragger_ )
    mDynamicCastGet( SoTranslate2Dragger*, t2d, dragger_ )
    mDynamicCastGet( SoDragPointDragger*, dpd, dragger_ )
    if ( t1d ) t1d->translation.setValue( 0, 0, 0 );
    else if ( t2d ) t2d->translation.setValue( 0, 0, 0 );
    else if ( dpd ) dpd->translation.setValue( 0, 0, 0 );
}


Coord3 Dragger::getPos() const
{
    SbVec3f pos( 0, 0, 0 );
    mDynamicCastGet( SoTranslate1Dragger*, t1d, dragger_ )
    mDynamicCastGet( SoTranslate2Dragger*, t2d, dragger_ )
    mDynamicCastGet( SoDragPointDragger*, dpd, dragger_ )
    if ( t1d ) pos = t1d->translation.getValue();
    else if ( t2d ) pos = t2d->translation.getValue();
    else if ( dpd ) pos = dpd->translation.getValue();


    const Coord3 coord = positiontransform_->transform(
	    Coord3(pos[0],pos[1],pos[2]) );

    return displaytrans_ ? displaytrans_->transformBack( coord ) : coord;
}


}; // namespace visBase
