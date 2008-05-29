#ifndef trigonometry_h
#define trigonometry_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		23-11-2002
 RCS:		$Id: trigonometry.h,v 1.24 2008-05-29 18:59:05 cvskris Exp $
________________________________________________________________________


-*/

#include "position.h"

template <class T> class TypeSet;
template <class T> class ObjectSet;
class Plane3;

/*!\brief
Here are some commonly used functions to judge the position relation between 
point and line, point and triangle, point and circle.
*/

inline float deter33( const float* matrix )
/*!<calculate the determinant of matrix. the 1st row is m0, m1, m2,
 					 the 2nd row is m3, m4, m5,
					 the 3rd row is m6, m7, m8. */
{
    return matrix[0]*matrix[4]*matrix[8]+matrix[1]*matrix[5]*matrix[6]+
	   matrix[2]*matrix[3]*matrix[7]-matrix[2]*matrix[4]*matrix[6]-
   	   matrix[0]*matrix[5]*matrix[7]-matrix[1]*matrix[3]*matrix[8];	   
}


inline bool isInsideCircle( const Coord& pt, 
			    const Coord& p1, const Coord& p2, const Coord& p3 )
    /*!<Check the point pt is inside the circumcircle of p1, p2, p3 or not. */
{
    float m0[] = { p1.x, p1.y, 1, p2.x, p2.y, 1, p3.x, p3.y, 1 };
    const float a = deter33( m0 );
    float m1[] = { p1.dot(p1), p1.y, 1, p2.dot(p2), p2.y, 1, 
		   p3.dot(p3), p3.y, 1 };
    float m2[] = { p1.dot(p1), p1.x, 1, p2.dot(p2), p2.x, 1, 
		   p3.dot(p3), p3.x, 1 };
    float m3[] = { p1.dot(p1), p1.x, p1.y, p2.dot(p2), p2.x, p2.y, 
		   p3.dot(p3), p3.x, p3.y };

    return a*( a*pt.dot(pt)-deter33(m1)*pt.x+deter33(m2)*pt.y-deter33(m3) )<0;
}


inline bool sameSide2D( const Coord& p1, const Coord& p2, 
			const Coord& a, const Coord& b )
    /*!< Check p1, p2 are on the same side of the edge AB or not.*/
{
    return ((p1.x-a.x)*(b.y-a.y)-(p1.y-a.y)*(b.x-a.x))*
	((p2.x-a.x)*(b.y-a.y)-(p2.y-a.y)*(b.x-a.x))>=0 ? true : false;
}


inline bool pointInTriangle2D( const Coord& p, 
			       const Coord& a, const Coord& b, const Coord& c )
    /*!< Check the point p is in the triangle ABC or not.*/
{
    if ( sameSide2D(p,a,b,c) && sameSide2D(p,b,a,c) && sameSide2D(p,c,a,b) )
	return true;
    else
	return false;
}


inline bool pointOnEdge2D( const Coord& p, const Coord& a, const Coord& b, 
			   double epsilon )
    /*!< Check to see if the point P in on the edge AB or not.*/
{
    if ( (p-a).x*(b-p).x<0 || (p-a).y*(b-p).y<0 )
	return false;

    if ( mIsZero((p-a).x*(b-p).y-(b-p).x*(p-a).y, epsilon) )
	return true;
    else
	return false;
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

    Vector3		direction() const
    			{
			    const Vector3 res( alpha_, beta_, gamma_ );
			    return res.normalize();
			}

    Coord3		getPoint(double t) const;
    bool		intersectWith( const Plane3&, double& t ) const;
    			/*!<Calculates the intersection between the line
			    and the plane. If success, it sets t. */
    			
 
    double		distanceToPoint( const Coord3& point ) const;
    double		closestPoint( const Coord3& point ) const;
    			/*!<\returns the point on the line that is closest to
			 	     the given point */
    bool		closestPoint( const Line3& line, double& t ) const;
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
