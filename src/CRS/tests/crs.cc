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
{ return Coords::AuthorityCode(sKeyRepoNm,32631); }
static Coords::AuthorityCode cED50ID()
{ return Coords::AuthorityCode(sKeyRepoNm,23031); }
static Coords::AuthorityCode cWGS72ID()
{ return Coords::AuthorityCode(sKeyRepoNm,32231); }
static Coords::AuthorityCode cWGS84N20ID()
{ return Coords::AuthorityCode(sKeyRepoNm,32620); }
static Coords::AuthorityCode cNAD27N20ID()
{ return Coords::AuthorityCode(sKeyRepoNm,26720); }

static double mDefEpsCoord = 1e-4;

/* Input points tested through:
http://epsg.io/transform
https://mygeodata.cloud/cs2cs

Can also best be checked using the proj applications on the command line:
Examples of X/Y to Lat/Long transformation within a CRS (EPSG:26720 -> EPSG:4267):
echo 732510.57 4891795.58 | proj -I +proj=utm +zone=20 +datum=NAD27 -s -f "%.7f"
echo 732510.57 4891795.58 | invproj +proj=utm +zone=20 +datum=NAD27 -s -f "%.7f"
echo 732510.57 4891795.58 | cs2cs +proj=utm +zone=20 +datum=NAD27 +to +proj=longlat +datum=NAD27 -s -f "%.7f"
echo 732510.57 4891795.58 | cs2cs EPSG:26720 EPSG:4267 -f "%.7f"
> 44.1443175	-60.0929968    ( cs2cs returns z as third output )

Examples of Lat/Long transformation within a CRS (EPSG:4267 -> EPSG:26720):
echo 44.1443175 -60.0929968 | proj -r +proj=utm +zone=20 +datum=NAD27 -f "%.7f"
> 732510.5674074  4891795.5809538
echo 44.1443175 -60.0929968 | cs2cs -r +proj=lonlat +datum=NAD27 +to +proj=utm +zone=20 +datum=NAD27
echo 44.1443175 -60.0929968 | cs2cs EPSG:4267 EPSG:26720
> 732510.57       4891795.58 0.00

Examples of transformations accross CRS:
X/Y NAD27 Zone 20N to Lat/Long WGS84:
echo 732510.57 4891795.58 | cs2cs +proj=utm +zone=20 +datum=NAD27 +to +proj=lonlat +datum=WGS84 -f "%.7f" -s
echo 732510.57 4891795.58 | cs2cs EPSG:26720 EPSG:4326 -f "%.7f"
> 44.1443426      -60.0922381 0.0000000

Lat/Long NAD27 to X/Y WGS84 Zone 20N:
echo 44.1443175 -60.0929968 | cs2cs -r +proj=lonlat +datum=NAD27 +to +proj=utm +zone=20 +datum=WGS84
echo 44.1443175 -60.0929968 | cs2cs EPSG:4267 EPSG:32620
> 732564.42  4892016.75 0.00

Lat/Long NAD27 to Lat/Long WGS84:
echo 44.1443175 -60.0929968 | cs2cs -r -s +proj=lonlat +datum=NAD27 +to +proj=lonlat +datum=WGS84 -f "%.7f"
echo 44.1443175 -60.0929968 | cs2cs EPSG:4267 EPSG:4326 -f "%.7f"
> 44.1443426      -60.0922381 0.0000000

X/Y NAD27 Zone 20N to X/Y WGS84 Zone 20N:
echo 732510.57 4891795.58 | cs2cs +proj=utm +zone=20 +datum=NAD27 +to +proj=utm +zone=20 +datum=WGS84
echo 732510.57 4891795.58 | cs2cs EPSG:26720 EPSG:32620
> 732564.42       4892016.75 0.00

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

static const Coord wgs84n20xy[] = { Coord(732564.42,4892016.76) };
static const LatLong wgs84n20ll[] = { LatLong(44.1443426,-60.0922381) };

static const Coord nad27n20xy[] = { Coord(732510.57,4891795.58) };
static const LatLong nad27n20ll[] = { LatLong(44.1443175,-60.0929968) };

#define mRunTest( func ) \
    if ( !(func) ) \
    { \
	errStream() << #func "\tfailed!\n"; \
	return false; \
    } \
    else \
    { \
	logStream() << #func "\tsuccess!\n"; \
    }


static bool testCoordToLatLong( const Coord& pos, const LatLong& ll,
				const Coords::CoordSystem& pbs )
{
    mRunTest( ll == LatLong::transform(pos,false,&pbs) );
    return true;
}


static bool coordIsEqual( const Coord pos1, const Coord pos2 )
{
    return mIsEqual(pos1.x,pos2.x,mDefEpsCoord) &&
	   mIsEqual(pos1.y,pos2.y,mDefEpsCoord);
}


static bool testLatLongToCoord( const LatLong& ll, const Coord& pos,
				const Coords::CoordSystem& pbs, bool wgs84 )
{
    mRunTest( coordIsEqual(LatLong::transform(ll,wgs84,&pbs),pos) );
    return true;
}


static bool testReversibility( Coords::AuthorityCode crsid )
{
    const Coords::ProjectionBasedSystem pbs( crsid );
    if ( !pbs.isOK() )
	return false;

    const Coord* xy = nullptr;
    const LatLong* ll = nullptr;
    if ( crsid == cWGS84ID() )
    {
	xy = wgs84xy;
	ll = wgs84ll;
    }
    else if ( crsid == cED50ID() )
    {
	xy = ed50xy;
	ll = ed50ll;

    }
    else if ( crsid == cWGS84N20ID() )
    {
	xy = wgs84n20xy;
	ll = wgs84n20ll;
    }
    else if ( crsid == cNAD27N20ID() )
    {
	xy = nad27n20xy;
	ll = nad27n20ll;
    }
    else
	return false;

    const int sz = sizeof(xy) / sizeof(Coord);
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

/*
   cross-conversions between NAD27 and WGS84 have severe errors >50m
   in the current version we build against, thus disabled this test
    const Coords::ProjectionBasedSystem wgs84n20pbs( cWGS84N20ID() );
    const Coords::ProjectionBasedSystem nad27n20pbs( cNAD27N20ID() );
    if ( !wgs84n20pbs.isOK() || !nad27n20pbs.isOK() )
	return false;

   mRunTest( coordIsEqual(wgs84n20xy[0],
	    Coords::CoordSystem::convert(nad27n20xy[0],
					nad27n20pbs,wgs84n20pbs)));*/

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
	mRunTest( coordIsEqual(ed50towgs84xy[idx],
		Coords::CoordSystem::convert(ed50xy[idx],ed50pbs,wgs84pbs)));
	mRunTest( coordIsEqual(ed50towgs84xy[idx],
		  wgs84pbs.convertFrom(ed50xy[idx],ed50pbs)) );
	mRunTest( coordIsEqual(wgs72xy[idx],
		Coords::CoordSystem::convert(ed50xy[idx],ed50pbs,wgs72pbs)));
	mRunTest( coordIsEqual(wgs72xy[idx],
		  wgs72pbs.convertFrom(ed50xy[idx],ed50pbs)) );
    }

    return true;
}


bool initPlugin()
{
    Coords::ProjectionBasedSystem::initClass();
    Coords::ProjectionRepos* repos = new Coords::ProjectionRepos( sKeyRepoNm,
				    toUiString("Standard EPSG Projectons") );
    BufferString epsgfnm = mGetSetupFileName( "CRS/epsg" );
    repos->readFromFile( epsgfnm );
    Coords::ProjectionRepos::addRepos( repos );
    SI().readSavedCoordSystem();

    mRunTest( Coords::ProjectionRepos::getRepos(sKeyRepoNm) )
    mRunTest( !Coords::ProjectionRepos::reposSet()[0]->isEmpty() )

    return true;
}



int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !initPlugin() ||
	 !testReversibility(cWGS84ID()) ||
	 !testReversibility(cED50ID()) ||
	 !testReversibility(cWGS84N20ID()) ||
	 !testReversibility(cNAD27N20ID()) ||
	 !testTransfer() )
	return 1;

    return 0;
}
