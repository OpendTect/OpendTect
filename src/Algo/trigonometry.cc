/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: trigonometry.cc,v 1.20 2004-01-05 15:18:07 nanne Exp $";

#include "trigonometry.h"

#include "errh.h"
#include "pca.h"
#include "position.h"
#include "sets.h"

#include <math.h>


TypeSet<Vector3>* makeSphereVectorSet( double dradius )
{
    TypeSet<Vector3>& vectors(*new TypeSet<Vector3>);


    const int nrdips = mNINT(M_PI_2/dradius)+1;
    const double ddip = M_PI_2/(nrdips-1);

    for ( int dipidx=0; dipidx<nrdips; dipidx++ )
    {
	const double dip = ddip*dipidx;
	static const double twopi = M_PI*2;
	const double radius = cos(dip);
	const double perimeter = twopi*radius;

	int nrazi = mNINT((dipidx ? perimeter : M_PI)/dradius);
	//!< No +1 since it's comes back to 0 when angle is 2*PI
	//!< The dipidx ? stuff is to avoid multiples 
	if ( !nrazi ) nrazi = 1;
	const double dazi = (dipidx ? twopi : M_PI)/nrazi;

	for ( int aziidx=0; aziidx<nrazi; aziidx++ )
	{
	    double azi = aziidx*dazi;
	    vectors += Vector3( cos(azi)*radius, sin(azi)*radius, sin(dip));
	}
    }

    return &vectors;
}


Coord3 estimateAverageVector( const TypeSet<Coord3>& vectors, bool normalize,
       		 	      bool checkforundefs )
{
    const int nrvectors = vectors.size();
    TypeSet<Coord3> ownvectors;
    if ( normalize || checkforundefs )
    {
	for ( int idx=0; idx<nrvectors; idx++ )
	{
	    const Coord3& vector = vectors[idx];
	    if ( checkforundefs && !vector.isDefined() )
		continue;

	    const double len = vector.abs();
	    if ( mIS_ZERO(len) )
		continue;

	    ownvectors += normalize ? vector/len : vector;
	}
    }

    const TypeSet<Coord3>& usedvectors =  normalize || checkforundefs
					 ? ownvectors : vectors;

    const int nrusedvectors = usedvectors.size();
    if ( !nrusedvectors )
	return Coord3( mUndefValue, mUndefValue, mUndefValue );

    if ( nrusedvectors==1 )
	return usedvectors[0];

    Coord3 average(0,0,0);
    for ( int idx=0; idx<nrusedvectors; idx++ )
	average += usedvectors[idx];

    const double avglen = average.abs();
    if ( !mIS_ZERO(avglen) )
	return average/avglen;

    PCA pca(3);
    for ( int idx=0; idx<nrusedvectors; idx++ )
	pca.addSample( usedvectors[idx] );
    
    if ( !pca.calculate() )
	return Coord3( mUndefValue, mUndefValue, mUndefValue );

    Coord3 res;
    pca.getEigenVector(0, res );

    int nrnegative = 0;
    for ( int idx=0; idx<nrusedvectors; idx++ )
    {
	if ( res.dot(usedvectors[idx])<0 ) nrnegative++;
    }

    if ( nrnegative*2> nrusedvectors )
	return -res;

    return res;
}


Quaternion::Quaternion( float s_, float x, float y, float z )
    : vec( x, y, z )
    , s( s_ )
{ }


Quaternion::Quaternion( float s_, const Vector3& vec_ )
    : vec( vec_ )
    , s( s_ )
{}


Quaternion Quaternion::operator+( const Quaternion& b ) const
{
    return Quaternion( s+b.s, vec+b.vec);
}


Quaternion& Quaternion::operator+=( const Quaternion& b )
{
    (*this) = (*this) + b;
    return *this;
}



Quaternion Quaternion::operator-( const Quaternion& b ) const
{
    return Quaternion( s-b.s, vec-b.vec);
}


Quaternion& Quaternion::operator-=( const Quaternion& b )
{
    (*this) = (*this) - b;
    return *this;
}




Quaternion Quaternion::operator*( const Quaternion& b ) const
{
    return Quaternion( s*b.s-vec.dot(b.vec),
	    s*b.vec + b.s*vec + vec.cross(b.vec));
}


Quaternion& Quaternion::operator*=( const Quaternion& b )
{
    (*this) = (*this) * b;
    return *this;
}


