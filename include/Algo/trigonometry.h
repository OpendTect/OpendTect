#ifndef trigonometry_h
#define trigonometry_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		23-11-2002
 RCS:		$Id: trigonometry.h,v 1.11 2003-12-27 10:20:04 kristofer Exp $
________________________________________________________________________


-*/

#include "position.h"

template <class T> class TypeSet;
template <class T> class ObjectSet;

/*!\brief


*/

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

class Quarternion
{
public:
    			Quarternion( float s, float x, float y, float z );
			Quarternion( float s, const Vector3& vec );

    Quarternion 	operator+( const Quarternion& ) const;
    Quarternion& 	operator+=( const Quarternion& );
    Quarternion 	operator-( const Quarternion& ) const;
    Quarternion& 	operator-=( const Quarternion& );
    Quarternion 	operator*( const Quarternion& ) const;
    Quarternion& 	operator*=( const Quarternion& );

    Quarternion		inverse() const;

    float		s;
    Vector3		vec;
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
    			Line3(	float x0, float y0, float z0,
				float alpha, float beta, float gamma );
    			Line3( const Coord3&, const Vector3& );

    Vector3		direction() const
    			{
			    Vector3 res( alpha, beta, gamma );
			    res.normalize(); 
			    return res;
			}
 
    float               distanceToPoint( const Coord3& point ) const;
 
    float		x0;
    float		y0;
    float		z0;
    float		alpha;
    float		beta;
    float		gamma;
};

/*!\brief

A Plane3 is a plane in space, with the equation:

Ax + By + Cz + D = 0

*/



class Plane3
{
public:
			Plane3();
			Plane3(float, float, float, float);
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

    Coord3		normal() const { return Coord3( A, B, C ); }
 
    float               distanceToPoint( const Coord3& ) const;
    bool		intersectWith( const Line3&, Coord3& ) const;
    			/*!< Returns true if the plane intersects with the
			     line. If it returns true, the Coord3 is set */
    bool		intersectWith( const Plane3&, Line3& ) const;
    			/*!< Returns true if the planes intersects.
			     If it returns true, the Line3 is set */

    float		A;
    float		B;
    float		C;
    float		D;
};

#endif
