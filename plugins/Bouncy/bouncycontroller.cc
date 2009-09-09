/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Karthika
 * DATE     : Sep 2009
-*/

static const char* rcsID = "$Id: bouncycontroller.cc,v 1.2 2009-09-09 07:56:07 cvskarthika Exp $";

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
    targetpos_ = pos + Coord3(10000, 20000, 0);
}


int BouncyController::nextStep()
{
    static int numMoves = 50;

    // This method will become complex later...

    if ( numMoves < 0 )
    {
	numMoves = 50;
        return Executor::Finished();
    }    
    else if ( numMoves == 50 )
	newposdelta_ = (targetpos_ - currpos_)/numMoves;
    
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
    currpos_ = result;
    /*BufferString bs;
    bs = toString( result.x );
    pErrMsg( bs );
    bs = toString( result.y );
    pErrMsg( bs );
    bs = toString( result.z );
    pErrMsg( bs );*/
    return result;
}


};

