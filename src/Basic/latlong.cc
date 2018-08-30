/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2007
________________________________________________________________________

-*/

#include "latlong.h"

#include "coordsystem.h"
#include "separstr.h"
#include "survinfo.h"

#include <math.h>
#include <string.h>


// LatLong
bool LatLong::operator==( const LatLong& ll ) const
{
    return mIsEqual(ll.lat_,lat_,mDefEps) &&
	   mIsEqual(ll.lng_,lng_,mDefEps);
}


Coord LatLong::transform( const LatLong& ll, bool towgs84,
			  const Coords::CoordSystem* coordsys )
{
    if ( !coordsys )
	coordsys = SI().getCoordSystem();

    return coordsys ? coordsys->fromGeographic( ll, towgs84 ) : Coord::udf();
}


LatLong LatLong::transform( const Coord& c, bool towgs84,
			    const Coords::CoordSystem* coordsys )
{
    if ( !coordsys )
	coordsys = SI().getCoordSystem();

    return coordsys ? coordsys->toGeographic( c, towgs84 ) : LatLong::udf();
}


const char* LatLong::toString() const
{
    mDeclStaticString( ret );
    ret.set( "[" ).add( lat_ ).add( "," ).add( lng_ ).add( "]" );
    return ret.buf();
}


bool LatLong::fromString( const char* s )
{
    if ( !s || !*s ) return false;

    BufferString str( s );
    char* ptrlat = str.getCStr(); mSkipBlanks( ptrlat );
    if ( *ptrlat == '[' ) ptrlat++;
    char* ptrlng = firstOcc( ptrlat, ',' );
    if ( !ptrlng ) return false;
    *ptrlng++ = '\0';
    if ( !*ptrlng ) return false;
    char* ptrend = firstOcc( ptrlng, ']' );
    if ( ptrend ) *ptrend = '\0';

    lat_ = toDouble( ptrlat );
    lng_ = toDouble( ptrlng );
    return true;
}


void LatLong::getDMS( bool lat, int& d, int& m, float& s ) const
{
    double v = lat ? lat_ : lng_;
    d = (int)v;
    v -= d; v *= 60;
    m = (int)v;
    v -= m; v *= 60;
    s = (float)v;
}


void LatLong::setDMS( bool lat, int d, int m, float s )
{
    double& v = lat ? lat_ : lng_;
    const double one60th = 1. / 60.;
    const double one3600th = one60th * one60th;
    v = d + one60th * m + one3600th * s;
}


bool LatLong::setFromString( const char* str, bool lat )
{
// Supports strings formatted as dddmmssssh or ggg.gggggh
// h is not mandatory
    BufferString llstr = str;
    if ( llstr.isEmpty() )
	return false;

    char& lastchar = llstr[llstr.size()-1];
    const bool hasSW = lastchar=='S' || lastchar=='W';
    if ( lastchar=='N' || lastchar=='E' || hasSW )
	lastchar = '\0';

    const char* nrstr = llstr.find( '.' );
    if ( nrstr )
    {
	double& val = lat ? lat_ : lng_;
	val = toDouble( llstr );
	if ( hasSW && val>0 )
	    val *= -1;
    }
    else
    {
	const int len = llstr.size();

	// parse seconds
	BufferString buf;
	strncpy(buf.getCStr(),llstr.buf()+(len-4),4);
	buf[4] = '\0';
	const float secs = toFloat(buf) / 100.f;

	// parse minutes
	buf.setEmpty();
	strncpy(buf.getCStr(),llstr.buf()+(len-6),2);
	buf[2] = '\0';
	const int mins = toInt( buf );

	// parse degrees
	buf.setEmpty();
	strncpy(buf.getCStr(),llstr.buf(),len-6);
	buf[len-6] = '\0';
	int degs = toInt( buf );
	if ( hasSW && degs>0 )
	    degs *= -1;

	setDMS( lat, degs, mins, secs );
    }

    return true;
}
