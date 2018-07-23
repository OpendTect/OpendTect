#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		23-11-2002
________________________________________________________________________


-*/

#include "algomod.h"
#include "coord.h"
#include "typeset.h"
#include <math.h>
class Plane3;


/*!\brief
Given a point pt in a triangle ABC, we calculate the interpolation weights for
each vertex.
 */
inline void interpolateOnTriangle2D( const Coord pt,
			const Coord a, const Coord b, const Coord c,
			double& weight_a, double& weight_b, double& weight_c )
{
    const Coord3 ba = Coord3(b-a,0.);
    const Coord3 ca = Coord3(c-a,0.);
    const double triarea = ba.cross( ca ).abs<double>()*0.5;

    const Coord3 pa = Coord3(pt-a,0.);
    const Coord3 pb = Coord3(pt-b,0.);
    const Coord3 pc = Coord3(pt-c,0.);

    if ( mIsZero(triarea,1e-8) )
    {
	const double distpa = pa.abs<double>();
	if ( mIsZero(distpa,1e-8) )
	{
	    weight_a = 1.; weight_b = 0.; weight_c = 0.;
	    return;
	}

	const double distpb = pb.abs<double>();
	if ( mIsZero(distpb,1e-8) )
	{
	    weight_a = 0.; weight_b = 1.; weight_c = 0.;
	    return;
	}

	const double distpc = pc.abs<double>();
	if ( mIsZero(distpc,1e-8) )
	{
	    weight_a = 0.; weight_b = 0.; weight_c = 1.;
	    return;
	}

	const double totalinversedist = 1./distpa + 1./distpb + 1./distpc;
	weight_a = 1./(distpa*totalinversedist);
	weight_b = 1./(distpb*totalinversedist);
	weight_c = 1./(distpc*totalinversedist);
    }
    else
    {
	const double triareapab = pa.cross( pb ).abs<double>()*0.5;
	const double triareapac = pa.cross( pc ).abs<double>()*0.5;
	const double triareapcb = pc.cross( pb ).abs<double>()*0.5;

	weight_a = triareapcb / triarea;
	weight_b = triareapac / triarea;
	weight_c = triareapab / triarea;
    }
}


/*!\brief
Here are some commonly used functions to judge the position relation between
point and line, point and triangle, point and circle or sphere.
*/

/*Calculate a 3x3 matrix's determinent given by v[0]-v[8] with 9 elements. */
inline double determinent33( const double* v )
{
    return v[0]*(v[4]*v[8]-v[5]*v[7])+v[1]*(v[5]*v[6]-v[3]*v[8])+
	v[2]*(v[3]*v[7]-v[4]*v[6]);
}


/*Calculate a 4x4 matrix's determinent given by rows r0, r1, r2, r3 with the
  last column 1, 1, 1, 1. */
inline double determinent44( const Coord3& r0, const Coord3& r1,
		             const Coord3& r2, const Coord3& r3 )
{
    const double d0[9] = { r1.y_, r1.z_, 1, r2.y_, r2.z_, 1, r3.y_, r3.z_, 1 };
    const double d1[9] = { r1.x_, r1.z_, 1, r2.x_, r2.z_, 1, r3.x_, r3.z_, 1 };
    const double d2[9] = { r1.x_, r1.y_, 1, r2.x_, r2.y_, 1, r3.x_, r3.y_, 1 };
    const double d3[9] = { r1.x_, r1.y_, r1.z_, r2.x_, r2.y_, r2.z_, r3.x_,
			   r3.y_, r3.z_};
    return r0.x_*determinent33( d0 )-r0.y_*determinent33( d1 )+
	   r0.z_*determinent33( d2 )-determinent33( d3 );
}

/*!<Each ri represents a row of 4 elements. */
inline double determinent44( const double* r0, const double* r1,
			     const double* r2, const double* r3 )
{
    const double d0[9] = { r1[1], r1[2], r1[3], r2[1], r2[2], r2[3],
			   r3[1], r3[2], r3[3] };
    const double d1[9] = { r1[0], r1[2], r1[3], r2[0], r2[2], r2[3],
			   r3[0], r3[2], r3[3] };
    const double d2[9] = { r1[0], r1[1], r1[3], r2[0], r2[1], r2[3],
			   r3[0], r3[1], r3[3] };
    const double d3[9] = { r1[0], r1[1], r1[2], r2[0], r2[1], r2[2],
			   r3[0], r3[1], r3[2] };
    return r0[0]*determinent33( d0 )-r0[1]*determinent33( d1 )+
	   r0[2]*determinent33( d2 )-r0[3]*determinent33( d3 );
}

