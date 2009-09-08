/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Karthika
 * DATE     : Sep 2009
-*/

static const char* rcsID = "$Id: bouncycontroller.cc,v 1.1 2009-09-08 08:44:31 cvskarthika Exp $";

#include "bouncycontroller.h"

namespace Bouncy
{

BouncyController::BouncyController(const char* title)
    : Executor(title)
    , newPosAvailable(this)  
    , currpos_(Coord3(0,0,0))  
    , newposdelta_(Coord3(0,0,0))
    , targetpos_(Coord3(300,300,0.5))
    , numlives_(3)
{
}


BouncyController::~BouncyController()
{
}


void BouncyController::init( Coord3 pos )
{
    currpos_ = pos;
    targetpos_ = pos + Coord3(100, 200, 0);
}


int BouncyController::nextStep()
{
    static int numMoves = 50;

    // This method will become complex later...

    if ( numMoves == 0 )
        return Executor::Finished();
    else if ( numMoves == 50 )
	newposdelta_ = (targetpos_ - currpos_)/numMoves;
    //sleep(100); 
    // make ball move one step!
    newPosAvailable.trigger();
   
    numMoves--;
    return Executor::MoreToDo();
}


void BouncyController::stop()
{
}


Coord3 BouncyController::findNewPos( Coord3 pos )
{
    Coord3 result = currpos_ + newposdelta_;
    currpos_ = pos;
    return result;
}


};

