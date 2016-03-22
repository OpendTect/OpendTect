/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/


#include "trigonometry.h"

#include "math2.h"
#include "pca.h"
#include "position.h"
#include "typeset.h"

#include <math.h>

static const Sphere nullsphere_;
const Sphere& Sphere::nullSphere() { return nullsphere_; }

bool Sphere::isNull() const
{
    return mIsZero(radius,mDefEpsF)
        && mIsZero(theta,mDefEpsF)
	&& mIsZero(phi,mDefEpsF);
}


TypeSet<Vector3>* makeSphereVectorSet( double dradius )
{
    TypeSet<Vector3>& vectors(*new TypeSet<Vector3>);

    const int nrdips = mNINT32(M_PI_2/dradius)+1;
    const double ddip = M_PI_2/(nrdips-1);
    const double twopi = M_2PI;

    for ( int dipidx=0; dipidx<nrdips; dipidx++ )
    {
	const double dip = ddip*dipidx;
	const double radius = cos(dip);
	const double perimeter = twopi*radius;

	int nrazi = mNINT32((dipidx ? perimeter : M_PI)/dradius);
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
	    if ( mIsZero(len,mDefEps) )
		continue;

	    ownvectors += normalize ? vector/len : vector;
	}
    }

    const TypeSet<Coord3>& usedvectors =  normalize || checkforundefs
					 ? ownvectors : vectors;

    const Coord3 udfcrd3( mUdf(double), mUdf(double), mUdf(double) );
    const int nrusedvectors = usedvectors.size();
    if ( !nrusedvectors )
	return udfcrd3;

    if ( nrusedvectors==1 )
	return usedvectors[0];

    Coord3 average(0,0,0);
    for ( int idx=0; idx<nrusedvectors; idx++ )
	average += usedvectors[idx];

    const double avglen = average.abs();
    if ( !mIsZero(avglen,mDefEps) )
	return average/avglen;

    PCA pca(3);
    for ( int idx=0; idx<nrusedvectors; idx++ )
	pca.addSample( usedvectors[idx] );

    if ( !pca.calculate() )
	return udfcrd3;

    Coord3 res;
    pca.getEigenVector(0, res );

    int nrnegative = 0;
    for ( int idx=0; idx<nrusedvectors; idx++ )
    {
	if ( res.dot(usedvectors[idx])<0 )
	    nrnegative++;
    }

    if ( nrnegative*2> nrusedvectors )
	return -res;

    return res;
}


Quaternion::Quaternion( double s, double x, double y, double z )
    : vec_( x, y, z )
    , s_( s )
{ }


Quaternion::Quaternion( const Vector3& axis, float angle )
{
    setRotation( axis, angle );
}


void Quaternion::setRotation( const Vector3& axis, float angle )
{
    const float halfangle = angle/2;
    s_ = cos(halfangle);
    const float sineval = sin(halfangle);

    const Coord3 a = axis.normalize();

    vec_.x = a.x * sineval;
    vec_.y = a.y * sineval;
    vec_.z = a.z * sineval;
}


void Quaternion::getRotation( Vector3& axis, float& angle ) const
{
    if ( s_>=1 || s_<=-1 ) angle = 0;
    else angle = (float) Math::ACos( s_ ) * 2;

    //This should really be axis=vec_/sin(angle/2)
    //but can be simplified to this since length of axis is irrelevant
    axis = vec_;
}


Coord3 Quaternion::rotate( const Coord3& v ) const
{
    const Coord3 qvv = s_*v + vec_.cross(v);

    const Coord3 iqvec = -vec_;

    return (iqvec.dot(v))*iqvec+s_*qvv+qvv.cross(iqvec);
}


Quaternion Quaternion::operator+( const Quaternion& b ) const
{
    const Vector3 vec = vec_+b.vec_;
    return Quaternion( s_+b.s_, vec.x, vec.y, vec.z );
}


Quaternion& Quaternion::operator+=( const Quaternion& b )
{
    (*this) = (*this) + b;
    return *this;
}



