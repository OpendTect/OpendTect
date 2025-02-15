/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "trigonometry.h"

#include "math2.h"
#include "pca.h"
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

    vec_.x_ = a.x_ * sineval;
    vec_.y_ = a.y_ * sineval;
    vec_.z_ = a.z_ * sineval;
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
    return Quaternion( s_+b.s_, vec.x_, vec.y_, vec.z_ );
}


Quaternion& Quaternion::operator+=( const Quaternion& b )
{
    (*this) = (*this) + b;
    return *this;
}



Quaternion Quaternion::operator-( const Quaternion& b ) const
{
    const Vector3 vec =  vec_-b.vec_;
    return Quaternion( s_-b.s_, vec.x_, vec.y_, vec.z_ );
}


Quaternion& Quaternion::operator-=( const Quaternion& b )
{
    (*this) = (*this) - b;
    return *this;
}


Quaternion Quaternion::operator*( const Quaternion& b ) const
{
    const Vector3 vec = s_*b.vec_ + b.s_*vec_ + vec_.cross(b.vec_);
    return Quaternion( s_*b.s_-vec_.dot(b.vec_), vec.x_, vec.y_, vec.z_ );
}


Quaternion& Quaternion::operator*=( const Quaternion& b )
{
    (*this) = (*this) * b;
    return *this;
}


Quaternion Quaternion::inverse() const
{
    return Quaternion( s_, -vec_.x_, -vec_.y_, -vec_.z_ );
}



ParamLine2::ParamLine2( double slope, double intcpt )
    : x0_( 0 )
    , y0_( intcpt )
    , alpha_( 1 )
    , beta_( slope )
{}


ParamLine2::ParamLine2( const Coord& pt, double slope )
    : x0_( pt.x_ )
    , y0_( pt.y_ )
{
    if ( mIsUdf(slope) ) //Vertical
    {
	alpha_ = 0;
	beta_ = 1;
    }
    else
    {
	alpha_ = 1;
	beta_ = slope;
    }
}


ParamLine2::ParamLine2( const Coord& start, const Coord& stop )
    : x0_(start.x_)
    , y0_(start.y_)
    , alpha_ ( stop.x_ - start.x_ )
    , beta_( stop.y_ - start.y_ )
{}


double ParamLine2::sqDistanceToPoint( const Coord& p ) const
{
    const double t = closestPoint( p );
    const Coord closestpoint = getPoint( t );
    return closestpoint.sqDistTo( p );
}


double ParamLine2::distanceToPoint( const Coord& point ) const
{
    return Math::Sqrt( sqDistanceToPoint( point ) );
}


bool ParamLine2::operator==( const ParamLine2& line ) const
{
    //Check if direction is same
    const double dotprod =
	Coord(alpha_,beta_).dot(Coord(line.alpha_,line.beta_));

    if ( !mIsEqual(dotprod,1,mDefEps) )
	return false;

    const double sqdist = sqDistanceToPoint(Coord(line.x0_,line.y0_) );

    return mIsZero(sqdist,mDefEps);
}


Coord ParamLine2::getPoint( double t ) const
{
    return Coord( x0_+alpha_*t, y0_+beta_*t );
}




Coord ParamLine2::direction( bool normalize ) const
{
    const Coord res = Coord(alpha_,beta_);
    return normalize ? res.normalize() : res;
}


double ParamLine2::closestPoint( const Coord& point ) const
{
    const Coord dir = direction( false );
    const Coord diff = point-Coord(x0_,y0_);
    return diff.dot(dir)/dir.sqAbs();
}


Line2::Line2( double slope, double intcpt )
    : slope_(slope),yintcpt_(intcpt)
    , start_(mUdf(double),mUdf(double))
    , stop_(mUdf(double),mUdf(double))
    , isvertical_(false),xintcpt_(mUdf(double))
{}


Line2::Line2( const Coord& pt, double slope )
    : slope_(slope)
    , start_(mUdf(double),mUdf(double))
    , stop_(mUdf(double),mUdf(double))
{
    isvertical_ = mIsUdf(slope_) ? true : false;
    xintcpt_ = isvertical_ ? pt.x_ :
                             (mIsZero(slope,mDefEps) ? mUdf(double) : pt.x_-pt.y_/slope);
    yintcpt_ = isvertical_ ? mUdf(double) : pt.y_ - slope_ * pt.x_;
}


Line2::Line2( const Coord& start, const Coord& stop )
    : start_(start),stop_(stop)
    , isvertical_(false),xintcpt_(mUdf(double))
{
    double xdiff = stop_.x_ - start_.x_;
    double ydiff = stop_.y_ - start_.y_;
    if ( mIsZero(xdiff,mDefEps) )
    {
	slope_ = mUdf(double);
	yintcpt_= mUdf(double);
	isvertical_ = true;
        xintcpt_ = start_.x_;
    }
    else
    {
	slope_ = ydiff / xdiff;
        yintcpt_ = start_.y_ - slope_ * start_.x_;
    }
}


