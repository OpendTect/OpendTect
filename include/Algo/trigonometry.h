#ifndef trigonometry_h
#define trigonometry_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		23-11-2002
 RCS:		$Id: trigonometry.h,v 1.2 2003-04-14 08:41:23 kristofer Exp $
________________________________________________________________________


-*/

class Coord3;
template <class T> class TypeSet;
template <class T> class ObjectSet;

/*!\brief


*/

class Vector3 
{
public:
    			Vector3();
    			Vector3( float x, float y, float z );

    Vector3		operator+(const Vector3&) const;
    Vector3&		operator+=(const Vector3&);
    Vector3		operator-(const Vector3&) const;
    Vector3&		operator-=(const Vector3&);
    Vector3		operator*(float) const;
    Vector3&		operator*=(float);
    Vector3		operator/(float) const;
    Vector3&		operator/=(float);
    bool		operator==(const Vector3&) const;

    float		dot( const Vector3& ) const;
    Vector3		cross( const Vector3& ) const;
    double		abs() const;
    Vector3&		normalize();

    float		x;
    float		y;
    float		z;
};


Vector3 operator*( float, const Vector3& b );

/*! \brief
Divides a sphere in a number of vectors, divided by approximately dangle from
each other. dradius is given in radians
*/

ObjectSet<Vector3>* makeSphereVectorSet( double dangle );

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
 
    float               distanceToPoint( Coord3& point ) const;
 
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
			Plane3( const Vector3& normal, const Coord3& );
			Plane3( const Vector3&, const Vector3& );
			Plane3( const Coord3&, const Coord3&, const Coord3& );
			Plane3( const TypeSet<Coord3>& );

    void		set( const Vector3& normal, const Coord3& );
    void		set( const Vector3&, const Vector3& );
    void		set( const Coord3&, const Coord3&, const Coord3& );
    void		set( const TypeSet<Coord3>& );

    Vector3		normal() const { return Vector3( A, B, C ); }
 
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
