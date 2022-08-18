#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "gendefs.h"
#include "geometry.h"

namespace Geom
{

template <class T>
mClass(Basic) Point3D
{
public:
				Point3D(const Point2D<T>&,T);
				Point3D(T xx=0,T yy=0, T zz=0);

    template <class TT>
    Point3D<T>&			setFrom(const Point3D<TT>&);

    template <class TT>
    inline void			setXY(TT xx,TT yy);
    inline void			setXY(T xx,T yy);
    inline void			setXY(const Point2D<T>&);
    inline Point2D<T>		getXY() const { return Point2D<T>( x_, y_ ); }
    inline Point3D<T>&		zero();
    inline Point3D<T>		operator-() const;

    inline T&			operator[](int idx);
    inline T			operator[](int idx) const;

    inline bool			operator==(const Point3D&) const;
    inline bool			operator!=(const Point3D&) const;
    inline bool			isSameAs(const Point3D& pos,
					 const Point3D& eps) const;

    inline Point3D<T>&		operator+=(T dist);
    inline Point3D<T>&		operator*=(T factor);
    inline Point3D<T>&		operator/=(T den);
    inline Point3D<T>&		operator+=(const Point3D<T>&);
    inline Point3D<T>&		operator-=(const Point3D<T>&);
    inline Point3D<T>		operator+(const Point3D<T>&) const;
    inline Point3D<T>		operator-(const Point3D<T>&) const;
    inline Point3D<T>		operator*(const T factor) const;
    inline Point3D<T>		operator/(const T den) const;

    inline Point3D<T>		scaleBy(const Point3D<T>& factor) const;
    inline Point3D<T>		unScaleBy(const Point3D<T>& denominator) const;

    inline T			dot(const Point3D<T>&) const;
    inline Point3D<T>		cross(const Point3D<T>&) const;
    inline Point3D<T>		normalize() const;
				//!<Returns vector with length one.
    inline bool			isDefined() const;
    inline bool			isUdf() const { return !isDefined(); }
    template <class FT> FT	abs() const;
    inline T			sqAbs() const;
		//!<Squared absolute value
    template <class FT> FT	distTo(const Point3D<T>&) const;
    inline T			sqDistTo(const Point3D<T>&) const;
				//!<Squared distance

    template <class FT> FT	xyDistTo(const Point3D<T>&) const;
				//!<Distance in the xy plane
    inline T			xySqDistTo(const Point3D<T>&) const;
				//!<Square distance in the xy plane
    template <class FT> FT	xyDistTo(const Point2D<T>&) const;
				//!<Distance in the xy plane
    inline T			xySqDistTo(const Point2D<T>&) const;
				//!<Square distance in the xy plane

    BufferString		toString() const;
    BufferString		toPrettyString() const	{ return toString(); }

    static Point3D<T>		udf();
    void			setUdf()		{ *this = udf(); }

    T				x_;
    T				y_;
    T				z_;
};

template <class T>
inline Point3D<T> operator*( int f, const Point3D<T>& b )
{ return Point3D<T>(b.x_*f, b.y_*f, b.z_*f ); }

template <class T>
inline Point3D<T> operator*( double f, const Point3D<T>& b )
{ return Point3D<T>(b.x_*f, b.y_*f, b.z_*f ); }


template <class T>
inline Point3D<T> operator*( float f, const Point3D<T>& b )
{ return Point3D<T>(b.x_*f, b.y_*f, b.z_*f ); }


template <class T> inline
Point3D<T>::Point3D( const Point2D<T>& p, T z)
    : x_( p.x_ )
    , y_( p.y_ )
    , z_( z )
{}


template <class T> inline
Point3D<T>::Point3D( T x, T y, T z)
    : x_( x )
    , y_( y )
    , z_( z )
{}


template <class T> template <class TT> inline
Point3D<T>& Point3D<T>::setFrom( const Point3D<TT>& a )
{ x_=(T)a.x_; y_=(T)a.y_; z_=(T)a.z_;	return *this;}


template <class T> template <class TT> inline
void Point3D<T>::setXY( TT xx, TT yy )
{ x_ = (T)xx; y_ = (T)yy; }


template <class T> inline
void Point3D<T>::setXY( T xx, T yy )
{ x_ = xx ; y_ = yy; }


template <class T> inline
void Point3D<T>::setXY( const Point2D<T>& p )
{ x_ = p.x_ ; y_ = p.y_; }

template <class T> inline
Point3D<T>& Point3D<T>::zero()
{
    x_ =  y_ = z_ = 0;
    return *this;
}



template <class T> inline
bool Point3D<T>::operator==( const Point3D<T>& b ) const
{
    const T dx = x_-b.x_;
    const T dy = y_-b.y_;
    const T dz = z_-b.z_;
    return mIsZero(dx,mDefEps) && mIsZero(dy,mDefEps) && mIsZero(dz,mDefEps);
}


template <class T> inline
bool Point3D<T>::operator!=( const Point3D<T>& oth ) const
{
    return !(*this==oth);
}


template <class T> inline
bool Point3D<T>::isSameAs( const Point3D<T>& pos,
			      const Point3D<T>& eps ) const
{
    return fabs(x_-pos.x_)<eps.x_ &&
	   fabs(y_-pos.y_)<eps.y_ &&
	   fabs(z_-pos.z_)<eps.z_;
}


template <class T> inline
BufferString Point3D<T>::toString() const
{
    if ( isUdf() )
	return BufferString( "<undef>" );
    BufferString res( "(", x_, "," );
    res.add( y_ ).add( "," ).add( z_ ).add( ')' );
    return res;
}


template <> inline
BufferString Point3D<float>::toPrettyString() const
{
    if ( !isDefined() )
	return toString();

    const Point3D<od_int64> pt( mRounded(od_int64,x_), mRounded(od_int64,y_),
				mRounded(od_int64,z_) );
    return pt.toString();
}


template <> inline
BufferString Point3D<double>::toPrettyString() const
{
    const Point3D<od_int64> pt( mRounded(od_int64,x_), mRounded(od_int64,y_),
				mRounded(od_int64,z_) );
    return pt.toString();
}


template <class T> inline
T& Point3D<T>::operator[]( int idx )
{
    if ( !idx )
	return x_;
    if ( idx==1 )
	return y_;
    return z_;
}

template <class T> inline
T Point3D<T>::operator[]( int idx ) const
{
    if ( !idx )
	return x_;
    if ( idx==1 )
	return y_;
    return z_;
}


template <class T> inline
bool Point3D<T>::isDefined() const
{
    return !Values::isUdf(x_) && !Values::isUdf(y_) && !Values::isUdf(z_);
}


template <class T>
inline Point3D<T> Point3D<T>::operator+( const Point3D<T>& p ) const
{
    return Point3D<T>( x_+p.x_, y_+p.y_, z_+p.z_ );
}


template <class T>
inline Point3D<T> Point3D<T>::operator-( const Point3D<T>& p ) const
{
    return Point3D<T>( x_-p.x_, y_-p.y_, z_-p.z_ );
}

template <class T>
inline Point3D<T> Point3D<T>::operator-() const
{
    return Point3D<T>( -x_, -y_, -z_ );
}

template <class T>
inline Point3D<T> Point3D<T>::operator*( T factor ) const
{ return Point3D<T>( x_*factor, y_*factor, z_*factor ); }


template <class T>
inline Point3D<T> Point3D<T>::operator/( T denominator ) const
{ return Point3D<T>( x_/denominator, y_/denominator, z_/denominator ); }


template <class T>
inline Point3D<T> Point3D<T>::scaleBy( const Point3D<T>& factor ) const
{ return Point3D<T>( x_*factor.x_, y_*factor.y_, z_*factor.z_ ); }


template <class T>
inline Point3D<T>
Point3D<T>::unScaleBy( const Point3D<T>& denominator ) const
{ return Point3D<T>( x_/denominator.x_,
			y_/denominator.y_,
		       z_/denominator.z_ );
}


template <class T> inline
Point3D<T>& Point3D<T>::operator+=(T dist)
{
    x_ += dist; y_ += dist; z_ += dist; return *this;
}

template <class T> inline
Point3D<T>& Point3D<T>::operator+=( const Point3D<T>& p )
{
    x_ += p.x_; y_ += p.y_; z_ += p.z_;
    return *this;
}


template <class T> inline
Point3D<T>& Point3D<T>::operator-=( const Point3D<T>& p )
{
    x_ -= p.x_; y_ -= p.y_; z_ -= p.z_;
    return *this;
}


template <class T> inline
Point3D<T>& Point3D<T>::operator*=( T factor )
{
    x_ *= factor; y_ *= factor; z_ *= factor;
    return *this;
}


template <class T> inline
Point3D<T>& Point3D<T>::operator/=( T denominator )
{
    x_ /= denominator; y_ /= denominator; z_ /= denominator;
    return *this;
}


template <class T> inline
T Point3D<T>::dot(const Point3D<T>& b) const
{ return x_*b.x_ + y_*b.y_ + z_*b.z_; }



template <class T> inline
Point3D<T> Point3D<T>::cross(const Point3D<T>& b) const
{ return Point3D<T>( y_*b.z_-z_*b.y_, z_*b.x_-x_*b.z_, x_*b.y_-y_*b.x_ ); }


template <class T> inline
Point3D<T> Point3D<T>::normalize() const
{
    const T sqabsval = sqAbs();
    if ( sqabsval == 0 )
	return *this;

    return *this / Math::Sqrt( sqabsval );
}


template <class T> template <class FT> inline
FT Point3D<T>::abs() const
{ return ::Math::Sqrt( (FT) sqAbs() ); }


template <class T> inline
T Point3D<T>::sqAbs() const
{
    return x_*x_ + y_*y_ + z_*z_;
}


template <class T> template <class FT> inline
FT Point3D<T>::distTo( const Point3D<T>& pt ) const
{ return ::Math::Sqrt( (FT)sqDistTo(pt) ); }


template <class T> inline
T Point3D<T>::sqDistTo( const Point3D<T>& pt ) const
{
    const T xdiff = x_-pt.x_;
    const T ydiff = y_-pt.y_;
    const T zdiff = z_-pt.z_;
    return xdiff*xdiff + ydiff*ydiff + zdiff*zdiff;
}


template <class T> template <class FT> inline
FT Point3D<T>::xyDistTo( const Point3D<T>& pt ) const
{ return ::Math::Sqrt( (FT)xySqDistTo(pt) ); }


template <class T> inline
T Point3D<T>::xySqDistTo( const Point3D<T>& pt ) const
{
    return xySqDistTo( pt.getXY() );
}

template <class T> template <class FT> inline
FT Point3D<T>::xyDistTo( const Point2D<T>& pt ) const
{ return ::Math::Sqrt( (FT)xySqDistTo(pt) ); }


template <class T> inline
T Point3D<T>::xySqDistTo( const Point2D<T>& pt ) const
{
    return getXY().sqDistTo( pt );
}


template <class T> inline
Point3D<T> Point3D<T>::udf()
{ return Point3D<T>(mUdf(T),mUdf(T), mUdf(T)); }

} // namespace Geom
