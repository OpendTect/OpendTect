/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"

#include "crssystem.h"
#include "oddirs.h"
#include "moddepmgr.h"
#include "plugins.h"
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

static double mDefEpsCoord = 1e-3;

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
				   LatLong(51.3895381337606,6.24518053063547),
				   LatLong(55.7122706213698,6.59425554113735),
				   LatLong(55.7638250988626,2.49278307736069) };

// Second set, not to be mixed with the first
static const Coord ed50xy[] = { Coord(468264.5625,5698352.5),
				Coord(725867.0625,5698352.5),
				Coord(725867.0625,6180123.5),
				Coord(468264.5625,6180123.5) };
static const LatLong ed50ll[] = { LatLong(51.4344233527787,2.54347672444672),
				  LatLong(51.3903314270215,6.24641371865727),
				  LatLong(55.7129190137685,6.59561538613625),
				  LatLong(55.7645193631684,2.49425017922724) };
static const LatLong ed50aswgs84ll[] =
				{ LatLong(51.4335865642503,2.54213686033878),
				  LatLong(51.3895343145551,6.24515978817066),
				  LatLong(55.7122701473912,6.59425717215459),
				  LatLong(55.7638250930592,2.49278262567496) };
static const Coord ed50towgs84xy[] = {
				  Coord(468172.3690299022,5698142.583463500),
				  Coord(725772.9246333143,5698142.526723889),
				  Coord(725774.4537187048,6179910.885402368),
				  Coord(468173.4715217450,6179910.932361473) };
static const Coord wgs72xy[] =	{ Coord(468161.6635768350,5698138.098187800),
				  Coord(725762.2786731090,5698137.498521693),
				  Coord(725764.8572872828,6179905.962497715),
				  Coord(468163.8062655053,6179906.583238922) };

static const Coord wgs84n20xy[] = { Coord(732564.42,4892016.76) };
static const LatLong wgs84n20ll[] = { LatLong(44.1443427171,-60.0922380627) };

static const Coord nad27n20xy[] = { Coord(732510.57,4891795.58) };
static const LatLong nad27n20ll[] = { LatLong(44.1443174906,-60.0929967680) };

#define mRunTest( func, desc ) \
    if ( !(func) ) \
    { \
	errStream() << desc << "\tFAILED!\n"; \
	return false; \
    } \
    else \
    { \
	logStream() << desc << "\tSUCCESS!\n"; \
    }


static bool testCoordToLatLong( const Coord& pos, const LatLong& ll,
				const Coords::CoordSystem& pbs,
				const char* desc )
{
    const LatLong calcll = LatLong::transform( pos, false, &pbs );
    mRunTest( ll == calcll, desc );
    return true;
}


static bool coordIsEqual( const Coord pos1, const Coord pos2 )
{
    return mIsEqual(pos1.x,pos2.x,mDefEpsCoord) &&
	   mIsEqual(pos1.y,pos2.y,mDefEpsCoord);
}


static bool testLatLongToCoord( const LatLong& ll, const Coord& pos,
				const Coords::CoordSystem& pbs, bool wgs84,
				const char* desc )
{
    mRunTest( coordIsEqual(LatLong::transform(ll,wgs84,&pbs),pos), desc );
    return true;
}


static bool testReversibility( Coords::AuthorityCode crsid )
{
    const Coords::ProjectionBasedSystem pbs( crsid );
    if ( !pbs.isOK() )
	return false;

    const Coord* xy = nullptr;
    const LatLong* ll = nullptr;
    int sz = 0;
    BufferString coord2lldesc, ll2coorddesc;

#define mUsePosArray( xyarr, llarr ) \
    { \
	xy = xyarr; \
	ll = llarr; \
	sz = sizeof(xyarr) / sizeof(Coord); \
	coord2lldesc.add( "From ").add( #xyarr ).add( " to " ).add( #llarr ); \
	ll2coorddesc.add( "From ").add( #llarr ).add( " to " ).add( #xyarr ); \
    }

    if ( crsid == cWGS84ID() )
	mUsePosArray(wgs84xy,wgs84ll)
    else if ( crsid == cED50ID() )
	mUsePosArray(ed50xy,ed50ll)
    else if ( crsid == cWGS84N20ID() )
	mUsePosArray(wgs84n20xy,wgs84n20ll)
    else if ( crsid == cNAD27N20ID() )
	mUsePosArray(nad27n20xy,nad27n20ll)
    else
	return false;

    for ( int idx=0; idx<sz; idx++ )
    {
	if ( !testCoordToLatLong(xy[idx],ll[idx],pbs,coord2lldesc) ||
	     !testLatLongToCoord(ll[idx],xy[idx],pbs,false,ll2coorddesc) )
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
	mRunTest( retll == ed50aswgs84ll[idx], "From ed50xy to ed50aswgs84ll" );
	if ( !testLatLongToCoord(retll,ed50towgs84xy[idx],wgs84pbs,true,
		    "From ed50aswgs84ll to ed50towgs84xy") )
	    return false;
    }

    for ( int idx=0; idx<sz; idx++ )
    {
	mRunTest( coordIsEqual(ed50towgs84xy[idx],
		Coords::CoordSystem::convert(ed50xy[idx],ed50pbs,wgs84pbs)),
		"From ed50xy to ed50towgs84xy" );
	mRunTest( coordIsEqual(ed50towgs84xy[idx],
		  wgs84pbs.convertFrom(ed50xy[idx],ed50pbs)),
	          "From ed50xy to ed50towgs84xy" );
	mRunTest( coordIsEqual(wgs72xy[idx],
		Coords::CoordSystem::convert(ed50xy[idx],ed50pbs,wgs72pbs)),
		"From ed50xy to wgs72xy" );
	mRunTest( coordIsEqual(wgs72xy[idx],
		  wgs72pbs.convertFrom(ed50xy[idx],ed50pbs)),
		  "From ed50xy to wgs72xy" );
    }

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    PIM().loadAuto( false );
    OD::ModDeps().ensureLoaded( "CRS" );
    Coords::initCRSDatabase();
    Coords::ProjectionBasedSystem::initClass();
    PIM().loadAuto( true );

    if ( !testReversibility(cWGS84ID()) ||
	 !testReversibility(cED50ID()) ||
	 !testReversibility(cWGS84N20ID()) ||
	 !testReversibility(cNAD27N20ID()) ||
	 !testTransfer() )
	return 1;

    return 0;
}
