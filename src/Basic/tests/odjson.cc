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

#define mRetResult( funcname ) \
    { \
	od_cout() << funcname << " failed" << od_endl; \
	return false; \
    } \
    else if ( !quiet ) \
	od_cout() << funcname << " succeeded" << od_endl; \
    return true;

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
    if ( (interval_.start != interval.start) &&
	(interval_.stop != interval.stop) &&
	(interval_.step != interval.step) )
	mRetResult( "Checking SetInterval And GetInterval" );

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
