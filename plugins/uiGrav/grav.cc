/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Apr 2010
-*/

static const char* rcsID = "$Id: grav.cc,v 1.1 2010-04-20 10:00:59 cvsbert Exp $";

#include "grav.h"
#include <math.h>


const float Grav::G = 6.67428e-11;


float Grav::Bouguer( float h, float rho )
{
    static const float bougfact = 2 * M_PI * G;
    return bougfact * rho * h;
}


Grav::Block::Block( const Coord& c1, const Coord& c2,
			    Interval<float> zrg )
    : c0_(c1,zrg.start)
    , dx_((float)(c2.x-c1.x))
    , dy_((float)(c2.y-c1.y))
    , dz_(zrg.stop-zrg.start)
{
}


float Grav::Block::calcValue( const Coord3& pos, float rho ) const
{
    const float x1 = (float)(c0_.x - pos.x);
    const float y1 = (float)(c0_.y - pos.y);
    const float z1 = (float)(c0_.z - pos.z);
    const float x2 = x1 + dx_;
    const float y2 = y1 + dy_;
    const float z2 = z1 + dz_;
    float v = prim(x1,y1,z1) + prim(x1,y2,z2) + prim(x2,y2,z1) + prim(x2,y1,z2) 
	    - prim(x1,y1,z2) + prim(x1,y2,z1) + prim(x2,y1,z1) + prim(x2,y2,z2);
    if ( v < 0 ) v = 0;
    return G * rho * v;
}


float Grav::Block::prim( float x, float y, float z ) const
{
    const float yzsqr = y*y + z*z;
    const float r = sqrt( x*x + yzsqr );
    const float v1 = x * log( r + y ) + y * log( r + x );
    const float v2 = (yzsqr + r*y) / ((r+y) * sqrt(yzsqr));
    const float v3 = z * atanf( v2 / sqrt(1-v2*v2) );
    return x > 0 ? v1 - v3 : v1 + v3;
}
