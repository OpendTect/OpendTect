/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: trigonometry.cc,v 1.1 2002-12-28 12:33:40 kristofer Exp $";

#include "trigonometry.h"

#include "pca.h"
#include "position.h"
#include "sets.h"

#include <math.h>


Vector3::Vector3( float x_, float y_, float z_ )
    : x( x_ )
    , y( y_ )
    , z( z_ )
{}


Vector3 Vector3::operator+(const Vector3& b) const
{
    return Vector3( x+b.x, y+b.y, z+b.z );
}


Vector3& Vector3::operator+=(const Vector3& b)
{
    (*this) = (*this) + b;
    return *this;
}


Vector3 Vector3::operator-(const Vector3& b) const
{
    return Vector3( x-b.x, y-b.y, z-b.z );
}


Vector3& Vector3::operator-=(const Vector3& b)
{
    (*this) = (*this) - b;
    return *this;
}


Vector3 Vector3::operator*(float f) const
{
    return Vector3( x*f, y*f, z*f );
}


Vector3& Vector3::operator*=(float f)
{
    (*this) = (*this) * f;
    return *this;
}


Vector3 Vector3::operator/(float f) const
{
    return Vector3( x/f, y/f, z/f );
}


Vector3& Vector3::operator/=(float f)
{
    (*this) = (*this) / f;
    return *this;
}


bool Vector3::operator==(const Vector3& b) const
{
    if ( mIS_ZERO( (*this).x - b.x ) && mIS_ZERO( (*this).y - b.y ) && 
         mIS_ZERO( (*this).z - b.z ) )
	return true;
    else
	return false;
}


float Vector3::dot( const Vector3& b ) const { return x*b.x + y*b.y + z*b.z; }


Vector3 Vector3::cross( const Vector3& b ) const
{
    return Vector3( y*b.z-z*b.y, z*b.x-x*b.z, x*b.y-y*b.x );
}


double Vector3::abs() const { return sqrt( x*x + y*y + z*z ); }


Vector3& Vector3::normalize()
{
    double absval = abs();
    x /= absval;
    y /= absval;
    z /= absval;
    return *this;
}


Vector3 operator*( float f, const Vector3& b )
{
    return Vector3( b.x*f, b.y*f, b.z*f );
}


Quarternion::Quarternion( float s_, float x, float y, float z )
    : vec( x, y, z )
    , s( s_ )
{ }


Quarternion::Quarternion( float s_, const Vector3& vec_ )
    : vec( vec_ )
    , s( s_ )
{}


Quarternion Quarternion::operator+( const Quarternion& b ) const
{
    return Quarternion( s+b.s, vec+b.vec);
}


Quarternion& Quarternion::operator+=( const Quarternion& b )
{
    (*this) = (*this) + b;
    return *this;
}



Quarternion Quarternion::operator-( const Quarternion& b ) const
{
    return Quarternion( s-b.s, vec-b.vec);
}


Quarternion& Quarternion::operator-=( const Quarternion& b )
{
    (*this) = (*this) - b;
    return *this;
}




Quarternion Quarternion::operator*( const Quarternion& b ) const
{
    return Quarternion( s*b.s-vec.dot(b.vec),
	    s*b.vec + b.s*vec + vec.cross(b.vec));
}


Quarternion& Quarternion::operator*=( const Quarternion& b )
{
    (*this) = (*this) * b;
    return *this;
}


Quarternion Quarternion::inverse() const
{
    return Quarternion( s, Vector3( -vec.x, -vec.y, -vec.z ));
}

Line3::Line3() {}

Line3::Line3( float _x0, float _y0, float _z0, float _alpha, float _beta,
	      float _gamma )
    : x0( _x0 )
    , y0( _y0 )
    , z0( _z0 )
    , alpha( _alpha )
    , beta( _beta )
    , gamma( _gamma )
{}


Line3::Line3( const Coord3& point, const Vector3& vector )
    : x0( point.x )
    , y0( point.y )
    , z0( point.z )
    , alpha( vector.x )
    , beta(vector.y )
    , gamma( vector.z )
{}


float Line3::distanceToPoint( Coord3& point ) const
{
   Vector3 p0p1( point.x - x0, point.y - y0, point.z - z0 );
   Vector3 v( alpha, beta, gamma );

   return v.cross( p0p1 ).abs() / v.abs();
}

							     
Plane3::Plane3( float _A, float _B, float _C, float _D )
    : A( _A )
    , B( _B )
    , C( _C )
    , D( _D )
{}


