/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "latlong.h"

#include "testprog.h"

bool testStringParsing()
{
    const double res = 58.4416;
    LatLong ll;
    ll.setFromString("058 26' 29.706\" N",true);
    mRunStandardTest( mIsEqual(ll.lat_,res,1e-3), "DMS Latitude double-quote" );

    ll.setFromString("058 26' 29.706'' N",true);
    mRunStandardTest( mIsEqual(ll.lat_,res,1e-3), "DMS Latitude single-quotes");

    ll.setFromString("58.4416N",true);
    mRunStandardTest( mIsEqual(ll.lat_,res,1e-3), "Decimal latitude");

    ll.setFromString("058 26' 29.706'' S",true);
    mRunStandardTest( mIsEqual(ll.lat_,-res,1e-3),
		      "DMS Latitude S single-quotes");

    ll.setFromString("58.4416S",true);
    mRunStandardTest( mIsEqual(ll.lat_,-res,1e-3), "Decimal S latitude");

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    return testStringParsing() ? 0 : 1;
}
