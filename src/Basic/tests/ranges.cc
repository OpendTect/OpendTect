/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Wayne Mogg
 * DATE     : Aug 2021
 * FUNCTION :
-*/


#include "testprog.h"
#include "math2.h"
#include "ranges.h"
#include "posinfo2d.h"

const float delf = 1e-7f;

bool test_niceInterval()
{
    StepInterval<float> si;
    mRunStandardTest(si.niceInterval(1)==StepInterval<float>(),
							"si().niceInterval(1)");
    si.set(2.f, 2.f, 1.f);
    mRunStandardTest(si.niceInterval(10).
			    isEqual(StepInterval<float>(1.6f, 2.4f, 0.1f),delf),
					    "si(2, 2, 1).niceInterval(10)");
    si.set(2.f, 98.f, 1.f);
    mRunStandardTest(si.niceInterval(10).
			isEqual(StepInterval<float>(0.f, 100.f, 10.f), delf),
					    "si(2, 98, 1).niceInterval(10)");
    si.set(98.f, 2.f, 1.f);
    mRunStandardTest(si.niceInterval(10).
			isEqual(StepInterval<float>(100.f, 0.f, 10.f), delf),
					    "si(98, 2, 1).niceInterval(10)");
    mRunStandardTest(si.niceInterval(10, false).
			    isEqual(StepInterval<float>(0.f,100.f,10.f), delf),
					"si(98, 2, 1).niceInterval(10, false)");
    si.set(-2.f, 98.f, 1.f);
    mRunStandardTest(si.niceInterval(10).
			isEqual(StepInterval<float>(-10.f, 100.f, 10.f),delf),
					    "si(-2, 98, 1).niceInterval(10)");
    si.set(-0.0600000024f, 0.0600000024f, 1.f);
    mRunStandardTest(si.niceInterval(5).
			isEqual(StepInterval<float>(-0.06f, 0.06f, 0.02f),delf),
				    "si(-0.06, 0.06, 1).niceInterval(5)");
    return true;
}


bool test_Line2D()
{
    PosInfo::Line2DData line2dv1, line2dv2;
    for ( int itrc=167; itrc<=2471; itrc++ )
	line2dv1.add( PosInfo::Line2DPos(itrc) );
    for ( int itrc=300; itrc<=2300; itrc+=2 )
	line2dv2.add( PosInfo::Line2DPos(itrc) );

    mRunStandardTest( line2dv1.isPresent(167) && line2dv1.isPresent(687) &&
		      line2dv1.isPresent(2471),
		      "Presence in standard PosInfo::Line2DData" );
    mRunStandardTest( !line2dv1.isPresent(100) && !line2dv1.isPresent(2500),
		      "Out of bounds in standard PosInfo::Line2DData" );
    mRunStandardTest( line2dv2.isPresent(300) && line2dv2.isPresent(900) &&
		      line2dv2.isPresent(2300),
		      "Presence in PosInfo::Line2DData with step 2" );
    mRunStandardTest( !line2dv2.isPresent(200) && !line2dv2.isPresent(2400) &&
		      !line2dv2.isPresent(349) && !line2dv2.isPresent(2151) &&
		      !line2dv2.isPresent(189) && !line2dv2.isPresent(2433),
		      "Out of bounds in PosInfo::Line2DData with step 2" );

    const StepInterval<int> trcrg1 = line2dv1.trcNrRange();
    const StepInterval<int> trcrg2 = line2dv2.trcNrRange();
    mRunStandardTest( trcrg1.includes(167,false) &&
		      trcrg1.includes(687,false) &&
	  mIsEqual(trcrg1.getfIndex(687),trcrg1.nearestIndex(687),1e-4f) &&
		      trcrg1.includes(2471,false),
		      "Trace ranges includes with step 1" );
    mRunStandardTest( !trcrg1.includes(100,false) &&
		      !trcrg1.includes(2500,false),
		      "Trace ranges out of bounds with step 1" );
    mRunStandardTest( trcrg2.includes(300,false) &&
		      trcrg2.includes(900,false) &&
	  mIsEqual(trcrg2.getfIndex(900),trcrg2.nearestIndex(900),1e-4f) &&
		      trcrg2.includes(2300,false),
		      "Trace ranges includes with step 2" );
    mRunStandardTest( !trcrg2.includes(200,false) &&
		      !trcrg2.includes(2400,false) &&
	  !mIsEqual(trcrg2.getfIndex(349),trcrg2.nearestIndex(349),1e-4f) &&
	  !mIsEqual(trcrg2.getfIndex(2151),trcrg2.nearestIndex(2151),1e-4f) &&
		      !trcrg2.includes(189,false) &&
		      !trcrg2.includes(2433,false),
		      "Trace ranges out of bounds with step 2" );

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !test_niceInterval() || !test_Line2D() )
	return 1;

    return 0;
}
