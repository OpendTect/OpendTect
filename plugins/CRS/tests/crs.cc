/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"

#include "crssystem.h"
#include "iopar.h"
#include "moddepmgr.h"
#include "oddirs.h"
#include "odjson.h"
#include "plugins.h"
#include "separstr.h"


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
static const char* ed50wkt =
"PROJCRS[\"ED50 / UTM zone 31N\",\n"
"    BASEGEOGCRS[\"ED50\",\n"
"        DATUM[\"European Datum 1950\",\n"
"            ELLIPSOID[\"International 1924\",6378388,297,\n"
"                LENGTHUNIT[\"metre\",1]]],\n"
"        PRIMEM[\"Greenwich\",0,\n"
"            ANGLEUNIT[\"degree\",0.0174532925199433]],\n"
"        ID[\"EPSG\",4230]],\n"
"    CONVERSION[\"UTM zone 31N\",\n"
"        METHOD[\"Transverse Mercator\",\n"
"            ID[\"EPSG\",9807]],\n"
"        PARAMETER[\"Latitude of natural origin\",0,\n"
"            ANGLEUNIT[\"degree\",0.0174532925199433],\n"
"            ID[\"EPSG\",8801]],\n"
"        PARAMETER[\"Longitude of natural origin\",3,\n"
"            ANGLEUNIT[\"degree\",0.0174532925199433],\n"
"            ID[\"EPSG\",8802]],\n"
"        PARAMETER[\"Scale factor at natural origin\",0.9996,\n"
"            SCALEUNIT[\"unity\",1],\n"
"            ID[\"EPSG\",8805]],\n"
"        PARAMETER[\"False easting\",500000,\n"
"            LENGTHUNIT[\"metre\",1],\n"
"            ID[\"EPSG\",8806]],\n"
"        PARAMETER[\"False northing\",0,\n"
"            LENGTHUNIT[\"metre\",1],\n"
"            ID[\"EPSG\",8807]]],\n"
"    CS[Cartesian,2],\n"
"        AXIS[\"(E)\",east,\n"
"            ORDER[1],\n"
"            LENGTHUNIT[\"metre\",1]],\n"
"        AXIS[\"(N)\",north,\n"
"            ORDER[2],\n"
"            LENGTHUNIT[\"metre\",1]],\n"
"    USAGE[\n"
"        SCOPE[\"Engineering survey, topographic mapping.\"],\n"
"        AREA[\"Europe - between 0°E and 6°E - Andorra; Denmark (North Sea); Germany offshore; Netherlands offshore; Norway including Svalbard - onshore and offshore; Spain - onshore (mainland and Balearic Islands); United Kingdom (UKCS) offshore.\"],\n"
"        BBOX[38.56,0,82.45,6.01]],\n"
"    ID[\"EPSG\",23031]]";

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


static bool testCoordToLatLong( const Coord& pos, const LatLong& ll,
				const Coords::CoordSystem& pbs,
				const char* desc )
{
    const LatLong calcll = LatLong::transform( pos, false, &pbs );
    mRunStandardTest( ll == calcll, desc );
    return true;
}


static bool coordIsEqual( const Coord pos1, const Coord pos2,
			  double eps=mDefEpsCoord )
{
    return mIsEqual(pos1.x,pos2.x,eps) &&
	   mIsEqual(pos1.y,pos2.y,eps);
}


static bool testLatLongToCoord( const LatLong& ll, const Coord& pos,
				const Coords::CoordSystem& pbs, bool wgs84,
				const char* desc )
{
    mRunStandardTest( coordIsEqual(LatLong::transform(ll,wgs84,&pbs),pos),desc);
    return true;
}


static bool testReversibility( const Coords::AuthorityCode& crsid )
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

    {
	const Coords::ProjectionBasedSystem wgs84n20pbs( cWGS84N20ID() );
	const Coords::ProjectionBasedSystem nad27n20pbs( cNAD27N20ID() );
	if ( !wgs84n20pbs.isOK() || !nad27n20pbs.isOK() )
	    return false;

	const Coord ret = Coords::CoordSystem::convert( nad27n20xy[0],
						    nad27n20pbs, wgs84n20pbs );
	mRunStandardTest( coordIsEqual(wgs84n20xy[0],ret,1e-2),
			  "From nad27n20xy to wgs84n20xy" );
    }

    const int sz = sizeof(wgs84xy) / sizeof(Coord);
    for ( int idx=0; idx<sz; idx++ )
    {
	const LatLong retll( LatLong::transform(ed50xy[idx],true,&ed50pbs) );
	mRunStandardTest( retll == ed50aswgs84ll[idx],
			  "From ed50xy to ed50aswgs84ll" );
	if ( !testLatLongToCoord(retll,ed50towgs84xy[idx],wgs84pbs,true,
		    "From ed50aswgs84ll to ed50towgs84xy") )
	    return false;
    }

    for ( int idx=0; idx<sz; idx++ )
    {
	mRunStandardTest( coordIsEqual(ed50towgs84xy[idx],
		Coords::CoordSystem::convert(ed50xy[idx],ed50pbs,wgs84pbs)),
		"From ed50xy to ed50towgs84xy" );
	mRunStandardTest( coordIsEqual(ed50towgs84xy[idx],
		  wgs84pbs.convertFrom(ed50xy[idx],ed50pbs)),
	          "From ed50xy to ed50towgs84xy" );
	mRunStandardTest( coordIsEqual(wgs72xy[idx],
		Coords::CoordSystem::convert(ed50xy[idx],ed50pbs,wgs72pbs)),
		"From ed50xy to wgs72xy" );
	mRunStandardTest( coordIsEqual(wgs72xy[idx],
		  wgs72pbs.convertFrom(ed50xy[idx],ed50pbs)),
		  "From ed50xy to wgs72xy" );
    }

    return true;
}