Quaternion Quaternion::operator-( const Quaternion& b ) const
{
    const Vector3 vec =  vec_-b.vec_;
    return Quaternion( s_-b.s_, vec.x, vec.y, vec.z );
}


Quaternion& Quaternion::operator-=( const Quaternion& b )
{
    (*this) = (*this) - b;
    return *this;
}


Quaternion Quaternion::operator*( const Quaternion& b ) const
{
    const Vector3 vec = s_*b.vec_ + b.s_*vec_ + vec_.cross(b.vec_);
    return Quaternion( s_*b.s_-vec_.dot(b.vec_), vec.x, vec.y, vec.z );
}


Quaternion& Quaternion::operator*=( const Quaternion& b )
{
    (*this) = (*this) * b;
    return *this;
}


Quaternion Quaternion::inverse() const
{
    return Quaternion( s_, -vec_.x, -vec_.y, -vec_.z );
}


Line2::Line2()
{
    p0_ = Coord::udf();
    dir_ = Coord::udf();
}


Line2::Line2( const Coord& strt, const Coord& stp )
{
    p0_ = strt;
    dir_ = stp-strt;
}



bool Line2::operator==( const Line2& line ) const
{
    //Check if direction is same
    const double dotprod = direction(true).dot(line.direction(true));

    if ( !mIsEqual(dotprod,1,mDefEps) )
	return false;

    const double sqdist = sqDistanceToPoint( line.p0_ );

    return mIsZero(sqdist,mDefEps);
}

Line2 Line2::fromPosAndDir( const Coord& p0, const Coord& dir )
{
    Line2 res;
    res.p0_ = p0;
    res.dir_ = dir;
    return res;
}


static const double oneplus = 1.0 + mDefEpsD;


Coord Line2::intersection( const Line2& line, bool checkinlimits) const
{
    if ( mIsEqual(direction(true).dot(line.direction(true)),1.0, mDefEpsD) )
	return Coord::udf();

    double devisor = line.dir_.x*dir_.y-line.dir_.y*dir_.x;
    if ( mIsZero( devisor, mDefEpsD) )
	return Coord::udf();

    //Compute point on line that crosses this.
    const double u =
	(dir_.x * (line.p0_.y-p0_.y) +dir_.y*(p0_.x-line.p0_.x))/ devisor ;
    if ( checkinlimits )
    {
	if ( u<-mDefEpsD || u>oneplus )
	    return Coord::udf();

	devisor = dir_.x*line.dir_.y-dir_.y*line.dir_.x;
	if ( mIsZero( devisor, mDefEpsD) )
	    return Coord::udf();

	//Compute corresponding position on this line
	const double t =
	    (line.dir_.x*(p0_.y-line.p0_.y) +line.dir_.y*(line.p0_.x-p0_.x)) /
		devisor;

	if ( t<-mDefEpsD || t>oneplus )
	     return Coord::udf();
    }

    return line.getPoint( u );
}


void Line2::getPerpendicularLine( Line2& line, const Coord& point ) const
{
    line.dir_.x = -dir_.y;
    line.dir_.y = dir_.x;
    line.p0_ = point;
}


void Line2::getParallelLine( Line2& line, double dist ) const
{
    line = *this;
    Coord movement = Coord(-dir_.y,dir_.x).normalize() * dist;
    line.p0_ += movement;
}


Line3::Line3() {}

Line3::Line3( double x0, double y0, double z0, double alpha, double beta,
	      double gamma )
{
    p0_.x = x0;
    p0_.y = y0;
    p0_.z = z0;
    dir_.x = alpha;
    dir_.y = beta;
    dir_.z = gamma;
}


Line3 Line3::fromPosAndDir( const Coord3& p0, const Vector3& dir )
{
    Line3 res;
    res.p0_ = p0;
    res.dir_ = dir;
    return res;
}


Line3::Line3( const Coord3& strt, const Coord3& stp )
{
    p0_ = strt;
    dir_ = stp-strt;
}


