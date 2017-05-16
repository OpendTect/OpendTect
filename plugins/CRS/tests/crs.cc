/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A. Huck
 * DATE     : May 2017
 * FUNCTION :
-*/


#include "testprog.h"

#include "projects.h"
#include "proj_api.h"

#include "crssystem.h"
#include "oddirs.h"
#include "survinfo.h"

static const char* sKeyRepoNm = "EPSG";
static Coords::ProjectionID cWGS84ID = Coords::ProjectionID::get( 32631 );
static Coords::ProjectionID cED50ID = Coords::ProjectionID::get( 23031 );

static double mDefEpsCoord = 1e-4;

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
				{ LatLong(51.4335905047152,2.54215313942492),
				  LatLong(51.3895376692996,6.24517875726681),
				  LatLong(55.7122701473909,6.59425717215963),
				  LatLong(55.7638250930593,2.49278262567497) };

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


static bool testCoordToLatLong( const Coord& pos, const LatLong& ll,
				const Coords::Projection& proj )
{
    mRunTest( ll.isEqualTo(proj.toGeographicWGS84(pos)) )
    return true;
}


static bool testLatLongToCoord( const LatLong& ll, const Coord& pos,
				const Coords::Projection& proj )
{
    mRunTest( mIsEqual(proj.fromGeographicWGS84(ll),pos,mDefEpsCoord) )
    return true;
}


static bool testWGS84()
{
    const Coords::ProjectionRepos* epsgrepos =
				Coords::ProjectionRepos::getRepos( sKeyRepoNm );
    const Coords::Projection* projwgs84 = epsgrepos->getByID( cWGS84ID );
    if ( !projwgs84 || !projwgs84->isOK() )
	return false;

    const int sz = sizeof(wgs84xy) / sizeof(Coord);
    for ( int idx=0; idx<sz; idx++ )
    {
	if ( !testCoordToLatLong(wgs84xy[idx],wgs84ll[idx],*projwgs84) ||
	     !testLatLongToCoord(wgs84ll[idx],wgs84xy[idx],*projwgs84) )
	    return false;
    }

    return true;
}


static bool testED50()
{
    const Coords::ProjectionRepos* epsgrepos =
				Coords::ProjectionRepos::getRepos( sKeyRepoNm );
    const Coords::Projection* projwgs84 = epsgrepos->getByID( cWGS84ID );
    const Coords::Projection* projed50 = epsgrepos->getByID( cED50ID );
    if ( !projwgs84 || !projwgs84->isOK() || !projed50 || !projed50->isOK() )
	return false;

    const projPJ proj4wgs84utm = pj_init_plus( projwgs84->defStr().str() );
    const projPJ proj4wgs84ll = pj_latlong_from_proj( proj4wgs84utm );
    const projPJ proj4ed50utm = pj_init_plus( projed50->defStr().str() );
    const projPJ proj4ed50ll = pj_latlong_from_proj( proj4ed50utm );

    const int sz = sizeof(wgs84xy) / sizeof(Coord);
    Coord tmppos;
    for ( int idx=0; idx<sz; idx++ )
    {
	tmppos = ed50xy[idx];
	pj_transform( proj4ed50utm, proj4ed50ll, 1, 1.f, &tmppos.x_,
							 &tmppos.y_, NULL );
	LatLong llret( tmppos.y_ * mRad2DegD, tmppos.x_ * mRad2DegD );
	mRunTest( llret.isEqualTo(ed50ll[idx]) )
        tmppos.x_ = ed50ll[idx].lng_ * mDeg2RadD;
	tmppos.y_ = ed50ll[idx].lat_ * mDeg2RadD;
	pj_transform( proj4ed50ll, proj4wgs84ll, 1, 1.f, &tmppos.x_,
							 &tmppos.y_, NULL );
	llret.lng_ = tmppos.x_ * mRad2DegD;
	llret.lat_ = tmppos.y_ * mRad2DegD;
	mRunTest( llret.isEqualTo(ed50aswgs84ll[idx]) );
    }

    pj_free( proj4wgs84utm );
    pj_free( proj4ed50utm );
    pj_free( proj4wgs84ll );
    pj_free( proj4ed50ll );

    return true;
}


bool initPlugin()
{
    Coords::ProjectionBasedSystem::initClass();
    Coords::ProjectionRepos* repos = new Coords::ProjectionRepos( sKeyRepoNm,
				    toUiString("Standard EPSG Projectons") );
    BufferString epsgfnm = mGetSetupFileName( "epsg" );
    repos->readFromFile( epsgfnm );
    Coords::ProjectionRepos::addRepos( repos );
    SI().readSavedCoordSystem();

    mRunTest( Coords::ProjectionRepos::getRepos(sKeyRepoNm) )

    return true;
}



int testMain( int argc, char** argv )
{
    mInitTestProg();

    if ( !initPlugin() || !testWGS84() || !testED50() )
	return 1;

    return 0;
}