static const char* getAuthString( int codeid )
{
    mDeclStaticString(ret);

    FileMultiString fms;
    fms.add( Coords::AuthorityCode::sKeyEPSG() ).add( codeid );
    ret = fms.buf();
    return ret.buf();
}


static bool meterAndFeetCheck()
{
    ConstRefMan<Coords::CoordSystem> crs; BufferString msg;

    TypeSet<int> meterCRSIDs;
    meterCRSIDs.add( 2991 );
    meterCRSIDs.add( 3168 );
    meterCRSIDs.add( 8101 );
    meterCRSIDs.add( 29873 );
    for ( const auto& crsid : meterCRSIDs )
    {
	crs = Coords::CoordSystem::createSystem( getAuthString(crsid), msg );
	mRunStandardTest( crs && crs->isOK() && crs->isProjection(),
					  "Meter CRS is defined" );
	mRunStandardTest( crs->isMeter(), "Meter unit CRS returning meter" )
    }

    TypeSet<int> feetCRSIDs;
    feetCRSIDs.add( 2155 ); //ft
    feetCRSIDs.add( 2194 ); //Has been deprecated, just for test
    feetCRSIDs.add( 2228 );
    feetCRSIDs.add( 2232 ); //ftUS
    feetCRSIDs.add( 2239 ); //ftUS
    feetCRSIDs.add( 3454 ); //deprecated ftUS
    feetCRSIDs.add( 29872 ); //ftSe
    feetCRSIDs.add( 5754 ); //British Foot
    for ( const auto& crsid : feetCRSIDs )
    {
	crs = Coords::CoordSystem::createSystem( getAuthString(crsid), msg );
	mRunStandardTest( crs && crs->isOK() && crs->isProjection(),
					 "Feet CRS is defined" );
	mRunStandardTest( crs->isFeet(), "Feet unit CRS returning feet" )
    }

    TypeSet<int> yardCRSIDs;
    yardCRSIDs.add( 27292 ); // NZGD49 / South Island Grid
    for ( const auto& crsid : yardCRSIDs )
    {
	crs = Coords::CoordSystem::createSystem( getAuthString(crsid), msg );
	mRunStandardTest( crs && crs->isOK() && crs->isProjection(),
			  "Yard CRS is defined" );
	mRunStandardTest( crs->getUnitName().isEqual("yd",OD::CaseInsensitive),
			  "Yard unit CRS returning yard" );
    }

    TypeSet<int> diffCRSIDs;
    diffCRSIDs.add( 3167 ); //chain unit of measure
    diffCRSIDs.add( 24571 ); //chain unit of measure
    diffCRSIDs.add( 29871 ); //chain unit of measure
    for ( const auto& crsid : diffCRSIDs )
    {
	crs = Coords::CoordSystem::createSystem( getAuthString(crsid), msg );
	mRunStandardTest( crs && crs->isOK() && crs->isProjection(),
			  "Chain CRS is defined" );
	mRunStandardTest( !crs->isFeet() && !crs->isMeter(),
			  "CRS unit different from meter and feet" )
    }

    return true;
}


