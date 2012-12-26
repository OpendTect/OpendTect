#ifndef trigonometry_h
#define trigonometry_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		23-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "algomod.h"
#include "position.h"
#include <math.h>

template <class T> class TypeSet;
template <class T> class ObjectSet;
class Plane3;

/*!\brief
Given a point pt in a triangle ABC, we calculate the interpolation weights for
each vertex.
 */
inline void interpolateOnTriangle2D( const Coord pt, 
			const Coord a, const Coord b, const Coord c, 
			float& weight_a, float& weight_b, float& weight_c )
{
    const Coord d0 = b-a;
    const Coord d1 = pt-a;
    const Coord d2 = b-c;
    const float para_pt = (float)((d0.x*d2.y-d0.y*d2.x)/(d1.x*d2.y-d1.y*d2.x));
    const float para_bc = (float)((d1.x*d0.y-d1.y*d0.x)/(d1.x*d2.y-d1.y*d2.x));

    if ( mIsZero(para_pt, 1e-5) )
    {
	weight_a = 1;
	weight_b = 0;
	weight_c = 0;
	return;
    }

    weight_a = 1-1/para_pt;
    weight_b = (1-para_bc)/para_pt;
    weight_c = para_bc/para_pt;
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
    const double d0[9] = { r1.y, r1.z, 1, r2.y, r2.z, 1, r3.y, r3.z, 1 };
    const double d1[9] = { r1.x, r1.z, 1, r2.x, r2.z, 1, r3.x, r3.z, 1 };
    const double d2[9] = { r1.x, r1.y, 1, r2.x, r2.y, 1, r3.x, r3.y, 1 };
    const double d3[9] = { r1.x, r1.y, r1.z, r2.x, r2.y, r2.z, r3.x, r3.y,r3.z};
    return r0.x*determinent33( d0 )-r0.y*determinent33( d1 )+
	   r0.z*determinent33( d2 )-determinent33( d3 );
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
    const double deter=d13.x*d12.y-d13.y*d12.x;
    const double a12 = ( p1.dot(p1)-p2.dot(p2) )/2;
    const double a13 = ( p1.dot(p1)-p3.dot(p3) )/2;
    center.x = (a13*d12.y-a12*d13.y)/deter;
    center.y = (d13.x*a12-d12.x*a13)/deter;
    return pt.sqAbs()-p1.sqAbs()+2*center.dot(d)<0;
}


/*!<Check the point p is inside the circumsphere of a, b, c, d or not. */
inline bool isInsideCircumSphere( const Coord3& p, const Coord3& a,
	const Coord3& b, const Coord3& c, const Coord3& d )
{
    const Coord3 ab = a-b;
    const Coord3 ac = a-c;
    const Coord3 ad = a-d;
    const double t[9] = { ab.x, ab.y, ab.z, ac.x, ac.y, ac.z, ad.x, ad.y, ad.z};
    const double deter = determinent33( t );
    
    const double sqra = a.x*a.x+a.y*a.y+a.z*a.z;
    const double d0 = (sqra-(b.x*b.x+b.y*b.y+b.z*b.z))/2;
    const double d1 = (sqra-(c.x*c.x+c.y*c.y+c.z*c.z))/2;
    const double d2 = (sqra-(d.x*d.x+d.y*d.y+d.z*d.z))/2;
    const double t0[9] = { d0, ab.y, ab.z, d1, ac.y, ac.z, d2, ad.y, ad.z };
    const double t1[9] = { ab.x, d0, ab.z, ac.x, d1, ac.z, ad.x, d2, ad.z };
    const double t2[9] = { ab.x, ab.y, d0, ac.x, ac.y, d1, ad.x, ad.y, d2 };
    const double centerx = determinent33(t0)/deter;
    const double centery = determinent33(t1)/deter;
    const double centerz = determinent33(t2)/deter;
    
    return (p.x*p.x+p.y*p.y+p.z*p.z-sqra+
	2*(centerx*(a.x-p.x)+centery*(a.y-p.y)+centerz*(a.z-p.z)))<0;
}


/*! Check p1, p2 are on the same side of the edge AB or not.*/
inline bool sameSide2D( const Coord& p1, const Coord& p2, 
			const Coord& a, const Coord& b, double epsilon )
{
    double xdiff = b.x-a.x;
    double ydiff = b.y-a.y;
    return ((p1.x-a.x)*ydiff-(p1.y-a.y)*xdiff)*
	((p2.x-a.x)*ydiff-(p2.y-a.y)*xdiff)>=-epsilon;
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
    if ( (p.x>a.x && p.x>b.x && p.x>c.x) || (p.x<a.x && p.x<b.x && p.x<c.x) ||
     	 (p.y>a.y && p.y>b.y && p.y>c.y) || (p.y<a.y && p.y<b.y && p.y<c.y) )
	return false;

    return sameSide2D(p,a,b,c,epsilon) && sameSide2D(p,b,a,c,epsilon) && 
	   sameSide2D(p,c,a,b,epsilon);
}


/*!<Only when four points are coplanar. */
inline bool pointInTriangle3D( const Coord3& p, const Coord3& a, 
			const Coord3& b, const Coord3& c, double epsilon )
{
    return sameSide3D(p,a,b,c,epsilon) && sameSide3D(p,b,a,c,epsilon) && 
	   sameSide3D(p,c,a,b,epsilon);
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

    const Coord intersectpt = a+Coord(t*ba.x, t*ba.y);
    const Coord pq = p-intersectpt;
    return pq.sqAbs()<epsilon*epsilon;
}