bool Line2::operator==( const Line2& line ) const
{
    if ( isvertical_ )
	return line.isvertical_ && mIsEqual(xintcpt_,line.xintcpt_,mDefEps);

    return mIsEqual(slope_,line.slope_,mDefEps) &&
	   mIsEqual(yintcpt_,line.yintcpt_,mDefEps);
}


#define mRetUdf return Coord( mUdf(double), mUdf(double) )
Coord Line2::direction() const
{
    if ( mIsUdf(slope_) )
    {
	if ( !isvertical_ || mIsUdf(xintcpt_) )
	    mRetUdf;

	return Coord( 0, 1 );
    }

    return Coord(1,slope_).normalize();
}


Coord Line2::closestPoint( const Coord& point ) const
{
    if ( mIsUdf(slope_) )
    {
	if ( !isvertical_ || mIsUdf(xintcpt_) )
	    mRetUdf;

        return Coord( xintcpt_, point.y_ );
    }

    const double x = ( point.x_ + slope_ * (point.y_-yintcpt_) ) /
		     ( 1 + slope_*slope_ );
    const double y = slope_ * x + yintcpt_;
    return Coord( x, y );
}


Coord Line2::intersection( const Line2& line, bool checkinlimits) const
{
    Coord pos( mUdf(double), mUdf(double) );
    if ( line.start_==line.stop_ && !isOnLine(line.start_) )
	mRetUdf;

    if ( mIsUdf(slope_) )
    {
	if ( !isvertical_ || mIsUdf(xintcpt_) )
	    mRetUdf;

	if ( mIsUdf(line.slope_) || line.isvertical_ )
	    mRetUdf;

        pos.x_ = xintcpt_;
        pos.y_ = line.slope_ * pos.x_ + line.yintcpt_;
    }
    else
    {
	if ( mIsUdf(line.slope_) )
	{
	    if ( !line.isvertical_ || mIsUdf(line.xintcpt_) )
		mRetUdf;

            pos.x_ = line.xintcpt_;
            pos.y_ = slope_ * pos.x_ + yintcpt_;
	}
	else
	{
	    double slopediff = slope_ - line.slope_;
	    if ( mIsZero(slopediff,mDefEps) )
		mRetUdf;

            pos.x_ = ( line.yintcpt_ - yintcpt_ ) / slopediff;
            pos.y_ = slope_ * pos.x_ + yintcpt_;
	}
    }

    if ( !checkinlimits )
	return pos;

    bool inlimits1 = true;
    if ( !mIsUdf(start_.x_) && !mIsUdf(stop_.x_) )
    {
        const double xdiff = stop_.x_ - start_.x_;
        const double ydiff = stop_.y_ - start_.y_;
        if ( !mIsZero(xdiff,mDefEps) && (pos.x_-start_.x_) * (stop_.x_-pos.x_) < 0 )
	    inlimits1 = false;

        if ( !mIsZero(ydiff,mDefEps) && (pos.y_-start_.y_) * (stop_.y_-pos.y_) < 0 )
	    inlimits1 = false;
    }

    bool inlimits2 = true;
    if ( !mIsUdf(line.start_.x_) && !mIsUdf(line.stop_.x_) )
    {
        const double xdiff = line.stop_.x_ - line.start_.x_;
        const double ydiff = line.stop_.y_ - line.start_.y_;
	if ( !mIsZero(xdiff,mDefEps)
             && (pos.x_-line.start_.x_) * (line.stop_.x_-pos.x_) < 0 )
	    inlimits1 = false;

	if ( !mIsZero(ydiff,mDefEps)
             && (pos.y_-line.start_.y_) * (line.stop_.y_-pos.y_) < 0 )
	    inlimits1 = false;
    }

    if ( !inlimits1 || !inlimits2 )
	mRetUdf;

    return pos;
}


bool Line2::isOnLine( const Coord& pt ) const
{
    return isvertical_ ? mIsEqual(pt.x_,xintcpt_,0.0001) :
                         mIsEqual(pt.y_,slope_*pt.x_+yintcpt_,0.0001);
}


double Line2::distanceTo( const Line2& line ) const
{
    if ( isvertical_ && line.isvertical_ )
	return fabs( xintcpt_ - line.xintcpt_ );

    if ( !mIsEqual(slope_,line.slope_,mDefEps) )
	return mUdf(double);

    const double intcptdiff = fabs( yintcpt_ - line.yintcpt_ );
    return intcptdiff / Math::Sqrt( 1 + slope_ * slope_ );
}


