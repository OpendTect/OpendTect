/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Nov 2007
 RCS:           $Id: latlong.cc,v 1.7 2008-01-09 13:54:34 cvsbert Exp $
________________________________________________________________________

-*/

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
    strcpy( str, "[" ); strcat( str, getStringFromDouble(0,lat_) );
    strcat( str, "," ); strcat( str, getStringFromDouble(0,lng_) );
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

    lat_ = atof( ptrlat );
    lng_ = atof( ptrlng );
    return true;
}


LatLong2Coord::LatLong2Coord()
    : lngdist_(mUdf(float))
    , latdist_(cAvgEarthRadius*cDeg2Rad)
    , scalefac_(1)
{
}


LatLong2Coord::LatLong2Coord( const Coord& c, const LatLong& l )
    : latdist_(cAvgEarthRadius*cDeg2Rad)
    , scalefac_(1)
{
    set( c, l );
}


void LatLong2Coord::set( const LatLong& ll, const Coord& c )
{
    refcoord_ = c; reflatlng_ = ll;
    lngdist_ = cDeg2Rad * cos( ll.lat_ * cDeg2Rad ) * cAvgEarthRadius;
}


LatLong LatLong2Coord::transform( const Coord& c ) const
{
    if ( mIsUdf(lngdist_) ) return reflatlng_;

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
    if ( mIsUdf(latdist_) ) return SI().minCoord(true);

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
    if ( !mIsZero(scalefac_-1,0.01) )
	str += "`ft";

    strcpy( s, str );
}


bool LatLong2Coord::use( const char* s )
{
    if ( !s || !*s ) return false;

    BufferString str( s );
    char* ptr = strchr( str.buf(), '=' );
    if ( !ptr ) return false;
    *ptr++ = '\0';
    Coord c; LatLong l;
    if ( !c.use(str) || !l.use(ptr) )
	return false;

    set( l, c );

    FileMultiString fms( ptr );
    bool isft = false;
    if ( fms.size() > 1 )
    {
	const char* unstr = fms[1];
	isft = *unstr == 'f' || *unstr == 'F';
    }
    setCoordsInFeet( isft );

    return true;
}
