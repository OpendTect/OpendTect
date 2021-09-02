/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Apr 2010
-*/


#include "grav.h"
#include "math2.h"
#include <math.h>


const double Grav::G = 6.67428e-11;


double Grav::Bouguer( double h, double rho )
{
    const double bougfact = 4.1935738032002570e-10; // 2 * M_PI * G
    return bougfact * rho * h;
}


Grav::Block::Block( const Coord& c1, const Coord& c2,
			    Interval<double> zrg )
    : c0_(c1,zrg.start)
    , dx_(c2.x-c1.x)
    , dy_(c2.y-c1.y)
    , dz_(zrg.stop-zrg.start)
{
}


double Grav::Block::calcValue( const Coord3& pos, double rho ) const
{
    const double x1 = (float)(c0_.x - pos.x);
    const double y1 = (float)(c0_.y - pos.y);
    const double z1 = (float)(c0_.z - pos.z);
    const double x2 = x1 + dx_;
    const double y2 = y1 + dy_;
    const double z2 = z1 + dz_;
    double v = prim(x1,y1,z2) + prim(x1,y2,z1) + prim(x2,y1,z1) + prim(x2,y2,z2)
	    - prim(x1,y1,z1) - prim(x1,y2,z2) - prim(x2,y2,z1) - prim(x2,y1,z2);
    return G * rho * v;
}


double Grav::Block::prim( double x, double y, double z ) const
{
    const double dsqr = y*y + z*z;
    const double d = Math::Sqrt( dsqr );
    const double r = Math::Sqrt( x*x + dsqr );
    const double v1 = x * log( y + r ) + y * log( x + r );
    const double v2 = z * Math::ASin( (dsqr + y*r) / (d * (y + r)) );

    return x > 0 ? v1 - v2 : v1 + v2;
}
