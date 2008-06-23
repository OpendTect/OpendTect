#ifndef trigonometry_h
#define trigonometry_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		23-11-2002
 RCS:		$Id: trigonometry.h,v 1.30 2008-06-23 18:31:45 cvskris Exp $
________________________________________________________________________


-*/

#include "position.h"

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
    const float para_pt = (d0.x*d2.y-d0.y*d2.x)/(d1.x*d2.y-d1.y*d2.x);
    const float para_bc = (d1.x*d0.y-d1.y*d0.x)/(d1.x*d2.y-d1.y*d2.x);

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
point and line, point and triangle, point and circle.
*/

inline bool isInsideCircle( const Coord& pt, 
			    const Coord& p1, const Coord& p2, const Coord& p3 )
    /*!<Check the point pt is inside the circumcircle of p1, p2, p3 or not. */
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


/*! Check p1, p2 are on the same side of the edge AB or not.*/
inline bool sameSide2D( const Coord& p1, const Coord& p2, 
			const Coord& a, const Coord& b, double epsilon )
{
    return ((p1.x-a.x)*(b.y-a.y)-(p1.y-a.y)*(b.x-a.x))*
	((p2.x-a.x)*(b.y-a.y)-(p2.y-a.y)*(b.x-a.x))>=-epsilon;
}


/*!< Check the point p is in the triangle ABC or not.*/
inline bool pointInTriangle2D( const Coord& p, const Coord& a, const Coord& b, 
			       const Coord& c, double epsilon )
{
    return sameSide2D(p,a,b,c,epsilon) && sameSide2D(p,b,a,c,epsilon) && 
	   sameSide2D(p,c,a,b,epsilon);
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

    const Coord intersectpt = a+t*ba;
    const double sqdist = p.sqDisTo( intersectpt );
    return sqdist<epsilon*epsilon 
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


/*!\brief Quaternion is an extension to complex numbers

 A Quaternion is represented by the equation:<br>
 q = s + xi + yj + zk <br>
 where: i*i = j*j = k*k = -1.
*/

class Quaternion
{
public:
    			Quaternion(float s,float x,float y,float z);
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

    float		s_;
    Vector3		vec_;
};

/*!\brief

A Line3 is a line in space, with the equations:

x = x0 + alpha*t
y = y0 + beta*t
z = z0 + gamma*t

*/


class Line3
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
    double		closestPoint( const Coord3& point ) const;
    			/*!<\returns the point on the line that is closest to
			 	     the given point */
    bool		closestPoint( const Line3& line, double& t_this,
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

/*!\brief

A Plane3 is a plane in space, with the equation:

Ax + By + Cz + D = 0

*/



class Plane3
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

    double		A_;
    double		B_;
    double		C_;
    double		D_;
};


/*!Defines a 2D coordinate system on a 3D plane, and transforms between the
   3D space and the coordiante system. */


class Plane3CoordSystem
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


/*!\brief Represents a point in spherical coordinates
  The angle phi lies in the horizontal plane, theta in the vertical plane.
*/

class Sphere
{
public:
			Sphere(float r=0,float t=0,float p=0)
			    : radius(r),theta(t),phi(p) {}

			Sphere(const Coord3& crd)
			    : radius(crd.x),theta(crd.y),phi(crd.z) {}
    bool		operator ==( const Sphere& s ) const;

    float		radius;
    float		theta;
    float		phi;
};


Sphere cartesian2Spherical(const Coord3&,bool math);
	    /*!< math=true: transformation done in math-system
		 math=false: transformation done in geo-system */
Coord3 spherical2Cartesian(const Sphere&,bool math);
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
