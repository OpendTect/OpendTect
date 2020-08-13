/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A. Huck
 * DATE     : May 2017
 * FUNCTION :
-*/


#include "testprog.h"

#include "crssystem.h"
#include "oddirs.h"
#include "survinfo.h"


static const char* sKeyRepoNm = "EPSG";
static Coords::AuthorityCode cWGS84ID()
{ return Coords::AuthorityCode( sKeyRepoNm, Coords::ProjectionID::get(32631) );}
static Coords::AuthorityCode cED50ID()
{ return Coords::AuthorityCode( sKeyRepoNm, Coords::ProjectionID::get(23031) );}
static Coords::AuthorityCode cWGS72ID()
{ return Coords::AuthorityCode( sKeyRepoNm, Coords::ProjectionID::get(32231) );}

static double mDefEpsCoord = 1e-4;

/* Input points tested through:
http://epsg.io/transform
https://mygeodata.cloud/cs2cs
   */

// First set to be matched upon:
static const Coord wgs84xy[] = { Coord(468173.499868268,5698143.01523399),
				 Coord(725774.348579317,5698143.01523399),
				 Coord(725774.348579317,6179910.93279991),
				 Coord(468173.499868268,6179910.93279991) };
static const LatLong wgs84ll[] = { LatLong(51.4335905100652,2.5421530888879),
				   LatLong(51.3895381337605,6.24518053063412),
				   LatLong(55.7122706213695,6.59425554114235),
				   LatLong(55.7638250988626,2.49278307736069) };

// Second set, not to be mixed with the first
static const Coord ed50xy[] = { Coord(468264.5625,5698352.5),
				Coord(725867.0625,5698352.5),
				Coord(725867.0625,6180123.5),
				Coord(468264.5625,6180123.5) };
static const LatLong ed50ll[] = { LatLong(51.4344233527787,2.54347672444672),
				  LatLong(51.3903314270213,6.24641371865591),
				  LatLong(55.7129190137682,6.59561538614129),
				  LatLong(55.7645193631684,2.49425017922724) };
static const LatLong ed50aswgs84ll[] =
				{ LatLong(51.4335855247148,2.54212440807778),
				  LatLong(51.3895335885033,6.24515019573659),
				  LatLong(55.7122667919298,6.59422556382703),
				  LatLong(55.7638206858426,2.49275079504226) };
static const Coord ed50towgs84xy[] = {
				  Coord(468171.5027124056,5698142.473264048),
				  Coord(725772.2610005473,5698142.416446488),
				  Coord(725772.4885505616,6179910.409241279),
				  Coord(468171.4706640901,6179910.456494385) };
static const Coord wgs72xy[] =	{ Coord(468160.7972588778,5698137.987990135),
				  Coord(725761.6150399777,5698137.388245675),
				  Coord(725762.8921176088,6179905.486340908),
				  Coord(468161.8054061992,6179906.107376129) };

#define mRunTest( func ) \
    if ( !(func) ) \
    { \
	handleTestResult( false, #func ); \
	return false; \
    } \
    else \
    { \
	handleTestResult( true, #func ); \
    }


static bool testCoordToLatLong( const Coord& pos, const LatLong& ll,
				const Coords::CoordSystem& pbs )
{
    mRunTest( ll == LatLong::transform(pos,false,&pbs) );
    return true;
}


static bool testLatLongToCoord( const LatLong& ll, const Coord& pos,
				const Coords::CoordSystem& pbs, bool wgs84 )
{
    mRunTest( mIsEqual(LatLong::transform(ll,wgs84,&pbs),pos,mDefEpsCoord) )
    return true;
}


static bool testReversibility( bool wgs84 )
{
    const Coords::ProjectionBasedSystem pbs( wgs84 ? cWGS84ID() : cED50ID() );
    if ( !pbs.isOK() )
	return false;

    const Coord* xy = wgs84 ? wgs84xy : ed50xy;
    const LatLong* ll = wgs84 ? wgs84ll : ed50ll;
    const int sz = sizeof(wgs84xy) / sizeof(Coord);
    for ( int idx=0; idx<sz; idx++ )
    {
	if ( !testCoordToLatLong(xy[idx],ll[idx],pbs) ||
	     !testLatLongToCoord(ll[idx],xy[idx],pbs,false) )
	    return false;
    }

    return true;
}


static bool testTransfer()
{
    const Coords::ProjectionBasedSystem wgs84pbs( cWGS84ID() );
    const Coords::ProjectionBasedSystem ed50pbs( cED50ID() );
    const Coords::ProjectionBasedSystem wgs72pbs( cWGS72ID() );
    if ( !wgs84pbs.isOK() || !ed50pbs.isOK() || !wgs72pbs.isOK() )
	return false;

    const int sz = sizeof(wgs84xy) / sizeof(Coord);
    for ( int idx=0; idx<sz; idx++ )
    {
	const LatLong retll( LatLong::transform(ed50xy[idx],true,&ed50pbs) );
	mRunTest( retll == ed50aswgs84ll[idx] );
	if ( !testLatLongToCoord(retll,ed50towgs84xy[idx],wgs84pbs,true) )
	    return false;
    }

    for ( int idx=0; idx<sz; idx++ )
    {
	mRunTest( mIsEqual(ed50towgs84xy[idx],
		  Coords::CoordSystem::convert(ed50xy[idx],ed50pbs,wgs84pbs),
		  mDefEpsCoord) );
	mRunTest( mIsEqual(ed50towgs84xy[idx],
		  wgs84pbs.convertFrom(ed50xy[idx],ed50pbs),mDefEpsCoord) );
	mRunTest( mIsEqual(wgs72xy[idx],
		  Coords::CoordSystem::convert(ed50xy[idx],ed50pbs,wgs72pbs),
		  mDefEpsCoord) );
	mRunTest( mIsEqual(wgs72xy[idx],
		  wgs72pbs.convertFrom(ed50xy[idx],ed50pbs),mDefEpsCoord) );
    }

    return true;
}


bool initPlugin()
{
    Coords::ProjectionBasedSystem::initClass();
    Coords::ProjectionRepos::initStdRepos();
    SI().readSavedCoordSystem();

    mRunTest( Coords::ProjectionRepos::getRepos(sKeyRepoNm) )
    mRunTest( !Coords::ProjectionRepos::reposSet()[0]->isEmpty() )

    return true;
}



int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !initPlugin() ||
	 !testReversibility(true) || !testReversibility(false) ||
	 !testTransfer() )
	return 1;

    return 0;
}
