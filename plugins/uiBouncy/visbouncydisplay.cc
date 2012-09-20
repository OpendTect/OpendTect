/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Karthika
 Date:          Sep 2009
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "visbouncydisplay.h"
#include "beachballdata.h"
#include "visbeachball.h"
#include "viscube.h"

#include "vissurvscene.h"
#include "uivispartserv.h"
#include "survinfo.h"
#include "uiodscenemgr.h"
#include "visevent.h"
#include "vistransform.h"

#include <Inventor/nodes/SoRotation.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/events/SoKeyboardEvent.h>

#define mPaddleSize Coord3( 2500, 1000, 800 )
#define mPaddleStep 500

mCreateFactoryEntry( uiBouncy::BouncyDisplay );

namespace uiBouncy
{

BouncyDisplay::BouncyDisplay()
    : VisualObjectImpl(false) 
    , bb_(visBase::BeachBall::create())
    , rotation_(new SoRotation)
    , paddle_(visBase::Cube::create())
    , sceneid_(0)
    , eventcatcher_(0)
    , newEvent(this)
    , ispaused_(false)
    , isstopped_(true)
{
    SoSeparator* sep = new SoSeparator;
    addChild( sep );
    
    paddle_->ref();
    sep->addChild( paddle_->getInventorNode() );

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


void BouncyDisplay::zScaleCB( CallBacker* )
{
    setScale();
}


void BouncyDisplay::setSceneID( const int newid )
{
    mDynamicCastGet( visSurvey::Scene*, newscene,
	    ODMainWin()->applMgr().visServer()->getObject( newid ) );
    if ( !newscene )
	return;
   
    mDynamicCastGet( visSurvey::Scene*, scene,
	    ODMainWin()->applMgr().visServer()->getObject( sceneid_ ) );
    if ( scene )
	scene->zstretchchange.remove( mCB(this,BouncyDisplay,zScaleCB) );   

    newscene->zstretchchange.notify( mCB(this,BouncyDisplay,zScaleCB) );
    sceneid_ = newid;
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
    {
	scene->zstretchchange.notify( mCB(this, BouncyDisplay, zScaleCB ) );
	bb_->setDisplayTransformation( scene->getUTM2DisplayTransform() );
	paddle_->setDisplayTransformation( scene->getUTM2DisplayTransform() );
    }

    bb_->setBallProperties( bp );
    paddle_->setWidth( mPaddleSize );
    setScale();
    
    bool work = true;
    Coord min = SI().minCoord( work );
    Coord max = SI().maxCoord( work );
    float z = SI().zRange( work ).stop;
    
    Coord3 c( min.x+(max.x-min.x)*0.5, min.y+(max.y-min.y)*0.5, z );  
    paddle_->setCenterPos( c );
}


void BouncyDisplay::removeBouncy()
{
    mDynamicCastGet( visSurvey::Scene*, scene,
	    ODMainWin()->applMgr().visServer()->getObject( sceneid_ ) );
    if ( scene )
	scene->zstretchchange.remove( mCB(this, BouncyDisplay, zScaleCB ) );

    // later: common paddle for all the balls
    if ( paddle_ )
    {
	paddle_->unRefNoDelete();
	paddle_ = 0;
    }

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


void BouncyDisplay::setScale()
{
    mDynamicCastGet( visSurvey::Scene*, scene,
	    ODMainWin()->applMgr().visServer()->getObject( sceneid_ ) );    
    const float zscale = scene ? scene->getZStretch()*scene->getZScale() : 1;
    if ( bb_ ) 
        bb_->setZScale( zscale );
    if ( paddle_ )
    {
	Coord3 ps = paddle_->width();
	ps.z /= (2*zscale);
	paddle_->setWidth( ps );
    }
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
    // check if the bat is selected; otherwise, the entire cube moves!
    if ( isstopped_ || eventcatcher_->isHandled() ) return;

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb );

    if ( eventinfo.type == visBase::MouseClick )
    {
	if ( !eventinfo.pressed || !eventinfo.worldpickedpos.isDefined() )
	    return;
        pErrMsg( "finally" );
        // move the paddle with the mouse
	BinID bid;
	bid = SI().transform( eventinfo.worldpickedpos );
//	if ( !SI().isInside( bid, true ) )
	if ( !SI().inlRange(true).includes( bid.inl,true ) ||
		    !SI().crlRange(true).includes( bid.crl,true ) )           
	    return;

	setPaddlePosition( Coord3( eventinfo.worldpickedpos.x, 
		    eventinfo.worldpickedpos.y, SI().zRange( true ).stop ) );
    }
    else if ( eventinfo.type == visBase::Keyboard && eventinfo.pressed )
    {
        switch ( eventinfo.key )
	{
	    case ' ':
	    {
		// pause/resume
		ispaused_ = !ispaused_;
		break;
	    }
	    case OD::Q:
	    {
	        // quit
	        isstopped_ = true;
		break;
	    }
	    case SoKeyboardEvent::LEFT_ARROW:
	    {
		movePaddleLeft();
	        break;
	    }
	    case SoKeyboardEvent::RIGHT_ARROW:
	    {
		movePaddleRight();
	        break;
	    }
	    case SoKeyboardEvent::UP_ARROW:
	    {
		movePaddleUp();
		break;
	    }
	    case SoKeyboardEvent::DOWN_ARROW:
	    {
		movePaddleDown();
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


void BouncyDisplay::movePaddleLeft()
{
    // Later: To be more correct, subtract the paddle's width to prevent the 
    // overshoot.
    Coord3 pos = getPaddlePosition();
    pos.x = ( pos.x-mPaddleStep ) < SI().minCoord( true ).x 
	? SI().minCoord( true ).x : pos.x-mPaddleStep;
    setPaddlePosition( pos );
}


void BouncyDisplay::movePaddleRight()
{
    Coord3 pos = getPaddlePosition();
    pos.x = ( pos.x+mPaddleStep ) > SI().maxCoord( true ).x 
	? SI().maxCoord( true ).x : pos.x+mPaddleStep;
    setPaddlePosition( pos );
}


void BouncyDisplay::movePaddleUp()
{
    Coord3 pos = getPaddlePosition();
    pos.y = ( pos.y+mPaddleStep ) > SI().maxCoord( true ).y 
	? SI().maxCoord( true ).y : pos.y+mPaddleStep;
    setPaddlePosition( pos );
}


void BouncyDisplay::movePaddleDown()
{
    Coord3 pos = getPaddlePosition();
    pos.y = ( pos.y-mPaddleStep ) < SI().minCoord( true ).y 
	? SI().minCoord( true ).y : pos.y-mPaddleStep;
    setPaddlePosition( pos );
}


const visBase::Transformation* BouncyDisplay::getDisplayTransformation() const
{
    return bb_->getDisplayTransformation();
}


void BouncyDisplay::setDisplayTransformation( const visBase::Transformation* nt )
{
    bb_->setDisplayTransformation( nt );
    paddle_->setDisplayTransformation( nt );
}


void BouncyDisplay::setBallProperties( const visBeachBall::BallProperties& bp )
{
    bb_->setBallProperties( bp );
}
   

visBeachBall::BallProperties BouncyDisplay::getBallProperties() const
{
    return bb_->getBallProperties();
}
   

void BouncyDisplay::setBallPosition( const Coord3& centerpos )
{
    bb_->setCenterPosition( centerpos );
}


Coord3 BouncyDisplay::getBallPosition() const
{
    return bb_->getCenterPosition();
}


void BouncyDisplay::setPaddlePosition( const Coord3& pos )
{
    paddle_->setCenterPos( pos );
}


Coord3 BouncyDisplay::getPaddlePosition() const
{
    return paddle_->centerPos();
}


bool BouncyDisplay::ispaused() const
{
    return ispaused_;
}


bool BouncyDisplay::isstopped() const
{
    return isstopped_;
}


}


