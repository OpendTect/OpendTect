/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Karthika
 Date:          Sep 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: visbouncydisplay.cc,v 1.1 2009-09-14 22:50:45 cvskarthika Exp $";

#include "visbouncydisplay.h"
#include "visbeachball.h"
#include "beachballdata.h"

#include "vissurvscene.h"
#include "uivispartserv.h"
#include "uiodscenemgr.h"
#include "visevent.h"

#include <Inventor/nodes/SoRotation.h>

mCreateFactoryEntry( uiBouncy::BouncyDisplay );

namespace uiBouncy
{

BouncyDisplay::BouncyDisplay()
    : VisualObjectImpl(false) 
    , bb_(visBase::BeachBall::create())
    , rotation_(new SoRotation)
    , sceneid_(0)
    , eventcatcher_(0)
    , newEvent(this)
    , ispaused_(false)
    , isstopped_(true)
{
    rotation_->ref();  // use RotationDragger instead?
    addChild( rotation_ );
   
    bb_->ref();
    addChild( bb_->getInventorNode() );
}


BouncyDisplay::~BouncyDisplay()
{
    removeBouncy();
}


visBase::BeachBall* BouncyDisplay::beachball() const
{
    return bb_;
}


void BouncyDisplay::zScaleCB( CallBacker* )
{
    setBallScale();
}


void BouncyDisplay::setSceneID( const int newid )
{
    mDynamicCastGet( visSurvey::Scene*, scene,
	    ODMainWin()->applMgr().visServer()->getObject( sceneid_ ) );
    if ( scene )
	scene->zstretchchange.remove( mCB(this,BouncyDisplay,zScaleCB) );

    mDynamicCastGet( visSurvey::Scene*, newscene,
	    ODMainWin()->applMgr().visServer()->getObject( newid ) );
    if ( newscene )
    {
	newscene->zstretchchange.notify( mCB(this,BouncyDisplay,zScaleCB) );
        sceneid_ = newid;
    }
}


int BouncyDisplay::sceneid() const
{
    return sceneid_;
}



void BouncyDisplay::addBouncy( visBeachBall::BallProperties bp )
{
    mDynamicCastGet( visSurvey::Scene*, scene,
	    ODMainWin()->applMgr().visServer()->getObject( sceneid_ ) );

    if ( scene )
	scene->zstretchchange.notify( mCB(this, BouncyDisplay, zScaleCB ) );

    bb_->setDisplayTransformation( scene->getUTM2DisplayTransform() );
    setBallScale();
    bb_->setBallProperties( bp );
    ODMainWin()->applMgr().visServer()->addObject( bb_, sceneid_, true );
}


void BouncyDisplay::removeBouncy()
{
    if ( !rotation_ || !bb_ )
	return;

    mDynamicCastGet( visSurvey::Scene*, scene,
	    ODMainWin()->applMgr().visServer()->getObject( sceneid_ ) );
    if ( scene )
	scene->zstretchchange.remove( mCB(this, BouncyDisplay, zScaleCB ) );
    ODMainWin()->applMgr().visServer()->removeObject( bb_, sceneid_ );
    
    if ( rotation_ )
    {
	rotation_->unrefNoDelete();
	rotation_ = 0;
    }

    if ( bb_ )
    {
	bb_->unRefNoDelete();
	bb_ = 0;
    }
}


void BouncyDisplay::setBallScale()
{
    if ( !bb_ ) return;
    
    mDynamicCastGet( visSurvey::Scene*, scene,
	    ODMainWin()->applMgr().visServer()->getObject( sceneid_ ) );    
    const float zscale = scene ? scene->getZStretch()*scene->getZScale() : 1;
    bb_->setZScale( zscale );
}


void BouncyDisplay::start()
{
    ispaused_ = isstopped_ = false;
}


void BouncyDisplay::stop()
{
    ispaused_ = false;
    isstopped_ = true;
}


bool BouncyDisplay::ispaused() const
{
    return ispaused_;
}


bool BouncyDisplay::isstopped() const
{
    return isstopped_;
}


void BouncyDisplay::setSceneEventCatcher( visBase::EventCatcher* nev )
{
    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.remove( 
		mCB(this, BouncyDisplay, eventCB) );
	eventcatcher_->unRef();
    }

    eventcatcher_ = nev;

    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.notify(
		mCB(this, BouncyDisplay, eventCB) );
	eventcatcher_->ref();
    }
}


void BouncyDisplay::eventCB( CallBacker* cb )
{
     if ( eventcatcher_->isHandled() ) return;

     mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb );

     if ( eventinfo.type == visBase::MouseMovement )
     {
         // move the paddle horizontally with the mouse   
     }
     else if ( eventinfo.type == visBase::Keyboard )
     {
	 switch ( eventinfo.key )
	 {
	     case ' ':
		 {
		     // pause/resume
		     ispaused_ = !ispaused_;
		     break;
		 }
	     case OD::Escape:
		 {
		     // quit
		     isstopped_ = true;
		     break;
		 }
	     case OD::Left:
		 {
		     break;
		 }
	     case OD::Right:
		 {
		     break;
		 }
	     case OD::Plus:
		 {
		     // increase speed
		     break;
		 }
	     case OD::Minus:
		 {
		     // decrease speed
		     break;
		 }
	 }
     }
     else
	 return;

     
     eventcatcher_->setHandled();
     newEvent.trigger();
}

}