/*!<Check the point pt is inside the circumcircle of p1, p2, p3 or not. */
inline bool isInsideCircle( const Coord& pt,
			    const Coord& p1, const Coord& p2, const Coord& p3 )
{
    Coord center;
    const Coord d12 = p1-p2;
    const Coord d13 = p1-p3;
    const Coord d = p1-pt;
    const double deter=d13.x_*d12.y_-d13.y_*d12.x_;
    const double a12 = ( p1.dot(p1)-p2.dot(p2) )/2;
    const double a13 = ( p1.dot(p1)-p3.dot(p3) )/2;
    center.x_ = (a13*d12.y_-a12*d13.y_)/deter;
    center.y_ = (d13.x_*a12-d12.x_*a13)/deter;
    return pt.sqAbs()-p1.sqAbs()+2*center.dot(d)<0;
}


/*!<Check the point p is inside the circumsphere of a, b, c, d or not. */
inline bool isInsideCircumSphere( const Coord3& p, const Coord3& a,
	const Coord3& b, const Coord3& c, const Coord3& d )
{
    const Coord3 ab = a-b;
    const Coord3 ac = a-c;
    const Coord3 ad = a-d;
    const double t[9] = { ab.x_, ab.y_, ab.z_, ac.x_, ac.y_, ac.z_, ad.x_,
			  ad.y_, ad.z_};
    const double deter = determinent33( t );

    const double sqra = a.x_*a.x_+a.y_*a.y_+a.z_*a.z_;
    const double d0 = (sqra-(b.x_*b.x_+b.y_*b.y_+b.z_*b.z_))/2;
    const double d1 = (sqra-(c.x_*c.x_+c.y_*c.y_+c.z_*c.z_))/2;
    const double d2 = (sqra-(d.x_*d.x_+d.y_*d.y_+d.z_*d.z_))/2;
    const double t0[9] = { d0, ab.y_, ab.z_, d1, ac.y_, ac.z_, d2, ad.y_,ad.z_};
    const double t1[9] = { ab.x_, d0, ab.z_, ac.x_, d1, ac.z_, ad.x_, d2,ad.z_};
    const double t2[9] = { ab.x_, ab.y_, d0, ac.x_, ac.y_, d1, ad.x_, ad.y_,d2};
    const double centerx = determinent33(t0)/deter;
    const double centery = determinent33(t1)/deter;
    const double centerz = determinent33(t2)/deter;

    return (p.x_*p.x_+p.y_*p.y_+p.z_*p.z_-sqra+
	2*(centerx*(a.x_-p.x_)+centery*(a.y_-p.y_)+centerz*(a.z_-p.z_)))<0;
}


/*! Check p1, p2 are on the same side of the edge AB or not.*/
inline bool sameSide2D( const Coord& p1, const Coord& p2,
			const Coord& a, const Coord& b, double epsilon )
{
    double xdiff = b.x_-a.x_;
    double ydiff = b.y_-a.y_;
    return ((p1.x_-a.x_)*ydiff-(p1.y_-a.y_)*xdiff)*
	((p2.x_-a.x_)*ydiff-(p2.y_-a.y_)*xdiff)>=-epsilon;
}


/*!<Only when four points are coplanar. */
inline bool sameSide3D( const Coord3& p1, const Coord3& p2,
			const Coord3& a, const Coord3& b, double epsilon )
{
    const Coord3 cpp1 = (b-a).cross(p1-a);
    const Coord3 cpp2 = (b-a).cross(p2-a);
    return cpp1.dot(cpp2)>=-epsilon;
}


/*!<Use this function only when the 4 points are all in a plane. */
inline bool pointInTriangle2D( const Coord& p, const Coord& a, const Coord& b,
			       const Coord& c, double epsilon )
{
    if ( (p.x_>a.x_ && p.x_>b.x_ && p.x_>c.x_) ||
	 (p.x_<a.x_ && p.x_<b.x_ && p.x_<c.x_) ||
	 (p.y_>a.y_ && p.y_>b.y_ && p.y_>c.y_) ||
	 (p.y_<a.y_ && p.y_<b.y_ && p.y_<c.y_) )
	return false;

    return sameSide2D(p,a,b,c,epsilon) && sameSide2D(p,b,a,c,epsilon) &&
	   sameSide2D(p,c,a,b,epsilon);
}


