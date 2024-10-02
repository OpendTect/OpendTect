#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
using OrdType = Pos::Ordinate_Type;
using DistType = Pos::Distance_Type;

		Coord();
		Coord(const Geom::Point2D<OrdType>&);
		Coord(OrdType cx,OrdType cy);
		~Coord();

    bool	operator==( const Coord& crd ) const
    { return mIsEqual(x_,crd.x_,mDefEps)
		&& mIsEqual(y_,crd.y_,mDefEps); }
    bool	operator!=( const Coord& crd ) const
		{ return ! (crd == *this); }
    bool	operator<(const Coord&crd) const
    { return x_<crd.x_ || (x_==crd.x_ && y_<crd.y_); }
    bool	operator>(const Coord&crd) const
    { return x_>crd.x_ || (x_==crd.x_ && y_>crd.y_); }

    DistType	horDistTo(const Coord&) const;
    DistType	sqHorDistTo(const Coord&) const;
		//!< saves the expensive sqrt call
    DistType	angle(const Coord& from,const Coord& to) const;
    DistType	cosAngle(const Coord& from,const Coord& to) const;
		//!< saves the expensive acos() call
		//
    Coord	normalize() const;
    OrdType	dot(const Coord&) const;

    const char*	toString() const;
    const char*	toPrettyString(int nrdec=2) const;
    bool	fromString(const char*);

    static const Coord& udf();
    inline bool isUdf() const		{ return !isDefined(); }
    inline void setUdf()		{ *this = udf(); }
};



/*!
\brief A cartesian coordinate in 3D space.
*/

mExpClass(Basic) Coord3 : public Coord
{
public:
			Coord3();
			Coord3(const Coord&,OrdType);
			Coord3(const Coord3& xyz );
			Coord3( OrdType _x, OrdType _y, OrdType _z );
			~Coord3();

    OrdType&		operator[]( int idx )
    { return idx ? (idx==1 ? y_ : z_) : x_; }
    OrdType		operator[]( int idx ) const
    { return idx ? (idx==1 ? y_ : z_) : x_; }

    inline Coord3&	operator=(const Coord3&);
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
    inline void		setUdf()		{ *this = udf(); }
    inline bool		isNull() const;

    const char*		toString() const;
    bool		fromString(const char*);

    OrdType		z_;

    mDeprecated("Use z_")
    OrdType&		z;

};


inline Coord3 operator*( double f, const Coord3& b )
{ return Coord3(b.x_*f, b.y_*f, b.z_*f ); }


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
    static bool		isUdf( const Coord3& crd )  { return !crd.isDefined(); }
    static void		setUdf( Coord3& crd )	{ crd = Coord3::udf(); }
};

} // namespace Values


inline bool Coord3::operator==( const Coord3& b ) const
{
    const DistType dx = x_-b.x_;
    const DistType dy = y_-b.y_;
    const DistType dz = z_-b.z_;
    return mIsZero(dx,mDefEps) && mIsZero(dy,mDefEps) && mIsZero(dz,mDefEps);
}


inline bool Coord3::operator!=( const Coord3& b ) const
{
    return !(b==*this);
}

inline bool Coord3::isDefined() const
{
    return !Values::isUdf(z_) && Geom::Point2D<OrdType>::isDefined();
}


inline bool Coord3::isNull() const
{
    return mIsZero(x_,mDefEps) && mIsZero(y_,mDefEps) && mIsZero(z_,mDefEps);
}


inline Coord3& Coord3::operator=( const Coord3& p )
{
    x_ = p.x_;
    y_ = p.y_;
    z_ = p.z_;
    return *this;
}


inline Coord3 Coord3::operator+( const Coord3& p ) const
{
    return Coord3( x_+p.x_, y_+p.y_, z_+p.z_ );
}


inline Coord3 Coord3::operator-( const Coord3& p ) const
{
    return Coord3( x_-p.x_, y_-p.y_, z_-p.z_ );
}


inline Coord3 Coord3::operator-() const
{
    return Coord3( -x_, -y_, -z_ );
}


inline Coord3 Coord3::operator*( double factor ) const
{ return Coord3( x_*factor, y_*factor, z_*factor ); }


inline Coord3 Coord3::operator/( double denominator ) const
{ return Coord3( x_/denominator, y_/denominator, z_/denominator ); }


inline Coord3 Coord3::scaleBy( const Coord3& factor ) const
{ return Coord3( x_*factor.x_, y_*factor.y_, z_*factor.z_ ); }


inline Coord3 Coord3::unScaleBy( const Coord3& denominator ) const
{ return Coord3( x_/denominator.x_, y_/denominator.y_, z_/denominator.z_ ); }


inline Coord3& Coord3::operator+=( const Coord3& p )
{
    x_ += p.x_; y_ += p.y_; z_ += p.z_;
    return *this;
}


inline Coord3& Coord3::operator-=( const Coord3& p )
{
    x_ -= p.x_; y_ -= p.y_; z_ -= p.z_;
    return *this;
}


inline Coord3& Coord3::operator*=( double factor )
{
    x_ *= factor; y_ *= factor; z_ *= factor;
    return *this;
}


inline Coord3& Coord3::operator/=( double denominator )
{
    x_ /= denominator; y_ /= denominator; z_ /= denominator;
    return *this;
}


inline Coord::DistType Coord3::dot(const Coord3& b) const
{ return x_*b.x_ + y_*b.y_ + z_*b.z_; }


inline Coord3 Coord3::cross(const Coord3& b) const
{ return Coord3( y_*b.z_-z_*b.y_, z_*b.x_-x_*b.z_, x_*b.y_-y_*b.x_ ); }


inline Coord3 Coord3::normalize() const
{
    const DistType absval = abs();
    if ( absval < 1e-10 )
	return *this;

    return *this / absval;
}