static bool testCRSIO()
{
    const Coords::ProjectionBasedSystem ed50pbs( cED50ID() );
    const Coords::Projection* ed50proj = ed50pbs.getProjection();
    mRunStandardTest( ed50pbs.isOK() && ed50proj && ed50proj->isOK(),
		      "Retrieve ED50 projection system" );

    const BufferString authstr = ed50proj->authCode().toString();
    const BufferString urnstr = ed50proj->authCode().toURNString();
    const BufferString usernm = ed50proj->userName();
    const BufferString dispstr = ed50proj->getProjDispString();
    const BufferString geodispstr = ed50proj->getGeodeticProjDispString();
    const BufferString wktstr = ed50proj->getWKTString();
    const BufferString jsonstr = ed50proj->getJSONString();

    BufferString jsonparsestr( jsonstr.buf() );
    OD::JSON::Object jsonobj;
    const uiRetVal uirv = jsonobj.parseJSon( jsonparsestr.getCStr(),
					     jsonparsestr.size() );

    mRunStandardTest( authstr == "EPSG`23031", "Authority string" );
    mRunStandardTest( urnstr == "urn:ogc:def:crs:EPSG::23031", "URN string");
    mRunStandardTest( usernm == "ED50 / UTM zone 31N", "User name" );
    mRunStandardTest( dispstr == "[EPSG:23031] ED50 / UTM zone 31N",
		      "Display string" );
    mRunStandardTest( geodispstr == "[EPSG:4230] ED50",
		      "Geodetic display string" );
    const StringView ed50str( ed50wkt );
    mRunStandardTest( wktstr == ed50wkt, "WKT string" );
    mRunStandardTest( uirv.isOK() && jsonobj.getStringValue("name") == usernm,
		      "JSON string - Name" )
    const OD::JSON::Object* jsonidobj = jsonobj.getObject("id");
    mRunStandardTest( jsonidobj && jsonidobj->getStringValue("authority") ==
		      Coords::AuthorityCode::sKeyEPSG(),
		      "JSON string - Authority" )
    mRunStandardTest( jsonidobj->getIntValue("code") == 23031,
		      "JSON string - Code" )

    mRunStandardTest( ed50pbs.getDescString() == urnstr, "toString(Default)" );
    mRunStandardTest( ed50pbs.getDescString(Coords::CoordSystem::URN) == urnstr,
			"toString(URN)" );
    mRunStandardTest( ed50pbs.getDescString(Coords::CoordSystem::WKT) == wktstr,
			"toString(WKT)" );
    mRunStandardTest( ed50pbs.getDescString(Coords::CoordSystem::JSON)==jsonstr,
			"toString(JSON)" );

    BufferString msg;
    ConstRefMan<Coords::CoordSystem> res =
			Coords::CoordSystem::createSystem(authstr.buf(),msg);
    mRunStandardTestWithError( res && res->isOK() && *res == ed50pbs,
			       "Coord system from authority string", msg.buf());

    res = Coords::CoordSystem::createSystem( urnstr.buf(), msg );
    mRunStandardTestWithError( res && res->isOK() && *res == ed50pbs,
			       "Coord system from URN string", msg.buf());

    res = Coords::CoordSystem::createSystem( usernm.buf(), msg );
    mRunStandardTestWithError( res && res->isOK() && *res == ed50pbs,
			       "Coord system from user name string", msg.buf());

    res = Coords::CoordSystem::createSystem( wktstr.buf(), msg );
    mRunStandardTestWithError( res && res->isOK() && *res == ed50pbs,
			       "Coord system from WKT string", msg.buf() );

    res = Coords::CoordSystem::createSystem( jsonstr.buf(), msg );
    mRunStandardTestWithError( res && res->isOK() && *res == ed50pbs,
			       "Coord system from JSON string", msg.buf() );

    return true;
}


static BufferString getAuthorityString( const char* jsonstr )
{
    BufferString jsonparsestr( jsonstr );
    OD::JSON::Object jsonobj;
    const uiRetVal uirv = jsonobj.parseJSon( jsonparsestr.getCStr(),
					     jsonparsestr.size() );
    if ( !uirv.isOK() )
	return BufferString::empty();

    const OD::JSON::Object* jsonidobj = jsonobj.getObject("id");
    if ( !jsonidobj || !jsonidobj->isPresent("authority") ||
	 !jsonidobj->isPresent("code") )
	return BufferString::empty();

    FileMultiString fms;
    fms.add( jsonidobj->getStringValue("authority") )
       .add( jsonidobj->getIntValue("code") );

    return BufferString( fms.buf() );
}


static bool testCRSRet()
{
    //Without CRS API

    BufferString msg;
    ConstRefMan<Coords::CoordSystem> res =
			Coords::CoordSystem::createSystem( ed50wkt, msg );
    mRunStandardTestWithError( res && res->isOK() && res->isProjection(),
			       "Coord system from WKT string", msg.buf() );
    const BufferString jsonstr = res->getDescString( Coords::CoordSystem::JSON);
    const BufferString authstr = getAuthorityString( jsonstr.buf() );
    mRunStandardTest( authstr == "EPSG`23031",
		      "Retrieved authority code from coordinate system" );

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProgDR();

    PIM().loadAuto( false );
    OD::ModDeps().ensureLoaded( "General" );
    PIM().loadAuto( true );

    if ( !testReversibility(cWGS84ID()) ||
	 !testReversibility(cED50ID()) ||
	 !testReversibility(cWGS84N20ID()) ||
	 !testReversibility(cNAD27N20ID()) ||
	 !testTransfer() ||
	 !meterAndFeetCheck() ||
	 !testCRSIO() ||
	 !testCRSRet() )
	return 1;

    return 0;
}
