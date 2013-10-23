#ifndef coord_h
#define coord_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		21-6-1996
 Contents:	Positions: Coordinates
 RCS:		$Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "geometry.h"
#include "undefval.h"


/*!
\brief A cartesian coordinate in 2D space.
*/

mExpClass(Basic) Coord : public Geom::Point2D<Pos::Ordinate_Type>
{
public:

    typedef Pos::Ordinate_Type	OrdType;
    typedef Pos::Distance_Type	DistType;

		Coord( const Geom::Point2D<OrdType>& p )
		    :  Geom::Point2D<OrdType>( p )			{}
		Coord() :  Geom::Point2D<OrdType>( 0, 0 )		{}
		Coord( OrdType cx, OrdType cy )
		    :  Geom::Point2D<OrdType>( cx, cy )			{}

    bool	operator==( const Coord& crd ) const
		{ return mIsEqual(x,crd.x,mDefEps)
		      && mIsEqual(y,crd.y,mDefEps); }
    bool	operator!=( const Coord& crd ) const
		{ return ! (crd == *this); }
    bool	operator<(const Coord&crd) const
		{ return x<crd.x || (x==crd.x && y<crd.y); }
    bool	operator>(const Coord&crd) const
		{ return x>crd.x || (x==crd.x && y>crd.y); }

    DistType	angle(const Coord& from,const Coord& to) const;
    DistType	cosAngle(const Coord& from,const Coord& to) const;
    		//!< saves the expensive acos() call
		//
    Coord	normalize() const;
    OrdType	dot(const Coord&) const;

    const char*	getUsrStr() const;
    bool	parseUsrStr(const char*);

    static const Coord& udf();
    inline bool isUdf() const		{ return !isDefined(); }
};



/*!
\brief A cartesian coordinate in 3D space.
*/

mExpClass(Basic) Coord3 : public Coord
{
public:

			Coord3() : z(0)					{}
			Coord3(const Coord& a, OrdType z_ )
			    : Coord(a), z(z_)				{}
			Coord3(const Coord3& xyz )
			    : Coord( xyz.x, xyz.y )
			    , z( xyz.z )				{}
    			Coord3( OrdType x_, OrdType y_, OrdType z_ )
			    : Coord(x_,y_), z(z_)			{}

    OrdType&		operator[]( int idx )
			{ return idx ? (idx==1 ? y : z) : x; }
    OrdType		operator[]( int idx ) const
			{ return idx ? (idx==1 ? y : z) : x; }

    inline Coord3	operator+(const Coord3&) const;
    inline Coord3	operator-(const Coord3&) const;
    inline Coord3	operator-() const;
    inline Coord3	operator*(double) const;
    inline Coord3	operator/(double) const;
    inline Coord3	scaleBy( const Coord3& ) const;
    inline Coord3	unScaleBy( const Coord3& ) const;

    inline Coord3&	operator+=(const Coord3&);
    inline Coord3&	operator-=(const Coord3&);
    inline Coord3&	operator/=(double);
    inline Coord3&	operator*=(double);
    inline Coord&	coord()				{ return *this; }
    inline const Coord&	coord() const			{ return *this; }

    inline bool		operator==(const Coord3&) const;
    inline bool		operator!=(const Coord3&) const;
    bool		isSameAs(const Coord3&, const Coord3&) const;

    DistType		distTo(const Coord3&) const;
    DistType		sqDistTo(const Coord3&) const;

    inline DistType	dot(const Coord3&) const;
    inline Coord3	cross(const Coord3&) const;
    DistType		abs() const;
    DistType		sqAbs() const;
    inline Coord3	normalize() const;

    static const Coord3& udf();
    inline bool		isDefined() const;
    inline bool		isUdf() const		{ return !isDefined(); }

    const char*		getUsrStr() const;
    bool		parseUsrStr(const char*);

    OrdType		z;

};


inline Coord3 operator*( double f, const Coord3& b )
{ return Coord3(b.x*f, b.y*f, b.z*f ); }


namespace Values {

/*!
\brief Undefined Coord.
*/

template<>
mClass(Basic) Undef<Coord>
{
public:
    static Coord	val()			{ return Coord::udf(); }
    static bool		hasUdf()		{ return true; }
    static bool		isUdf( Coord crd )	{ return !crd.isDefined(); }
    static void		setUdf( Coord& crd )	{ crd = Coord::udf(); }
};


/*!
\brief Undefined Coord3.
*/

template<>
mClass(Basic) Undef<Coord3>
{
public:
    static Coord3	val()			{ return Coord3::udf(); }
    static bool		hasUdf()		{ return true; }
    static bool		isUdf( Coord3 crd )	{ return !crd.isDefined(); }
    static void		setUdf( Coord3& crd )	{ crd = Coord3::udf(); }
};

} // namespace Values


inline bool Coord3::operator==( const Coord3& b ) const
{
    const DistType dx = x-b.x; const DistType dy = y-b.y;
    const DistType dz = z-b.z;
    return mIsZero(dx,mDefEps) && mIsZero(dy,mDefEps) && mIsZero(dz,mDefEps);
}


inline bool Coord3::operator!=( const Coord3& b ) const
{
    return !(b==*this);
}

inline bool Coord3::isDefined() const
{
    return !Values::isUdf(z) && Geom::Point2D<OrdType>::isDefined();
}


inline Coord3 Coord3::operator+( const Coord3& p ) const
{
    return Coord3( x+p.x, y+p.y, z+p.z );
}


inline Coord3 Coord3::operator-( const Coord3& p ) const
{
    return Coord3( x-p.x, y-p.y, z-p.z );
}


inline Coord3 Coord3::operator-() const
{
    return Coord3( -x, -y, -z );
}


inline Coord3 Coord3::operator*( double factor ) const
{ return Coord3( x*factor, y*factor, z*factor ); }


inline Coord3 Coord3::operator/( double denominator ) const
{ return Coord3( x/denominator, y/denominator, z/denominator ); }


inline Coord3 Coord3::scaleBy( const Coord3& factor ) const
{ return Coord3( x*factor.x, y*factor.y, z*factor.z ); }


inline Coord3 Coord3::unScaleBy( const Coord3& denominator ) const
{ return Coord3( x/denominator.x, y/denominator.y, z/denominator.z ); }


inline Coord3& Coord3::operator+=( const Coord3& p )
{
    x += p.x; y += p.y; z += p.z;
    return *this;
}


inline Coord3& Coord3::operator-=( const Coord3& p )
{
    x -= p.x; y -= p.y; z -= p.z;
    return *this;
}


inline Coord3& Coord3::operator*=( double factor )
{
    x *= factor; y *= factor; z *= factor;
    return *this;
}


inline Coord3& Coord3::operator/=( double denominator )
{
    x /= denominator; y /= denominator; z /= denominator;
    return *this;
}


inline Coord::DistType Coord3::dot(const Coord3& b) const
{ return x*b.x + y*b.y + z*b.z; }


inline Coord3 Coord3::cross(const Coord3& b) const
{ return Coord3( y*b.z-z*b.y, z*b.x-x*b.z, x*b.y-y*b.x ); }


inline Coord3 Coord3::normalize() const
{
    const DistType absval = abs();
    if ( absval < 1e-10 )
	return *this;

    return *this / absval;
}


#endif
