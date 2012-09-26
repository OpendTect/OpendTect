/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2007
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "latlong.h"
#include "survinfo.h"
#include "separstr.h"
#include <math.h>

const double cAvgEarthRadius = 6367450;
const double cDeg2Rad = M_PI / 180;


Coord LatLong::transform( const LatLong& ll )
{
    return SI().latlong2Coord().transform( ll );
}


LatLong LatLong::transform( const Coord& c )
{
    return SI().latlong2Coord().transform( c );
}


void LatLong::fill( char* str ) const
{
    if ( !str ) return;
    strcpy( str, "[" );
    getStringFromDouble(0,lat_,str+1);
    strcat( str, "," );
    getStringFromDouble( 0, lng_, str+strlen(str) );
    strcat( str, "]" );
}


bool LatLong::use( const char* s )
{
    if ( !s || !*s ) return false;

    BufferString str( s );
    char* ptrlat = str.buf(); mSkipBlanks( ptrlat );
    if ( *ptrlat == '[' ) ptrlat++;
    char* ptrlng = strchr( ptrlat, ',' );
    if ( !ptrlng ) return false;
    *ptrlng++ = '\0';
    if ( !*ptrlng ) return false;
    char* ptrend = strchr( ptrlng, ']' );
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
    static const double one60th = 1. / 60.;
    static const double one3600th = one60th * one60th;
    v = d + one60th * m + one3600th * s;
}


LatLong2Coord::LatLong2Coord()
    : lngdist_(mUdf(float))
    , latdist_(cAvgEarthRadius*cDeg2Rad)
    , scalefac_(-1)
{
}


LatLong2Coord::LatLong2Coord( const Coord& c, const LatLong& l )
    : latdist_(cAvgEarthRadius*cDeg2Rad)
    , scalefac_(-1)
{
    set( c, l );
}


void LatLong2Coord::set( const LatLong& ll, const Coord& c )
{
    refcoord_ = c; reflatlng_ = ll;
    lngdist_ = cDeg2Rad * cos( ll.lat_ * cDeg2Rad ) * cAvgEarthRadius;
}


// We cannot put this in the constructor: that leads to disaster at startup
#define mPrepScaleFac() \
    if ( scalefac_ < 0 ) \
	const_cast<LatLong2Coord*>(this)->scalefac_ \
		= SI().xyInFeet() ? mFromFeetFactorF : 1;


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
    return Coord( refcoord_.x + scalefac_ * latlongdist.lng_ * lngdist_,
		  refcoord_.y + scalefac_ * latlongdist.lat_ * latdist_ );
}


void LatLong2Coord::fill( char* s ) const
{
    if ( !s ) return;

    BufferString str;
    refcoord_.fill( str.buf() );
    str += "=";
    reflatlng_.fill( str.buf() + str.size() );
    strcpy( s, str );
}


bool LatLong2Coord::use( const char* s )
{
    lngdist_ = mUdf(float);
    if ( !s || !*s ) return false;

    BufferString str( s );
    char* ptr = strchr( str.buf(), '=' );
    if ( !ptr ) return false;
    *ptr++ = '\0';
    Coord c; LatLong l;
    if ( !c.use(str) || !l.use(ptr) )
	return false;
    else if ( mIsZero(c.x,1e-3) && mIsZero(c.y,1e-3) )
	return false;

    set( l, c );
    return true;
}
