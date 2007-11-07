/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Nov 2007
 RCS:           $Id: latlong.cc,v 1.1 2007-11-07 16:06:10 cvsbert Exp $
________________________________________________________________________

-*/

#include "latlong.h"
#include "survinfo.h"
#include <math.h>

const double cRadiusEarthPoles = 6356750;
const double cRadiusEarthEquator = 6378135;
const double cDeg2Rad = M_PI / 180;


Coord LatLong::transform( const LatLong& ll )
{
    return SI().latlong2Coord().transform( ll );
}


LatLong LatLong::transform( const Coord& c )
{
    return SI().latlong2Coord().transform( c );
}


LatLong2Coord::LatLong2Coord()
    : latdist_(mUdf(float))
    , lngdist_(cDeg2Rad * cRadiusEarthPoles)
    , scalefac_(1)
{
}


LatLong2Coord::LatLong2Coord( const Coord& c, const LatLong& l )
    : lngdist_(cDeg2Rad * cRadiusEarthPoles)
    , scalefac_(1)
{
    set( c, l );
}


void LatLong2Coord::set( const LatLong& ll, const Coord& c )
{
    refcoord_ = c; reflatlng_ = ll;
    const double latrad = ll.lat_ * cDeg2Rad;
    const double sinlat = sin( latrad );
    latdist_ = (111320 + 373 * sinlat * sinlat) * cos( latrad );
}


LatLong LatLong2Coord::transform( const Coord& c ) const
{
    if ( mIsUdf(latdist_) ) return reflatlng_;

    Coord coorddist( (c.x - refcoord_.x) * scalefac_,
	    	     (c.y - refcoord_.y) * scalefac_ );
    return LatLong( reflatlng_.lat_ + coorddist.x / latdist_,
	   	    reflatlng_.lng_ + coorddist.y / lngdist_ );
}


Coord LatLong2Coord::transform( const LatLong& ll ) const
{
    if ( mIsUdf(latdist_) ) return SI().minCoord(true);

    const LatLong latlongdist( ll.lat_ - reflatlng_.lat_,
			       ll.lng_ - reflatlng_.lng_ );
    return Coord( refcoord_.x + scalefac_ * latlongdist.lat_ * latdist_,
		  refcoord_.y + scalefac_ * latlongdist.lng_ * lngdist_ );
}
