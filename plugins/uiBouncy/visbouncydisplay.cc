/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Karthika
 Date:          Sep 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: visbouncydisplay.cc,v 1.2 2009-09-15 14:40:46 cvskarthika Exp $";

#include "visbouncydisplay.h"
#include "visbeachball.h"
#include "beachballdata.h"

#include "vissurvscene.h"
#include "uivispartserv.h"
#include "survinfo.h"
#include "uiodscenemgr.h"
#include "visevent.h"

#include <Inventor/nodes/SoRotation.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoTransform.h>

mCreateFactoryEntry( uiBouncy::BouncyDisplay );

namespace uiBouncy
{

BouncyDisplay::BouncyDisplay()
    : VisualObjectImpl(false) 
    , bb_(visBase::BeachBall::create())
    , rotation_(new SoRotation)
    , paddle_(new SoCube)
    , paddletransform_(new SoTransform)
    , sceneid_(0)
    , eventcatcher_(0)
    , newEvent(this)
    , ispaused_(false)
    , isstopped_(true)
{
    SoSeparator* sep = new SoSeparator;
    addChild( sep );
    
    paddletransform_->ref();
    bool work = true;
    Coord min = SI().minCoord( work );
    Coord max = SI().maxCoord( work );
    float z = SI().zRange( work ).start + 
	(SI().zRange( work ).stop - SI().zRange( work ).start) * 0.5;
    paddletransform_->translation = SbVec3f( 
	    min.x+(max.x-min.x)*0.5,
	    min.y+(max.y-min.y)*0.5,
	    z );
    sep->addChild(paddletransform_);
    paddle_->ref();
    paddle_->width = 2000;
    paddle_->height = 500;
    paddle_->depth = 800;
    sep->addChild( paddle_ );

    rotation_->ref();  // use RotationDragger instead?
    addChild( rotation_ );
   
    bb_->ref();
    addChild( bb_->getInventorNode() );
}


BouncyDisplay::~BouncyDisplay()
{
    removeBouncy();
    setSceneEventCatcher( 0 );
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
    pErrMsg("addBouncy");
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

    // later: common paddle for all the balls
    if ( paddle_ )
    {
	paddle_->unrefNoDelete();
	paddle_ = 0;
    }

    if ( paddletransform_ )
    {
	paddletransform_->unrefNoDelete();
	paddletransform_ = 0;
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
    pErrMsg("Oh");
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
    pErrMsg("hi");
    if ( isstopped_ || eventcatcher_->isHandled() ) return;

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