Quaternion Quaternion::inverse() const
{
    return Quaternion( s, Vector3( -vec.x, -vec.y, -vec.z ));
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


float Line3::distanceToPoint( const Coord3& point ) const
{
   Vector3 p0p1( point.x - x0, point.y - y0, point.z - z0 );
   Vector3 v( alpha, beta, gamma );

   return v.cross( p0p1 ).abs() / v.abs();
}


Plane3::Plane3() {}

							     
Plane3::Plane3( float A_, float B_, float C_, float D_ )
    : A( A_ )
    , B( B_ )
    , C( C_ )
    , D( D_ )
{}


Plane3::Plane3( const Coord3& vec, const Coord3& point, bool istwovec )
{
    set( vec, point, istwovec );
}


Plane3::Plane3( const Coord3& a, const Coord3& b, const Coord3& c )
{
    set( a, b, c );
}


Plane3::Plane3( const TypeSet<Coord3>& pts )
{
    set( pts );
}


void Plane3::set( const Vector3& norm, const Coord3& point, bool istwovec )
{
    if ( istwovec )
    {
	Vector3 cross = point.cross( norm );
	A = cross.x;
	B = cross.y;
	C = cross.z;
	D = 0;
    }
    else
    {
	A = norm.x;
	B = norm.y;
	C = norm.z;
	D =  -(norm.x*point.x) - (norm.y*point.y) - ( norm.z*point.z );
    }
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


float Plane3::set( const TypeSet<Coord3>& pts )
{
    const int nrpts = pts.size();
    if ( nrpts<3 )
    {
	A = 0; B=0; C=0; D=0;
	return 0;
    }
    if ( nrpts==3 )
    {
	set( pts[0], pts[1], pts[2] );
	return 1;
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

    if ( !pca.calculate() )
	return -1;

    Vector3 normal;
    pca.getEigenVector(2,normal);
    set( normal, midpt, false );
    return 1-pca.getEigenValue(2)/
	   (pca.getEigenValue(0)+pca.getEigenValue(0)+pca.getEigenValue(0));
}


bool Plane3::operator==(const Plane3& b ) const
{
    Vector3 a_vec = normal();
    Vector3 b_vec = normal();

    const double a_len = a_vec.abs();
    const double b_len = b_vec.abs();

    const bool a_iszero = mIS_ZERO( a_len );
    const bool b_iszero = mIS_ZERO( b_len );

    if ( a_iszero||b_iszero) 
    {
	if ( a_iszero&&b_iszero ) return true;
	pErrMsg("Zero-length Vector");
	return false;
    }

    float cross = 1-a_vec.dot(b_vec);

    if ( !mIS_ZERO(cross) ) return false;

    const double ddiff = D/a_len - b.D/b_len;

    return mIS_ZERO(ddiff);
}


bool Plane3::operator!=(const Plane3& b ) const
{
    return !((*this)==b);
}




float Plane3::distanceToPoint( const Coord3& point ) const
{
    Vector3 norm( normal() );
    const Line3 linetoplane( point, norm );

    Coord3 p0;
    if ( intersectWith( linetoplane, p0 ) ) 
    {
    	return point.distance(p0);
    }
    else 
        return 0;	
}


bool Plane3::intersectWith( const Line3& b, Coord3& res ) const
{
    const float denominator = ( b.alpha*A + b.beta*B + b.gamma*C );
    if ( mIS_ZERO( denominator ) )
	return false;

    const float t = -( A*b.x0 + B*b.y0 + C*b.z0 + D ) / denominator;
    
    Coord3 intersect( b.x0+b.alpha*t, b.y0+b.beta*t, b.z0+b.gamma*t ); 
    res = intersect;

    return true;
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

	
Sphere cartesian2Spherical( const Coord3& crd, bool math )
{
    float theta, phi;
    float rad = sqrt( crd.x*crd.x + crd.y*crd.y + crd.z*crd.z );
    if ( math )
    {
	theta = rad ? acos( crd.z / rad ) : 0;
	phi = atan2( crd.y, crd.x );
    }
    else
    {
	theta = rad ? asin( crd.z / rad ) : 0;
	phi = atan2( crd.x, crd.y );
    }

    return Sphere(rad,theta,phi);
}


Coord3 spherical2Cartesian( const Sphere& sph, bool math )
{
    float x, y, z;
    if ( math )
    {
	x = sph.radius * cos(sph.phi) * sin(sph.theta);
	y = sph.radius * sin(sph.phi) * sin(sph.theta);
	z = sph.radius * cos(sph.theta);
    }
    else
    {
	x = sph.radius * sin(sph.phi) * cos(sph.theta);
	y = sph.radius * cos(sph.phi) * cos(sph.theta);
	z = sph.radius * sin(sph.theta);
    }

    return Coord3(x,y,z);
}
