/*+
* (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
* AUTHOR   : Prajjaval Singh
* DATE	   : Jul 2020
* FUNCTION :
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "odjson.h"
#include "testprog.h"
#include "ranges.h"

const StepInterval<int> interval_( 10, 20, 5 );
const char* sKeyInterval() { return "Interval"; }
OD::JSON::Object obj_;

void setInterval()
{
    obj_.set( sKeyInterval(), interval_ );
}


bool getInterval()
{
    StepInterval<int> interval;
    obj_.get( sKeyInterval(), interval );
    mRunStandardTest( interval_ == interval,
	"Checking SetInterval And GetInterval" );

    return true;
}


int main( int argc, char** argv )
{
    mInitTestProg();

    setInterval();
    if ( !getInterval() )
	ExitProgram( 1 );

    return ExitProgram( 0 );
}
