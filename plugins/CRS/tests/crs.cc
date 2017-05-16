/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A. Huck
 * DATE     : May 2017
 * FUNCTION :
-*/


#include "testprog.h"

#include "crssystem.h"

static Coords::ProjectionID cWGS84ID = Coords::ProjectionID::get( 32631 );
static Coords::ProjectionID cED50ID = Coords::ProjectionID::get( 23031 );

// First set to be matched upon:
static const double wgs84x[] = { 468173.499868268, 725774.348579317,
				 725774.348579317, 468173.499868268 };
static const double wgs84y[] = { 5698143.01523399, 5698143.01523399,
				 6179910.93279991, 6179910.93279991 };
static const double wgs84long[] = { 2.5421530888879, 6.24518053063412,
				    6.59425554114235, 2.49278307736069 };
static const double wgs84lat[] = { 51.4335905100652, 51.3895381337605,
				   55.7122706213695, 55.7638250988626 };

// Second set, not to be mixed with the first
static const double ed50x[] = { 468264.5625, 725867.0625,
				725867.0625, 468264.5625 };
static const double ed50y[] = { 5698352.5, 5698352.5, 6180123.5, 6180123.5 };
static const double ed50long[] = { 2.54347672444672, 6.24641371865591,
				   6.59561538614129, 2.49425017922724 };
static const double ed50lat[] = { 51.4344233527787, 51.3903314270213,
				  55.7129190137682, 55.7645193631684 };
static const double ed50longwgs84[] = { 2.54215313942492, 6.24517875726681,
					6.59425717215963, 2.49278262567497 };
static const double ed50latwgs84[] = { 51.4335905047152, 51.3895376692996,
				       55.7122701473909, 55.7638250930593 };

#define mRunTest( func ) \
    if ( (func)==false ) \
    { \
	od_cout() << #func "\tfailed!\n"; \
	return false; \
    } \
    else if ( !quiet ) \
    { \
	od_cout() << #func "\tsuccess!\n"; \
    }



bool testXYtoLatLong()
{
    const ObjectSet<Coords::ProjectionRepos>& projs =
					Coords::ProjectionRepos::reposSet();
    if ( projs.isEmpty() )
	return false;

    TypeSet<Coord> utmxy;
    TypeSet<LatLong> utmlatlong;
    for ( int idx=0; idx<4; idx++ )
    {
	utmxy += Coord( wgs84x[idx], wgs84y[idx] );
	utmlatlong += LatLong( wgs84lat[idx], wgs84long[idx] );
    }

    const Coords::Projection* projwgs84 = projs[0]->getByID( cWGS84ID );
    if ( !projwgs84 )
	return false;

    for ( int idx=0; idx<utmxy.size(); idx++ )
    {
	mRunTest( utmlatlong[idx] == projwgs84->toGeographicWGS84(utmxy[idx]) )
	mRunTest( utmxy[idx] == projwgs84->fromGeographicWGS84(utmlatlong[idx]))
    }

    return true;
}


bool testLatLongToXY()
{

    return true;
}

int testMain( int argc, char** argv )
{
    mInitTestProg();

    if ( !testXYtoLatLong() || testLatLongToXY() )
	return 1;

    return 0;
}
