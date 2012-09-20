/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Karthika
 * DATE     : Sep 2009
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "bouncycontroller.h"
#include "statrand.h"

// number of moves to reach target
#define mMaxNumMoves 50

// animation timer interval in msec
#define mTimerInterval 10

namespace Bouncy
{

BouncyController::BouncyController()
    : newPosAvailable(this)  
    , currpos_(Coord3(0,0,0))  
    , newposdelta_(Coord3(0,0,0))
    , targetpos_(Coord3(3000,3000,0.5))
    , minpos_(Coord(0,0))
    , maxpos_(Coord(10000,10000))
    , zStart_(0)
    , zStop_(2) 
    , numlives_(3)
    , updatetimer_(0)
    , simulate_(true)
    , nummovesrem_(mMaxNumMoves)		    
{
}


BouncyController::~BouncyController()
{
    updatetimer_.stop();
}


void BouncyController::init( const Coord3& pos, const Coord& minpos, 
			     const Coord& maxpos, float zStart, float zStop, 
			     bool simulategame )
{
    currpos_ = pos;
    minpos_ = minpos;
    maxpos_ = maxpos;
    zStart_ = zStart;
    zStop_ = zStop;
    simulate_ = simulategame;

    nummovesrem_ = mMaxNumMoves;
    
    if ( updatetimer_.isActive() )
	updatetimer_.stop();
    
    if ( simulate_ )
        updatetimer_.tick.notify( mCB(this,BouncyController,simulateCB) );
    else
        updatetimer_.tick.notify( mCB(this,BouncyController,runCB) );
    
    updatetimer_.start( mTimerInterval, false );
}


void BouncyController::runCB( CallBacker* )
{
    // to do: fill in the real game logic. Bounce from wells, roll, etc.
}


// Callback of the animation timer. This runs a simulation of the ball within
// the box defined by the min and max coordinates of the survey. The ball
// deflects from imaginary objects in its path!
void BouncyController::simulateCB( CallBacker* )
{
    if ( nummovesrem_ == mMaxNumMoves )
    {
	Stats::RandGen::init();

	int randx = Stats::RandGen::getIndex(int(maxpos_.x - minpos_.x));
	int randy = Stats::RandGen::getIndex(int(maxpos_.y - minpos_.y));
	int randz = Stats::RandGen::getIndex(int((zStop_-zStart_)*100));
	  // multiply by 100 and later divide by 100 to get more precision

	// Later: to be more correct, subtract the ball's radius to get rid 
	// of the overshoot.
	
        targetpos_ = Coord3( minpos_.x+randx, minpos_.y+randy, 
		zStart_+randz/100.0 );
	newposdelta_ = (targetpos_ - currpos_)/mMaxNumMoves;
    }    
    else if ( nummovesrem_ < 0 )
	nummovesrem_ = mMaxNumMoves+1;
    
    // make ball move one step
    currpos_ += newposdelta_;
    newPosAvailable.trigger();
   
    nummovesrem_--;
}


void BouncyController::stop()
{
    updatetimer_.stop();
}


void BouncyController::pause( bool p )
{
    if ( p )
	updatetimer_.stop();
    else 
	updatetimer_.start( mTimerInterval, false );
}


void BouncyController::setPos( const Coord3& pos )
{
    currpos_ = pos;
}


Coord3 BouncyController::getPos() const
{
    return currpos_;
}


};

