/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Karthika
 * DATE     : Sep 2009
-*/

static const char* rcsID = "$Id: bouncycontroller.cc,v 1.3 2009-09-09 11:44:37 cvskarthika Exp $";

#include "bouncycontroller.h"

namespace Bouncy
{

BouncyController::BouncyController(const char* title)
    : Executor(title)
    , newPosAvailable(this)  
    , currpos_(Coord3(0,0,0))  
    , newposdelta_(Coord3(0,0,0))
    , targetpos_(Coord3(3000,3000,0.5))
    , numlives_(3)
{
}


BouncyController::~BouncyController()
{
}


void BouncyController::init( Coord3 pos )
{
    currpos_ = pos;
    targetpos_ = pos + Coord3(10000, 20000, 0);
}


int BouncyController::nextStep()
{
    static int numMoves = 100;

    // This method will become complex later...

    if ( numMoves < 0 )
    {
	numMoves = 50;
        return Executor::Finished();
    }    
    else if ( numMoves == 50 )
	newposdelta_ = (targetpos_ - currpos_)/numMoves;
    
    // make ball move one step!
    currpos_ += newposdelta_;
    newPosAvailable.trigger();
   
    numMoves--;
    return Executor::MoreToDo();
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