void Line3::closestPointToLine( const Line3& line, double& t_this,
				double& t_line ) const
{
    const Coord3 dir0 = direction( false );
    const Coord3 dir1 = line.direction( false );
    const Coord3 diff = p0_ - line.p0_;

    const double d0 = dir0.dot(dir0);
    const double d1 = dir1.dot(dir1);
    const double d01 = dir0.dot(dir1);
    const double di0 = diff.dot(dir0);
    const double di1 = diff.dot(dir1);
    const double det = d01/d0 - d1/d01;

    t_line = mIsZero(det,1e-8) ? 0.5 : (di0/d0-di1/d01)/det;
    t_this = (t_line*d01-di0)/d0;
}


bool Line3::intersectWith( const Plane3& b, double& t ) const
{
    const double denominator = dir_.x*b.A_ + dir_.y*b.B_ + dir_.z*b.C_;
    const double dist0 = b.A_*p0_.x + b.B_*p0_.y + b.C_*p0_.z + b.D_;
    if ( mIsZero(denominator,mDefEps) )
    {
	const double test = dist0/Math::Sqrt(b.A_*b.A_+b.B_*b.B_+b.C_*b.C_);
	if ( mIsZero( test, mDefEps) )
	{
	    t = 0;
	    return true;
	}

	return false;
    }

    t = -dist0 / denominator;

    return true;
}


Plane3::Plane3() {}


Plane3::Plane3( double A, double B, double C, double D )
    : A_( A )
    , B_( B )
    , C_( C )
    , D_( D )
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
	A_ = cross.x;
	B_ = cross.y;
	C_ = cross.z;
	D_ = 0;
    }
    else
    {
	A_ = norm.x;
	B_ = norm.y;
	C_ = norm.z;
	D_ =  -(norm.x*point.x) - (norm.y*point.y) - ( norm.z*point.z );
    }
}


void Plane3::set( const Coord3& a, const Coord3& b, const Coord3& c )
{
    Vector3 ab( b.x -a.x, b.y -a.y, b.z -a.z );
    Vector3 ac( c.x -a.x, c.y -a.y, c.z -a.z );

    Vector3 n = ab.cross( ac );

    A_ = n.x;
    B_ = n.y;
    C_ = n.z;
    D_ = A_*(-a.x)  - B_*a.y - C_*a.z;
}