bool Line2::getParallelLine( Line2& line, double dist ) const
{
    if ( mIsUdf(slope_) )
    {
	if ( !isvertical_ || mIsUdf(xintcpt_) )
	    return false;

	line.yintcpt_ = mUdf(double);
	line.isvertical_ = true;
	line.xintcpt_ = xintcpt_ + dist;
    }
    else
    {
	double constterm = dist * Math::Sqrt( 1 + slope_ * slope_ );
	line.yintcpt_ = yintcpt_ + constterm;
    }

    line.slope_ = slope_;
    return true;
}


bool Line2::getPerpendicularLine( Line2& line, const Coord& point ) const
{
    if ( mIsUdf(slope_) )
    {
	if ( !isvertical_ || mIsUdf(xintcpt_) )
	    return false;

	line.slope_ = 0;
	line.isvertical_ = false;
        line.yintcpt_ = point.y_;
    }
    else if ( mIsZero(slope_,mDefEps) )
    {
	line.slope_ = mUdf(double);
	line.yintcpt_ = mUdf(double);
	line.isvertical_ = true;
        line.xintcpt_ = point.x_;
    }
    else
    {
	line.slope_ = -1. / slope_;
        line.yintcpt_ = point.y_ - line.slope_ * point.x_;
    }

    return true;
}



Line3::Line3() {}

Line3::Line3( double x0, double y0, double z0, double alpha, double beta,
	      double gamma )
    : x0_( x0 )
    , y0_( y0 )
    , z0_( z0 )
    , alpha_( alpha )
    , beta_( beta )
    , gamma_( gamma )
{}


Line3::Line3( const Coord3& point, const Vector3& vector )
    : x0_( point.x_ )
    , y0_( point.y_ )
    , z0_( point.z_ )
    , alpha_( vector.x_ )
    , beta_(vector.y_ )
    , gamma_( vector.z_ )
{}


double Line3::distanceToPoint( const Coord3& point ) const
{
    return Math::Sqrt( sqDistanceToPoint( point ) );
}


double Line3::sqDistanceToPoint( const Coord3& point ) const
{
    const Vector3 p0p1( point.x_ - x0_, point.y_ - y0_, point.z_ - z0_ );
    const Vector3 v( alpha_, beta_, gamma_ );

    return v.cross( p0p1 ).sqAbs() / v.sqAbs();
}


/*
   |
   B-------C
   |      /
   |     /
   |    /
   |   /
   |  /
   |a/
   |/
   A
   |
   |

Given: A, C and dir
Wanted: B

B = dir/|dir| * |AB|
|AB| = |AC| * cos(a)
dir.AC = |dir|*|AC|*cos(a)

dir.AC / |dir| = |AC|*cos(a)

B = dir/|dir| * dir.AC / |dir|

*/

double Line3::closestPoint( const Coord3& point ) const
{
    const Coord3 dir = direction( false );
    const Coord3 diff = point-Coord3(x0_,y0_,z0_);
    return diff.dot(dir)/dir.sqAbs();
}


void Line3::closestPoint( const Line3& line, double& t_this,
			  double& t_line ) const
{
    const Coord3 dir0 = direction( false );
    const Coord3 dir1 = line.direction( false );
    const Coord3 diff(x0_-line.x0_,y0_-line.y0_,z0_-line.z0_);
    const double d0 = dir0.dot(dir0);
    const double d1 = dir1.dot(dir1);
    if ( mIsZero(d0,1e-8) || mIsZero(d1,1e-8) )
    {
	t_this = t_line = mUdf(double);
	return;
    }

    const double d01 = dir0.dot(dir1);
    const double di0 = diff.dot(dir0);
    const double di1 = diff.dot(dir1);
    const double det = d01*d01-d0*d1;

    t_this = mIsZero(det,1e-8) ? 0.0 : (di0*d1-di1*d01)/det;
    t_line = (di1+t_this*d01)/d1;
}


bool Line3::intersectWith( const Plane3& b, double& t ) const
{
    const double denominator = alpha_*b.A_ + beta_*b.B_ + gamma_*b.C_;
    const double dist0 = b.A_*x0_ + b.B_*y0_ + b.C_*z0_ + b.D_;
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


Coord3 Line3::getPoint( double t ) const
{
    return Coord3( x0_+alpha_*t, y0_+beta_*t, z0_+gamma_*t );
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
        A_ = cross.x_;
        B_ = cross.y_;
        C_ = cross.z_;
	D_ = 0;
    }
    else
    {
        A_ = norm.x_;
        B_ = norm.y_;
        C_ = norm.z_;
        D_ =  -(norm.x_*point.x_) - (norm.y_*point.y_) - ( norm.z_*point.z_ );
    }
}