/*!<Only when four points are coplanar. */
inline bool pointInTriangle3D( const Coord3& p, const Coord3& a,
			const Coord3& b, const Coord3& c, double epsilon,
			bool useangularmethod=false )
{
    if ( !useangularmethod )
    {
	return sameSide3D(p,a,b,c,epsilon) && sameSide3D(p,b,a,c,epsilon) &&
	       sameSide3D(p,c,a,b,epsilon);
    }

    Coord3 ap = a - p;
    ap = ap.normalize();
    Coord3 bp = b - p;
    bp = bp.normalize();
    Coord3 cp = c - p;
    cp = cp.normalize();

    const double d1 = ap.dot( bp );
    const double d2 = bp.dot( cp );
    const double d3 = cp.dot( ap );
    const double angle = Math::ACos(d1) + Math::ACos(d2) + Math::ACos(d3);

    return mIsEqual(angle,M_2PI,epsilon);
}


/*!< Check to see if the point P is on the edge AB or not.*/
inline bool pointOnEdge2D( const Coord& p, const Coord& a, const Coord& b,
			   double epsilon )
{
    const Coord pa = p-a;
    const Coord ba = b-a;
    const double t = pa.dot(ba)/ba.sqAbs();
    if ( t<0 || t>1 )
	return false;

    const Coord intersectpt = a+Coord(t*ba.x_, t*ba.y_);
    const Coord pq = p-intersectpt;
    return pq.sqAbs()<epsilon*epsilon;
}


inline bool pointOnEdge3D( const Coord3& p, const Coord3& a, const Coord3& b,
			   double epsilon )
{
    if ( (p.x_<a.x_ && p.x_<b.x_) || (p.x_>a.x_ && p.x_>b.x_) ||
	 (p.y_<a.y_ && p.y_<b.y_) || (p.y_>a.y_ && p.y_>b.y_) ||
	 (p.z_<a.z_ && p.z_<b.z_) || (p.z_>a.z_ && p.z_>b.z_) )
	return false;

    const Coord3 pa = p-a;
    const Coord3 ba = b-a;
    const double t = pa.dot(ba)/ba.sqAbs();
    if ( t<0 || t>1 )
	return false;

    const Coord3 pq = pa-t*ba;
    return pq.sqAbs()<epsilon*epsilon;
}


/*!<For point and polygon lie on the same plane only. In 2D case, set all zs'
    to be 0, we also consider the case that the point is exactly on a vertex as
    inside. Use the check that all the angle sum should be 2 Pi. */
inline bool pointInPolygon( const Coord3& pt, const TypeSet<Coord3>& plgknots,
			    double epsilon )
{
    const int nrvertices = plgknots.size();
    if ( nrvertices==2 )
    {
	const double newepsilon = plgknots[0].distTo<double>(plgknots[1])*0.001;
	return pointOnEdge3D( pt, plgknots[0], plgknots[1], newepsilon );
    }
    else if ( nrvertices==3 )
	return pointInTriangle3D( pt, plgknots[0], plgknots[1], plgknots[2],
				  epsilon );
    else
    {
	Coord3 p1, p2;
	double anglesum = 0, cosangle;
	for ( int idx=0; idx<nrvertices; idx++ )
	{
	    p1 = plgknots[idx] - pt;
	    p2 = plgknots[(idx+1)%nrvertices] - pt;

	    const double d1 = p1.abs<double>();
	    const double d2 = p2.abs<double>();
	    if ( d1*d2 <= epsilon*epsilon || d1 <= epsilon || d2 <= epsilon )
		return true;
	    else
		cosangle = p1.dot(p2) / (d1*d2);

	    anglesum += acos( cosangle );
	}

	return mIsEqual( anglesum, 6.2831853071795862, 1e-4 );
    }
}


typedef Coord3 Vector3;

/*! \brief
Divides a sphere in a number of vectors, divided by approximately dangle from
each other. dradius is given in radians
*/

TypeSet<Vector3>* makeSphereVectorSet( double dangle );

/*!\brief
Computes an average of a number of vectors using:
1. Summation
2. PCA estimation
*/

Coord3 estimateAverageVector( const TypeSet<Coord3>&, bool normalize,
			      bool checkforundefs );


/*!
\brief Quaternion is an extension to complex numbers.

  A Quaternion is represented by the equation:<br>
  q = s + xi + yj + zk <br>
  where: i*i = j*j = k*k = -1.
*/

