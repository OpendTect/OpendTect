/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2007
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "latlong.h"

#include "coordsystem.h"
#include "separstr.h"
#include "survinfo.h"

#include <math.h>
#include <string.h>


const double cAvgEarthRadius = 6367450;

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

    const FixedString nrstr = llstr.find( '.' );
    const int trailsz = nrstr.size();
    const int sz = llstr.size() - trailsz; // size of string before .
    if ( sz < 5 ) // Size never larger than 4 when in degrees
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
	int parsesz = 4 + trailsz;
	int offset = parsesz;
	strncpy(buf.getCStr(),llstr.buf()+(len-offset),parsesz);
	buf[parsesz] = '\0';
	float secs = toFloat(buf) / 100.f;

	// parse minutes
	buf.setEmpty();
	parsesz = 2;
	offset += parsesz;
	if ( parsesz <= 0 )
	    return false;
	strncpy(buf.getCStr(),llstr.buf()+(len-offset),parsesz);
	buf[parsesz] = '\0';
	int mins = toInt( buf );

	// parse degrees
	buf.setEmpty();
	parsesz = len - offset;
	if ( parsesz <= 0 )
	    return false;
	strncpy(buf.getCStr(),llstr.buf(),parsesz);
	buf[parsesz] = '\0';
	int degs = toInt( buf );
	if ( hasSW && degs>0 )
	    degs *= -1;

	if ( degs < 0 )
	{
	    mins *= -1;
	    secs *= -1;
	}

	setDMS( lat, degs, mins, secs );
    }

    return true;
}


// LatLong2Coord
LatLong2Coord::LatLong2Coord()
    : latdist_(cAvgEarthRadius*mDeg2RadD)
    , lngdist_(mUdf(float))
    , scalefac_(-1)
{
}


LatLong2Coord::LatLong2Coord( const Coord& c, const LatLong& l )
    : latdist_(cAvgEarthRadius*mDeg2RadD)
    , scalefac_(-1)
{
    set( c, l );
}


void LatLong2Coord::set( const LatLong& ll, const Coord& c )
{
    refcoord_ = c; reflatlng_ = ll;
    lngdist_ = mDeg2RadD * cos( ll.lat_ * mDeg2RadD ) * cAvgEarthRadius;
}


// We cannot put this in the constructor: that leads to disaster at startup
#define mPrepScaleFac() \
    if ( scalefac_ < 0 ) \
	const_cast<LatLong2Coord*>(this)->scalefac_ \
		= SI().xyInFeet() ? mFromFeetFactorD : 1;


LatLong LatLong2Coord::transform( const Coord& c ) const
{
    if ( !isOK() ) return reflatlng_;
    mPrepScaleFac();

    Coord coorddist( (c.x - refcoord_.x) * scalefac_,
		     (c.y - refcoord_.y) * scalefac_ );
    LatLong ll( reflatlng_.lat_ + coorddist.y / latdist_,
		reflatlng_.lng_ + coorddist.x / lngdist_ );

    if ( ll.lat_ > 90 )		ll.lat_ = 180 - ll.lat_;
    else if ( ll.lat_ < -90 )	ll.lat_ = -180 - ll.lat_;
    if ( ll.lng_ < -180 )	ll.lng_ = ll.lng_ + 360;
    else if ( ll.lng_ > 180 )	ll.lng_ = ll.lng_ - 360;

    return ll;
}


Coord LatLong2Coord::transform( const LatLong& ll ) const
{
    if ( !isOK() ) return SI().minCoord(true);
    mPrepScaleFac();

    const LatLong latlongdist( ll.lat_ - reflatlng_.lat_,
			       ll.lng_ - reflatlng_.lng_ );
    return Coord( refcoord_.x + latlongdist.lng_ * lngdist_ / scalefac_,
		  refcoord_.y + latlongdist.lat_ * latdist_ / scalefac_ );
}


const char* LatLong2Coord::toString() const
{
    mDeclStaticString( ret );
    ret.set( refcoord_.toString() ).add( "=" ).add( reflatlng_.toString() );
    return ret.buf();
}


bool LatLong2Coord::fromString( const char* s )
{
    lngdist_ = mUdf(float);
    if ( !s || !*s ) return false;

    BufferString str( s );
    char* ptr = str.find( '=' );
    if ( !ptr ) return false;
    *ptr++ = '\0';
    Coord c; LatLong l;
    if ( !c.fromString(str) || !l.fromString(ptr) )
	return false;
    else if ( mIsZero(c.x,1e-3) && mIsZero(c.y,1e-3) )
	return false;

    set( l, c );
    return true;
}