void Plane3::set( const Coord3& a, const Coord3& b, const Coord3& c )
{
    Vector3 ab( b.x_ -a.x_, b.y_ -a.y_, b.z_ -a.z_ );
    Vector3 ac( c.x_ -a.x_, c.y_ -a.y_, c.z_ -a.z_ );

    Vector3 n = ab.cross( ac );

    A_ = n.x_;
    B_ = n.y_;
    C_ = n.z_;
    D_ = A_*(-a.x_)  - B_*a.y_ - C_*a.z_;
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

    midpt.x_ /= nrpts;
    midpt.y_ /= nrpts;
    midpt.z_ /= nrpts;

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
    const double param = (A_*pos.x_+B_*pos.y_+C_*pos.z_+D_)/(A_*A_+B_*B_+C_*C_);
    return Coord3( pos.x_-A_*param, pos.y_-B_*param, pos.z_-C_*param );
}


bool Plane3::onSameSide( const Coord3& p1, const Coord3& p2 )
{ return (A_*p1.x_+B_*p1.y_+C_*p1.z_+D_) * (A_*p2.x_+B_*p2.y_+C_*p2.z_+D_) >= 0; }


double Plane3::distanceToPoint( const Coord3& point, bool whichside ) const
{
    Vector3 norm( normal().normalize() );
    const Line3 linetoplane( point, norm );

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

    res.alpha_ = dir.x_;
    res.beta_ = dir.y_;
    res.gamma_ = dir.z_;

    double deter;
    if ( !mIsZero(dir.x_,mDefEps) )
    {
        deter = dir.x_;
	res.x0_ = 0;
	res.y0_ = (-D_*b.C_+C_*b.D_)/deter;
	res.z0_ = (-B_*b.D_+D_*b.B_)/deter;
	return true;
    }
    else if ( !mIsZero(dir.y_,mDefEps) )
    {
        deter = -dir.y_;
	res.y0_ = 0;
	res.x0_ = (-D_*b.C_+C_*b.D_)/deter;
	res.z0_ = (-A_*b.D_+D_*b.A_)/deter;;
	return true;
    }
    else
    {
        deter = dir.z_;
	res.x0_ = (-D_*b.B_+B_*b.D_)/deter;
	res.y0_ = (-A_*b.D_+D_*b.A_)/deter;
	res.z0_ = 0;
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
	const Line3 line( pt, plane_.normal() );
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
    return origin_ + vec10_*coord.x_ + vec01_*coord.y_;
}


Sphere cartesian2Spherical( const Coord3& crd, bool math )
{
    double theta, phi;
    double rad = crd.abs();
    if ( math )
    {
        theta = rad ? Math::ACos( (crd.z_ / rad) ) : 0;
        phi = Math::Atan2( crd.y_, crd.x_ );
    }
    else
    {
        theta = rad ? Math::ASin( (crd.z_ / rad) ) : 0;
        phi = Math::Atan2( crd.x_, crd.y_ );
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



// NearestCoordFinder
NearestCoordFinder::NearestCoordFinder( const TypeSet<Coord3>& crds,
					const Coord3& crd )
    : ParallelTask("Finding nearest MD")
    , crds_(crds)
    , crd_(crd)
{}


NearestCoordFinder::~NearestCoordFinder()
{}


od_int64 NearestCoordFinder::nrIterations() const
{
    return crds_.size();
}


bool NearestCoordFinder::doPrepare( int nrthreads )
{
    minidxs_.setSize( nrthreads, -1 );
    mindists_.setSize( nrthreads, mUdf(double) );
    return true;
}


bool NearestCoordFinder::doWork( od_int64 start, od_int64 stop, int threadidx )
{
    double& mindist = mindists_[threadidx];
    int& minidx = minidxs_[threadidx];
    for ( int idx=start; idx<=stop; idx++ )
    {
	const double dist = crd_.sqDistTo( crds_[idx] );
	if ( dist < mindist )
	{
	    mindist = dist;
	    minidx = idx;
	}
    }

    return true;
}


bool NearestCoordFinder::doFinish( bool success )
{
    int thridx = -1;
    double dist = mUdf(double);
    for ( int idx=0; idx<mindists_.size(); idx++ )
    {
	if ( mindists_[idx] < dist )
	{
	    dist = mindists_[idx];
	    thridx = idx;
	}
    }

    nearestidx_ = thridx != -1 ? minidxs_[thridx] : -1;
    nearestdist_ = thridx != -1 ? Math::Sqrt(mindists_[thridx]) : mUdf(float);
    return success;
}