mExpClass(Algo) Quaternion
{
public:
			Quaternion(double s,double x,double y,double z);
			Quaternion(const Vector3& axis,float angle);

    void		setRotation(const Vector3& axis,float angle);
    void		getRotation(Vector3& axis,float& angle) const;
			/*!<\note axis is not normalized. */
    Coord3		rotate(const Coord3&) const;

    Quaternion		operator+(const Quaternion&) const;
    Quaternion& operator+=(const Quaternion&);
    Quaternion		operator-(const Quaternion&) const;
    Quaternion& operator-=(const Quaternion&);
    Quaternion		operator*(const Quaternion&) const;
    Quaternion& operator*=(const Quaternion&);

    Quaternion		inverse() const;

    double		s_;
    Vector3		vec_;
};


/*!Base class for parametric lines, i.e. lines on the form:
    p = p0 + t*dir

 This class is instantiated with T=Coord and T=Coord3.
 */

template <class T>
class ParamLineBase
{
public:

    T		direction(bool normalized=true) const;

    T		getPoint(double t) const;
    T		start() const { return getPoint(0); }
    T		stop() const { return getPoint(1); }
    double	closestParam(const T& point) const;
		/*!<\returns the t-value of the point on the line that is
		 closest to the given point. */

    T		closestPoint(const T&) const;
		/*!<\returns the coordinates of the point on the line that
		  is closest to the given point. */
    double	distanceToPoint(const T&) const;
    double	sqDistanceToPoint(const T&) const;
		/*!<Returns the squared distance, which is cheaper to compute.*/
    bool	isOnLine(const T& pt) const;


    T		p0_;
    T		dir_;
};



/*!
\brief A Line2 is a line in the plane, with the following equations:

  p(x,y) = p0 + dir * t

 Though the line is infinitely long, one can treat the line as having
 a start when t=0 and a stop when t=1.
*/

mExpClass(Algo) Line2 : public ParamLineBase<Coord>
{
public:
			Line2();
			Line2(const Coord& start,const Coord& stop);
			/*!<Normalizes so that t=0 is p0 and t=1 is p1 */

    static Line2	fromPosAndDir(const Coord&, const Coord&);

    bool		operator==(const Line2&) const;
			mImplSimpleIneqOper(Line2)

    Coord		intersection(const Line2&,bool checkinlimit=true) const;
			/*!<If checkinlimit is true, Coord::udf() will be
			    returned if the intersection is ouside t=[0,1] on
			    either line. */
    void		getPerpendicularLine(Line2&,const Coord& pt) const;
    void		getParallelLine(Line2& line,double dist) const;
};


/*!
\brief A Line3 is a line in space, with the following equations:

   p(x,y,z) = p0 + dir * t
*/

mExpClass(Algo) Line3 : public ParamLineBase<Coord3>
{
public:
			Line3();
			Line3(	double x0, double y0, double z0,
				double alpha, double beta, double gamma );
			Line3( const Coord3&, const Coord3& );
			/*<Create line using point and direction. */
    static Line3	fromPosAndDir(const Coord3&, const Vector3&);

    bool		intersectWith( const Plane3&, double& t ) const;
			/*!<Calculates the intersection between the line
			    and the plane. If success, it sets t. */

    void		closestPointToLine(const Line3& line, double& t_this,
				     double& t_line ) const;
			/*!<\returns the t for the point point on the line
				     that is closest to the given line*/
};


/*!
\brief A Plane3 is a plane in space, with the equation: Ax + By + Cz + D = 0
*/

mExpClass(Algo) Plane3
{
public:
			Plane3();
			Plane3(double, double, double, double);
			Plane3( const Coord3& vectors, const Coord3&,
				bool twovectors );
			/*!<\param twovectors	Specifies if the second argument
						is a vector or a position
			*/
			Plane3( const Coord3&, const Coord3&, const Coord3& );
			Plane3( const TypeSet<Coord3>& );

    void		set( const Coord3& vector, const Coord3&,
			     bool twovectors );
			/*!<\param twovectors	Specifies if the second argument
						is a vector or a position
			*/
    void		set( const Coord3&, const Coord3&, const Coord3& );
    float		set( const TypeSet<Coord3>& );
			/*!< \returns	a value between 0-1 that indicates how
					well the points fit to a plane.
					1 = perfect fit 0 = no fit
			*/

    bool		operator==(const Plane3&) const;
			mImplSimpleIneqOper(Plane3)

    Coord3		normal() const { return Coord3( A_, B_, C_ ); }

    double		distanceToPoint( const Coord3&,
					bool wichside=false ) const;
			/*!<\param wichside if true, the distance along the
				   normal will be returned, wich can be
				   negative.
		        */
    bool		intersectWith( const Line3&, Coord3& ) const;
			/*!< Returns true if the plane intersects with the
			     line. If it returns true, the Coord3 is set */
    bool		intersectWith( const Plane3&, Line3& ) const;
			/*!< Returns true if the planes intersects.
			     If it returns true, the Line3 is set */
    Coord3		getProjection(const Coord3& pos);
    bool		onSameSide(const Coord3& p1,const Coord3& p2);
			/*!<Check p1, p2 are on the same side of the plane or
			    not, will return true if one is on the plane.*/

    double		A_;
    double		B_;
    double		C_;
    double		D_;
};


