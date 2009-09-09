/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Karthika
 * DATE     : Sep 2009
-*/

static const char* rcsID = "$Id: bouncycontroller.cc,v 1.4 2009-09-09 15:22:38 cvskarthika Exp $";

#include "bouncycontroller.h"
#include "statrand.h"

namespace Bouncy
{

BouncyController::BouncyController()
    : newPosAvailable(this)  
    , currpos_(Coord3(0,0,0))  
    , newposdelta_(Coord3(0,0,0))
    , targetpos_(Coord3(3000,3000,0.5))
    , minpos_(Coord(0,0))
    , maxpos_(Coord(10000,10000))
    , numlives_(3)
    , updatetimer_(0)
    , simulate_(true)
{
}


BouncyController::~BouncyController()
{
    updatetimer_.stop();
}


void BouncyController::init( Coord3 pos, Coord minpos, Coord maxpos,
			     bool simulategame )
{
    currpos_ = pos;
    minpos_ = minpos;
    maxpos_ = maxpos;
    simulate_ = simulategame;

    if ( updatetimer_.isActive() )
	updatetimer_.stop();
    
    if ( simulate_ )
        updatetimer_.tick.notify( mCB(this,BouncyController,simulateCB) );
    else
        updatetimer_.tick.notify( mCB(this,BouncyController,runCB) );
    
    updatetimer_.start( 10, false );
}


void BouncyController::runCB( CallBacker* )
{
    // to do: fill in the real game logic. Bounce from wells, roll, etc.
}


void BouncyController::simulateCB( CallBacker* )
{
    const int maxnummoves = 50;
    static int nummovesrem = maxnummoves;

    if ( nummovesrem == maxnummoves )
    {
	Stats::RandGen::init();

	int randx = Stats::RandGen::getIndex(int(maxpos_.x - minpos_.x));
	int randy = Stats::RandGen::getIndex(int(maxpos_.y - minpos_.y));
	
        targetpos_ = Coord3( minpos_.x+randx, minpos_.y+randy, 0.5 );
	newposdelta_ = (targetpos_ - currpos_)/maxnummoves;
    }    
    else if ( nummovesrem < 0 )
	nummovesrem = maxnummoves+1;
    
    // make ball move one step
    currpos_ += newposdelta_;
    newPosAvailable.trigger();
   
    nummovesrem--;
}


void BouncyController::stop()
{
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