float Plane3::set( const TypeSet<Coord3>& pts )
{
    const int nrpts = pts.size();
    if ( nrpts<3 )
    {
	A_ = 0; B_=0; C_=0; D_=0;
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

    Vector3 norm;
    pca.getEigenVector(2,norm);
    set( norm, midpt, false );
    return 1-pca.getEigenValue(2)/
	   (pca.getEigenValue(0)+pca.getEigenValue(0)+pca.getEigenValue(0));
}


bool Plane3::operator==(const Plane3& b ) const
{
    Vector3 a_vec = normal();
    Vector3 b_vec = normal();

    const double a_len = a_vec.abs();
    const double b_len = b_vec.abs();

    const bool a_iszero = mIsZero(a_len,mDefEps);
    const bool b_iszero = mIsZero(b_len,mDefEps);

    if ( a_iszero||b_iszero)
    {
	if ( a_iszero&&b_iszero )
	    return true;
	pErrMsg("Zero-length Vector"); return false;
    }

    const double cross = 1-a_vec.dot(b_vec);

    if ( !mIsZero(cross,mDefEps) ) return false;

    const double ddiff = D_/a_len - b.D_/b_len;

    return mIsZero(ddiff,mDefEps);
}


bool Plane3::operator!=(const Plane3& b ) const
{
    return !((*this)==b);
}


Coord3 Plane3::getProjection( const Coord3& pos )
{
    const double param = (A_*pos.x+B_*pos.y+C_*pos.z+D_)/(A_*A_+B_*B_+C_*C_);
    return Coord3( pos.x-A_*param, pos.y-B_*param, pos.z-C_*param );
}


bool Plane3::onSameSide( const Coord3& p1, const Coord3& p2 )
{ return (A_*p1.x+B_*p1.y+C_*p1.z+D_) * (A_*p2.x+B_*p2.y+C_*p2.z+D_) >= 0; }


double Plane3::distanceToPoint( const Coord3& point, bool whichside ) const
{
    Vector3 norm( normal().normalize() );
    const Line3 linetoplane = Line3::fromPosAndDir( point, norm );

    Coord3 p0;
    if ( intersectWith( linetoplane, p0 ) )
    {
	const Coord3 diff = point-p0;
	return whichside ? diff.dot(norm) : diff.abs();
    }
    else
	return 0;
}


bool Plane3::intersectWith( const Line3& b, Coord3& res ) const
{
    double t;
    if ( !b.intersectWith( *this, t ) )
	return false;

    res = b.getPoint( t );

    return true;
}


bool Plane3::intersectWith( const Plane3& b, Line3& res ) const
{
    const Coord3 normal0(A_, B_, C_);
    const Coord3 normal1(b.A_, b.B_, b.C_);
    const Coord3 dir = normal0.cross( normal1 );
    if ( mIsZero(dir.abs(),mDefEps) )
	return false;

    res.dir_ = dir;

    double deter;
    if ( !mIsZero(dir.x,mDefEps) )
    {
	deter = dir.x;
	res.p0_.x = 0;
	res.p0_.y = (-D_*b.C_+C_*b.D_)/deter;
	res.p0_.z = (-B_*b.D_+D_*b.B_)/deter;
	return true;
    }
    else if ( !mIsZero(dir.y,mDefEps) )
    {
	deter = -dir.y;
	res.p0_.y = 0;
	res.p0_.x = (-D_*b.C_+C_*b.D_)/deter;
	res.p0_.z = (-A_*b.D_+D_*b.A_)/deter;;
	return true;
    }
    else
    {
	deter = dir.z;
	res.p0_.x = (-D_*b.B_+B_*b.D_)/deter;
	res.p0_.y = (-A_*b.D_+D_*b.A_)/deter;
	res.p0_.z = 0;
    }

    return true;
}


Plane3CoordSystem::Plane3CoordSystem(const Coord3& normal, const Coord3& origin,
				     const Coord3& pt10)
    : origin_( origin )
    , plane_( normal, origin, false )
{
    vec10_ = pt10-origin;
    const double vec10len = vec10_.abs();
    isok_ = !mIsZero(vec10len, 1e-5 );
    if ( isok_ )
	vec10_ /= vec10len;

    vec01_ = normal.cross( vec10_ ).normalize();
}


bool Plane3CoordSystem::isOK() const { return isok_; }


Coord Plane3CoordSystem::transform( const Coord3& pt, bool project ) const
{
    if ( !isok_ )
	return Coord3::udf();

    Coord3 v0;
    if ( project )
    {
	const Line3 line = Line3::fromPosAndDir( pt, plane_.normal() );
	plane_.intersectWith( line, v0 );
	v0 -= origin_;
    }
    else
	v0 = pt-origin_;

    const double len = v0.abs();
    if ( !len )
	return Coord( 0, 0 );
    const Coord3 dir = v0 / len;

    const double cosphi = vec10_.dot( dir );
    const double sinphi = vec01_.dot( dir );

    return Coord( cosphi * len, sinphi * len );
}


Coord3 Plane3CoordSystem::transform( const Coord& coord ) const
{
    return origin_ + vec10_*coord.x + vec01_*coord.y;
}


Sphere cartesian2Spherical( const Coord3& crd, bool math )
{
    double theta, phi;
    double rad = crd.abs();
    if ( math )
    {
	theta = rad ? Math::ACos( (crd.z / rad) ) : 0;
	phi = Math::Atan2( crd.y, crd.x );
    }
    else
    {
	theta = rad ? Math::ASin( (crd.z / rad) ) : 0;
	phi = Math::Atan2( crd.x, crd.y );
    }

    return Sphere( (float)rad, (float)theta, (float)phi );
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
