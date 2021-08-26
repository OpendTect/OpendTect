/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Wayne Mogg
 * DATE     : Aug 2021
 * FUNCTION :
-*/


#include "testprog.h"
#include "math2.h"
#include "ranges.h"


bool test_niceInterval()
{
    StepInterval<float> si;
    mRunStandardTest(si.niceInterval(1)==StepInterval<float>(),
							"si().niceInterval(1)");
    si.set(2.f, 2.f, 1.f);
    mRunStandardTest(si.niceInterval(10)==StepInterval<float>(1.6f, 2.4f, 0.1f),
					    "si(2, 2, 1).niceInterval(10)");
    si.set(2.f, 98.f, 1.f);
    mRunStandardTest(si.niceInterval(10)==StepInterval<float>(0.f, 100.f, 10.f),
					    "si(2, 98, 1).niceInterval(10)");
    si.set(98.f, 2.f, 1.f);
    mRunStandardTest(si.niceInterval(10)==StepInterval<float>(100.f, 0.f, 10.f),
					    "si(98, 2, 1).niceInterval(10)");
    mRunStandardTest(
		si.niceInterval(10, false)==StepInterval<float>(0.f,100.f,10.f),
					"si(98, 2, 1).niceInterval(10, false)");
    si.set(-2.f, 98.f, 1.f);
    mRunStandardTest(
		si.niceInterval(10)==StepInterval<float>(-10.f, 100.f, 10.f),
					    "si(-2, 98, 1).niceInterval(10)");
    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !test_niceInterval() )
	return 1;

    return 0;
}