inline bool pointOnEdge3D( const Coord3& p, const Coord3& a, const Coord3& b, 
			   double epsilon )
{
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
	const double newepsilon = plgknots[0].distTo(plgknots[1])*0.001;
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
	    
	    const double d1 = p1.abs();
	    const double d2 = p2.abs();
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
\ingroup Algo
\brief Quaternion is an extension to complex numbers.
  
  A Quaternion is represented by the equation:<br>
  q = s + xi + yj + zk <br>
  where: i*i = j*j = k*k = -1.
*/

mClass(Algo) Quaternion
{
public:
    			Quaternion(double s,double x,double y,double z);
			Quaternion(const Vector3& axis,float angle);

    void		setRotation(const Vector3& axis,float angle);
    void		getRotation(Vector3& axis,float& angle) const;
    			/*!<\note axis is not normalized. */
    Coord3		rotate(const Coord3&) const;

    Quaternion		operator+(const Quaternion&) const;
    Quaternion& 	operator+=(const Quaternion&);
    Quaternion		operator-(const Quaternion&) const;
    Quaternion& 	operator-=(const Quaternion&);
    Quaternion		operator*(const Quaternion&) const;
    Quaternion& 	operator*=(const Quaternion&);

    Quaternion		inverse() const;

    double		s_;
    Vector3		vec_;
};


/*!
\ingroup Algo
\brief A Line2 is a line on XY-plane, and it is defined in slope-intercept
form y = slope*x + y-intercept; for making operations easier.
*/

mClass(Algo) Line2
{
public:			
    			Line2(double slope=0,double intcpt=0);
    			Line2(const Coord&,double slope);
			Line2(const Coord&,const Coord&);

    bool		operator==(const Line2&) const;

    Coord		direction() const;	/*!<Normalized */

    Coord		closestPoint(const Coord& point) const;
    			/*!<\return the point on the line that is closest to
			 	     the given point */

    Coord		intersection(const Line2&,bool checkinlimit=true) const;

    double		distanceTo(const Line2&) const;
    			/*!<Gives distance to another parallel line */
    bool		getParallelLine(Line2& line,double dist) const;
			/*!<Gives a parallel line at a distance dist */
    bool		getPerpendicularLine(Line2& line,const Coord& pt) const;
    			/*!<Gives a perpendicular line through point pt*/
    bool		isOnLine(const Coord& pt) const;

    double		slope_;
    double		yintcpt_;

    bool		isvertical_;		/*!<Parallel to y-axis */
    double		xintcpt_;		/*!<only if isvertical_ true */

    Coord		start_;			/*!<For line-segments only */
    Coord		stop_;			/*!<For line-segments only */
};


/*!
\ingroup Algo
\brief A Line3 is a line in space, with the following equations:
  
  x = x0 + alpha*t
  y = y0 + beta*t
  z = z0 + gamma*t
*/

mClass(Algo) Line3
{
public:			
    			Line3();
    			Line3(	double x0, double y0, double z0,
				double alpha, double beta, double gamma );
    			Line3( const Coord3&, const Vector3& );

    Vector3		direction( bool normalize = true ) const
    			{
			    const Vector3 res( alpha_, beta_, gamma_ );
			    return normalize ? res.normalize() : res;
			}

    Coord3		getPoint(double t) const;
    bool		intersectWith( const Plane3&, double& t ) const;
    			/*!<Calculates the intersection between the line
			    and the plane. If success, it sets t. */
    			
 
    double		distanceToPoint( const Coord3& point ) const;
    double		sqDistanceToPoint( const Coord3& point ) const;
    double		closestPoint( const Coord3& point ) const;
    			/*!<\returns the point on the line that is closest to
			 	     the given point */
    void		closestPoint( const Line3& line, double& t_this,
	   			      double& t_line ) const;
    			/*!<\returns the t for the point point on the line
			   	     that is closest to the given line*/
 
    double		x0_;
    double		y0_;
    double		z0_;
    double		alpha_;
    double		beta_;
    double		gamma_;
};


/*!
\ingroup Algo
\brief A Plane3 is a plane in space, with the equation: Ax + By + Cz + D = 0
*/

mClass(Algo) Plane3
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
    bool		operator!=(const Plane3&) const;

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
\ingroup Algo
\brief Defines a 2D coordinate system on a 3D plane and transforms between the
3D space and the coordinate system.
*/

mClass(Algo) Plane3CoordSystem
{
public:
    			Plane3CoordSystem(const Coord3& normal,
					  const Coord3& origin,
				          const Coord3& pt10);
			/*!<\param normal The normal of the plane
			    \param origin A point on the plane
			    \param pt10   A point on the plane, not identical
			    		  to origin. */
    virtual 		~Plane3CoordSystem() {}
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
\ingroup Algo
\brief Represents a point in spherical coordinates.
The angle phi lies in the horizontal plane, theta in the vertical plane.
*/

mClass(Algo) Sphere
{
public:
			Sphere(float r=0,float t=0,float p=0)
			    : radius(r),theta(t),phi(p) {}

			Sphere(const Coord3& crd)
			    : radius((float) crd.x),theta((float) crd.y),
											phi((float) crd.z) {}
    bool		operator ==( const Sphere& s ) const;

    float		radius;
    float		theta;
    float		phi;
};


mGlobal(Algo) Sphere cartesian2Spherical(const Coord3&,bool math);
	    /*!< math=true: transformation done in math-system
		 math=false: transformation done in geo-system */
mGlobal(Algo) Coord3 spherical2Cartesian(const Sphere&,bool math);
	    /*!< math=true: transformation done in math-system
		 math=false: transformation done in geo-system */

inline bool Sphere::operator ==( const Sphere& s ) const
{
    const float dr = radius-s.radius;
    const float dt = theta-s.theta;
    const float dp = phi-s.phi;
    return mIsZero(dr,1e-8) && mIsZero(dt,1e-8) && mIsZero(dp,1e-8);
}


#endif