/*!
\brief Defines a 2D coord system on a 3D plane and transforms between the
3D space and the coord system.
*/

mExpClass(Algo) Plane3CoordSystem
{
public:
			Plane3CoordSystem(const Coord3& normal,
					  const Coord3& origin,
				          const Coord3& pt10);
			/*!<\param normal The normal of the plane
			    \param origin A point on the plane
			    \param pt10   A point on the plane, not identical
					  to origin. */
    virtual		~Plane3CoordSystem() {}
    bool		isOK() const;
			/*!<\returns false if two identical points were given
			             in the constructor. */

    const Plane3&	plane() const { return plane_; }

    Coord		transform(const Coord3&,bool project) const;
			/*!<\param project should be true if the coord is
			           not located on the plane. If true, the
				   point will be projected onto the plane. */

    Coord3		transform(const Coord&) const;

protected:

    const Plane3	plane_;
    const Coord3	origin_;
    Coord3		vec10_;
    Coord3		vec01_;
    bool		isok_;
};


/*!
\brief Represents a point in spherical coordinates.
The angle phi lies in the horizontal plane, theta in the vertical plane.
*/

mExpClass(Algo) Sphere
{
public:
			Sphere(float r=0,float t=0,float p=0)
			    : radius_(r),theta_(t),phi_(p)		{}

    explicit		Sphere(const Coord3& crd)
			    : radius_((float) crd.x_),theta_((float) crd.y_)
			    , phi_((float) crd.z_)		{}
    inline bool		operator ==(const Sphere&) const;
			mImplSimpleIneqOper(Sphere)

    float		radius_;
    float		theta_;
    float		phi_;

    static const Sphere& nullSphere();
    bool		isNull() const; //!< compares with epsilon

};


mGlobal(Algo) Sphere cartesian2Spherical(const Coord3&,bool math);
	    /*!< math=true: transformation done in math-system
		 math=false: transformation done in geo-system */
mGlobal(Algo) Coord3 spherical2Cartesian(const Sphere&,bool math);
	    /*!< math=true: transformation done in math-system
		 math=false: transformation done in geo-system */

inline bool Sphere::operator ==( const Sphere& s ) const
{
    const float dr = radius_ - s.radius_;
    const float dt = theta_ - s.theta_;
    const float dp = phi_ - s.phi_;
    return mIsZero(dr,1e-8) && mIsZero(dt,1e-8) && mIsZero(dp,1e-8);
}

//Implementations of ParamLineBase



template <class T> inline
double ParamLineBase<T>::sqDistanceToPoint(const T& p) const
{
    const double t = closestParam(p);
    const T closestpoint = getPoint(t);
    return closestpoint.sqDistTo(p);
}


template <class T> inline
T ParamLineBase<T>::closestPoint(const T& pt) const
{
    return getPoint(closestParam(pt));
}


template <class T> inline
double ParamLineBase<T>::distanceToPoint(const T& point) const
{
    return Math::Sqrt(sqDistanceToPoint(point));
}

template <class T> inline
T ParamLineBase<T>::getPoint(double t) const
{
    return p0_ + dir_ * t;
}


template <class T> inline
T ParamLineBase<T>::direction(bool normalize) const
{
    return normalize ? dir_.normalize() : dir_;
}


template <class T> inline
double ParamLineBase<T>::closestParam(const T& point) const
{
    const T diff = point - p0_;
    return diff.dot(dir_) / dir_.sqAbs();
}


template <class T> inline
bool ParamLineBase<T>::isOnLine(const T& pt) const
{
    return sqDistanceToPoint(pt)<0.0001;
}