Plane3::Plane3( const Vector3& norm, const Coord3& point )
{
    set( norm, point );
}


Plane3::Plane3( const Vector3& v0, const Vector3& v1 )
{
    set( v0, v1 );
}


Plane3::Plane3( const Coord3& a, const Coord3& b, const Coord3& c )
{
    set( a, b, c );
}


Plane3::Plane3( const TypeSet<Coord3>& pts )
{
    set( pts );
}


void Plane3::set( const Vector3& norm, const Coord3& point )
{
    A = norm.x;
    B = norm.y;
    C = norm.z;
    D =  -(norm.x*point.x) - (norm.y*point.y) - ( norm.z*point.z );
}


void Plane3::set( const Vector3& v0, const Vector3& v1 )
{
    Vector3 norm = v1.cross( v0 );
    A = norm.x;
    B = norm.y;
    C = norm.z;
    D = 0;
}


void Plane3::set( const Coord3& a, const Coord3& b, const Coord3& c )
{
    Vector3 ab( b.x -a.x, b.y -a.y, b.z -a.z );
    Vector3 ac( c.x -a.x, c.y -a.y, c.z -a.z );

    Vector3 n = ab.cross( ac );

    A = n.x;
    B = n.y;
    C = n.z;
    D = A*(-a.x)  - B*a.y - C*a.z;
}


void Plane3::set( const TypeSet<Coord3>& pts )
{
    const int nrpts = pts.size();
    if ( nrpts<3 )
    {
	A = 0; B=0; C=0; D=0;
	return;
    }
    if ( nrpts==3 )
    {
	set( pts[0], pts[1], pts[2] );
	return;
    }

    PCA pca(3);
    Coord3 midpt( 0, 0, 0 );
    for ( int idx=0; idx<nrpts; idx++ )
    {
	pca.addSample( pts[idx] );
	midpt += pts[idx];
    }

    midpt.x /= nrpts;
    midpt.y /= nrpts;
    midpt.z /= nrpts;

    pca.calculate();
    Vector3 normal = pca.getEigenVector(3);

    set( normal, midpt );
}


float Plane3::distanceToPoint( const Coord3& point ) const
{
    Vector3 norm( normal() );
    Line3 const linetoplane( point.x, point.y, point.z, norm.x, 
	    		     norm.y, norm.z );

    Coord3 p0;
    if ( intersectWith( linetoplane, p0 ) ) 
    {
    	return sqrt( (point.x-p0.x)*(point.x-p0.x) + 
		     (point.y-p0.y)*(point.y-p0.y) +			
	             (point.z-p0.z)*(point.z-p0.z) );
    }
    else 
        return 0;	
}


bool Plane3::intersectWith( const Line3& b, Coord3& res ) const
{
    const float t = -( A*b.x0 + B*b.y0 + C*b.z0 + D ) / 
		     ( b.alpha*A + b.beta*B + b.gamma*C );
    
    if ( mIS_ZERO(t) )
	return false;
    else
    {
    	Coord3 intersect( b.x0+b.alpha*t, b.y0+b.beta*t, b.z0+b.gamma*t ); 
    	res = intersect;
    
    	return true;
    }
}


bool Plane3::intersectWith( const Plane3& b, Line3& res ) const
{
    const float detbc = B*b.C-b.C*B;
    const float detca = C*b.A-b.C*A;
    const float detab = A*b.B-b.A*B;

    if ( mIS_ZERO(detbc) && mIS_ZERO(detca) && mIS_ZERO(detab) ) return false; 

    res.alpha = detbc; res.beta=detca; res.gamma=detab;

    const float denumerator = detbc*detbc+detca*detca+detab*detab;

    const float detdc = D*b.C-b.D*C;
    const float detdb = D*b.B-b.D*B;
    const float detda = D*b.A-b.D*A;

    res.x0 = (res.beta*detdc-res.gamma*detdb)/denumerator;
    res.y0 = (res.gamma*detda-res.alpha*detdc)/denumerator;
    res.z0 = (res.alpha*detdb-res.beta*detda)/denumerator;

    return true;
}

	


    
    

    

