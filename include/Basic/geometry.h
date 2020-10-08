#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          01/02/2000
________________________________________________________________________

-*/

#include "gendefs.h"
#include "ranges.h"
#include "math2.h"

namespace Geom
{

/*!\brief Point in 2D (i.e. having X and Y). */

template <class T>
mClass(Basic) Point2D
{
public:
				Point2D(T xx=0,T yy=0);

    template <class TT>
    Point2D<T>&			setFrom(const Point2D<TT>&);

    template <class TT>
    inline void			setXY(TT xx,TT yy);
    inline void			setXY(T xx,T yy);
    inline Point2D<T>&		zero();
    inline Point2D<T>		operator-() const;

    inline T&			operator[](int idx);
    inline T			operator[](int idx) const;

    inline bool			operator==(const Point2D&) const;
				mImplSimpleIneqOper(Point2D)
    inline Point2D<T>&		operator+=(T dist);
    inline Point2D<T>&		operator*=(T factor);
    inline Point2D<T>&		operator/=(T den);
    inline Point2D<T>&		operator+=(const Point2D<T>&);
    inline Point2D<T>&		operator-=(const Point2D<T>&);
    inline Point2D<T>		operator+(const Point2D<T>&) const;
    inline Point2D<T>		operator-(const Point2D<T>&) const;
    inline Point2D<T>		operator*(const T factor) const;
    inline Point2D<T>		operator/(const T den) const;
    inline bool			operator<(const Point2D<T>& crd) const;
    inline bool			operator>(const Point2D<T>& crd) const;

    inline void			swapXY();

    inline T			dot(const Point2D<T>&) const;
    inline bool			isDefined() const;
    inline bool			isUdf() const { return !isDefined(); }
    inline Point2D<T>		normalize() const;
    template <class FT> FT	abs() const;
    inline T			sqAbs() const;
				//!<Squared absolute value
    template <class FT> FT	distTo(const Point2D<T>&) const;
    inline T			sqDistTo(const Point2D<T>&) const;
				//!<Squared distance
    template <class FT> FT	angle(const Point2D<T>& from,
				      const Point2D<T>& to) const;
    template <class FT> FT	cosAngle(const Point2D<T>& from,
				      const Point2D<T>& to) const;
				//!< saves the expensive acos() call

    BufferString		toString() const;
    BufferString		toPrettyString() const	{ return toString(); }
    bool			fromString(const char*);

    static Point2D<T>		udf() { return Point2D<T>(mUdf(T),mUdf(T)); }
    void			setUdf() { *this = udf(); }

    T				x_;
    T				y_;
};


template <class T> inline
Point2D<T> operator*( int f, const Point2D<T>& b )
{ return Point2D<T>( b.x_*f, b.y_*f ); }

template <class T> inline
Point2D<T> operator*( double f, const Point2D<T>& b )
{ return Point2D<T>( b.x_*f, b.y_*f ); }

template <class T> inline
Point2D<T> operator*( float f, const Point2D<T>& b )
{ return Point2D<T>( b.x_*f, b.y_*f ); }



/*!\brief Point in 3D (i.e. having X, Y and Z). */

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
				mImplSimpleIneqOper(Point3D)
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


/*!\brief 2D sizes (width/height). */

template <class T>
mClass(Basic) Size2D
{
public:
			Size2D( T w = 0 , T h = 0 );

    inline bool		operator==(const Size2D&) const;
			mImplSimpleIneqOper(Size2D)

    inline T		width() const;
    inline T		height() const;
    inline void		setWidth(T val);
    inline void		setHeight(T val);
    inline void		set(T w,T h);
    inline Size2D<T>	operator+(T val) const;
    inline Size2D<T>&	operator+=(T val);
    inline Size2D<T>&	operator-=(T val);
    inline Size2D<T>&	operator+=(const Size2D<T>&);
    inline Size2D<T>&	operator-=(const Size2D<T>&);

protected:

    T	width_;
    T	height_;

};


/*!\brief 2D rectangles.

  This class is a bit more complicated than would be expected at first sight.
  This is caused by the problem of coord system sign. For example, in
  user interfaces, top is a lower number than bottom. But for normal
  coordinates, this is (of course) not the case. Still, also for floating point
  types, reverse axes are common.
*/

template <class T>
mClass(Basic) Rectangle
{
public:
			Rectangle(T l=0,T t=0,T r=0,T b=0) ;
			Rectangle(const Point2D<T>& tl,const Point2D<T>& br);
			Rectangle(const Point2D<T>& tl,const Size2D<T>& sz);

    inline bool		operator==(const Rectangle&) const;
			mImplSimpleIneqOper(Rectangle)

    inline Point2D<T>	topLeft() const;
    inline Point2D<T>	topRight() const;
    inline Point2D<T>	bottomLeft() const;
    inline Point2D<T>	bottomRight() const;
    inline Point2D<T>	centre() const;
    inline void		setTopLeft(Point2D<T>);
    inline void		setBottomRight(Point2D<T>);
    inline void		setTopRight(Point2D<T>);
    inline void		setBottomLeft(Point2D<T>);
    inline void		setTopBottom(const Interval<T>&);
    inline void		setLeftRight(const Interval<T>&);

    inline Point2D<T>	moveInside(const Point2D<T>&) const;

    inline void		include(const Rectangle<T>&);
    inline void		include(const Point2D<T>&);
    inline void		limitTo(const Rectangle<T>&);
    inline void		translate(const Point2D<T>&);

    inline bool		operator >(const Rectangle<T>&) const;

    inline T		width() const;
    inline T		height() const;

    inline T		left() const;
    inline T		top() const;
    inline T		right() const;
    inline T		bottom() const;
    inline void		setLeft(T val);
    inline void		setTop(T val);
    inline void		setRight(T val);
    inline void		setBottom(T val);

    bool		checkCorners(bool leftislow=true,
				     bool topislow=true) const;
			//!\returns if the corners are consistent
    void		sortCorners(bool leftislow=true,bool topislow=true);
    inline Size2D<T>	size() const;
    inline void		zero();
    inline bool		isDefined() const;

    inline Rectangle<T>& operator+=(const Point2D<T>&); // shifts
    inline Rectangle<T>& operator-=(const Point2D<T>&);
    inline Rectangle<T>& operator+=(const Size2D<T>&); // keeps topleft in place
    inline Rectangle<T>& operator-=(const Size2D<T>&);

    inline void		swapHor();
    inline void		swapVer();

    inline bool		revX() const;
    inline bool		revY() const;

protected:

    Point2D<T>		topleft_;
    Point2D<T>		bottomright_;

};


/*!\brief Integer rectangles.

  The difference with the floating point type rectangle is in range handling.
  In the float world, everything must be epsiloned. Integer rectangles are
  more straightforward.
*/

template <class T>
mClass(Basic) PixRectangle : public Rectangle<T>
{
public:
			PixRectangle(T l=0 , T t=0, T r=0 , T b=0 ) ;
			PixRectangle(const Point2D<T>& tl,const Point2D<T>& br);
			PixRectangle(const Point2D<T>& tl,const Size2D<T>& sz);
			PixRectangle(const Rectangle<T>&);

    inline bool		isInside(const Point2D<T>&) const;
    inline bool		isOutside(const Point2D<T>&) const;
    inline bool		isOnSide(const Point2D<T>&) const;
    inline bool		contains(const Point2D<T>&) const;
    inline bool		contains(const PixRectangle<T>&) const;
    inline bool		isInside(const PixRectangle<T>&) const;

    inline PixRectangle<T> grownBy(double sidesincreasebyfactor=1) const;

protected:

    inline bool		xOutside(T) const;
    inline bool		yOutside(T) const;
};


/*!\brief Floating-point rectangles.

  The difference with the integer type rectangle is in range handling. In the
  float world, everything must be epsiloned.
  with inside and outside.
*/

template <class T>
mClass(Basic) PosRectangle : public Rectangle<T>
{
public:
			PosRectangle( T l = 0 , T t = 0, T r = 0 , T b = 0 )
			: Rectangle<T>(l,t,r,b)		{}
			PosRectangle( Point2D<T> tl, Point2D<T> br )
			: Rectangle<T>(tl,br)		{}

    inline bool		isOutside( const Point2D<T>& p, T eps ) const
			{ return xOutside(p.x_,eps) || yOutside(p.y_,eps); }
    inline bool		isInside(const Point2D<T>&,T eps) const;
    inline bool		isOnSide(const Point2D<T>& p,T eps) const;

    inline bool		contains( const Point2D<T>& p, T eps ) const
			{ return !isOutside(p,eps); }

    inline bool		contains( const PosRectangle<T>& other, T eps ) const
			{
			    return contains(other.topleft_,eps)
				&& contains(other.bottomright_,eps);
			}
    inline bool		isInside( const PosRectangle<T>& other, T eps ) const
			{
			    return other.isInside(this->topleft_,eps)
				&& other.isInside(this->bottomright_,eps);
			}

    inline PosRectangle<T> grownBy(T sidesincreasebyfactor=1) const;

protected:

    inline bool		xOutside(T,T) const;
    inline bool		yOutside(T,T) const;
};


template <class T> inline
Point2D<T>::Point2D ( T xx , T yy )
    : x_(xx), y_(yy)
{}

template <class T> template <class TT> inline
Point2D<T>& Point2D<T>::setFrom( const Point2D<TT>& a )
{ x_=(T) a.x_; y_=(T) a.y_; return *this;}

template <class T> inline
void Point2D<T>::setXY( T xx, T yy )
{ x_ = xx ; y_ = yy; }

template <class T> template <class TT> inline
void Point2D<T>::setXY( TT xx, TT yy )
{ x_ = (T)xx; y_ = (T)yy; }

template <class T> inline
Point2D<T>& Point2D<T>::zero()
{ x_ = y_ = 0; return *this; }

template <class T> inline
Point2D<T> Point2D<T>::operator -() const
{ return Point2D<T>( -x_, -y_ ); }

template <class T> inline
T& Point2D<T>::operator[]( int idx )
{ return idx ? y_ : x_; }

template <class T> inline
T Point2D<T>::operator[]( int idx ) const
{ return idx ? y_ : x_; }

template <class T> inline
bool Point2D<T>::operator ==( const Point2D<T>& p ) const
{ return p.x_ == x_ && p.y_ == y_; }

template <class T> inline
bool Point2D<T>::operator<(const Point2D<T>& crd) const
{ return x_<crd.x_ || (x_==crd.x_ && y_<crd.y_); }

template <class T> inline
bool Point2D<T>::operator>(const Point2D<T>& crd) const
{ return x_>crd.x_ || (x_==crd.x_ && y_>crd.y_); }


template <class T> inline
Point2D<T>& Point2D<T>::operator+=( T dist )
{ x_ += dist; y_ += dist; return *this; }


template <class T> inline
Point2D<T>& Point2D<T>::operator*=( T factor )
{ x_ *= factor; y_ *= factor; return *this; }


template <class T> inline
Point2D<T>& Point2D<T>::operator/=( T den )
{ x_ /= den; y_ /= den; return *this; }


template <class T> inline
Point2D<T>& Point2D<T>::operator +=( const Point2D<T>& p )
{ x_ += p.x_; y_ += p.y_; return *this; }


template <class T> inline
Point2D<T>& Point2D<T>::operator -=( const Point2D<T>& p )
{ x_ -= p.x_; y_ -= p.y_; return *this; }


template <class T> inline
Point2D<T> Point2D<T>::operator +( const Point2D<T>& p ) const
{ return Point2D<T>(x_+p.x_,y_+p.y_); }


template <class T> inline
Point2D<T> Point2D<T>::operator -( const Point2D<T>& p ) const
{ return Point2D<T>(x_-p.x_,y_-p.y_); }


template <class T> inline
Point2D<T> Point2D<T>::operator *( const T factor ) const
{ return Point2D<T>(factor*x_,factor*y_); }


template <class T> inline
Point2D<T> Point2D<T>::operator /( const T den ) const
{ return Point2D<T>(x_/den,y_/den); }


template <class T> inline
bool Point2D<T>::isDefined() const
{ return !mIsUdf(x_) && !mIsUdf(y_); }


template <class T>
inline T Point2D<T>::dot(const Point2D<T>& b) const
{ return x_*b.x_ + y_*b.y_; }


template <class T> inline
void Point2D<T>::swapXY()
{
    std::swap( x_, y_ );
}

template <class T> inline
BufferString Point2D<T>::toString() const
{
    if ( isUdf() )
	return BufferString( "<undef>" );

    BufferString res( "(", x_, "," );
    res.add( y_ ).add( ')' );
    return res;
}


template <> inline
BufferString Point2D<float>::toPrettyString() const
{
    if ( !isDefined() )
	return toString();
    const Point2D<od_int64> pt( mRounded(od_int64,x_), mRounded(od_int64,y_) );
    return pt.toString();
}


template <> inline
BufferString Point2D<double>::toPrettyString() const
{
    if ( !isDefined() )
	return toString();
    const Point2D<od_int64> pt( mRounded(od_int64,x_), mRounded(od_int64,y_) );
    return pt.toString();
}


template <class T> inline
bool Point2D<T>::fromString( const char* s )
{
    if ( !s || !*s ) return false;
    if ( *s == '<' )
	{ setUdf(); return true; }

    BufferString str( s );
    char* ptrx = str.getCStr(); mSkipBlanks( ptrx );
    if ( *ptrx == '(' ) ptrx++;
    char* ptry = firstOcc( ptrx, ',' );
    if ( !ptry ) return false;
    *ptry++ = '\0';
    if ( !*ptry ) return false;
    char* ptrend = firstOcc( ptry, ')' );
    if ( ptrend ) *ptrend = '\0';

    x_ = Conv::to<T>( (const char*)ptrx );
    y_ = Conv::to<T>( (const char*)ptry );
    return isDefined();
}


template <>
inline Point2D<double> Point2D<double>::normalize() const
{
    const double sqabsval = sqAbs();
    if (sqabsval == 0)
	return *this;

    return *this / Math::Sqrt(sqabsval);
}


template <>
inline Point2D<float> Point2D<float>::normalize() const
{
    const float sqabsval = sqAbs();
    if (sqabsval == 0)
	return *this;

    return *this / Math::Sqrt(sqabsval);
}

template <class T>
inline Point2D<T> Point2D<T>::normalize() const
{
    const T sqabsval = sqAbs();
    if ( sqabsval == 0 )
	return *this;

    const float absval = Math::Sqrt((float)sqabsval);
    return Point2D<T>((T) ( ((float) x_) / absval), (T) (((float) y_) / absval));
}


template <class T> template <class FT> inline
FT Point2D<T>::abs() const
{ return ::Math::Sqrt( (FT)sqAbs() ); }


template <class T> inline
T Point2D<T>::sqAbs() const
{ return x_*x_ + y_*y_; }


template <class T> template <class FT> inline
FT Point2D<T>::distTo( const Point2D<T>& pt ) const
{ return ::Math::Sqrt( (FT)sqDistTo(pt) ); }


template <class T> inline
T Point2D<T>::sqDistTo( const Point2D<T>& pt ) const
{
    const T xdiff = x_-pt.x_;
    const T ydiff = y_-pt.y_;
    return xdiff*xdiff + ydiff*ydiff;
}


template <class T> template <class FT> inline
FT Point2D<T>::cosAngle( const Point2D<T>& from, const Point2D<T>& to ) const
{
    const T rsq = sqDistTo( from );
    const T lsq = sqDistTo( to );
    if ( !rsq || !lsq ) return 1;

    const T osq = from.sqDistTo( to );
    return (rsq +  lsq - osq) / (2 * Math::Sqrt((FT)rsq) * Math::Sqrt((FT)lsq));
}


template <class T> template <class FT> inline
FT Point2D<T>::angle( const Point2D<T>& from, const Point2D<T>& to ) const
{
    const FT cosang = cosAngle<FT>( from, to );
    if ( cosang >=  1 ) return 0;
    if ( cosang <= -1 ) return M_PI;

    const Point2D<T>& vec1 = from - *this;
    const Point2D<T>& vec2 =  to  - *this;
    const T det = vec1.x_ * vec2.y_ - vec1.y_ * vec2.x_;

    const FT ang = Math::ACos( cosang );
    return det<0 ? 2*M_PI - ang : ang;
}


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


template <class T> inline
Size2D<T>::Size2D( T w , T h )
{ width_=w; height_=h; }


template <class T> inline
bool Size2D<T>::operator ==( const Size2D<T>& s ) const
{ return s.width_ == width_ && s.height_ == height_; }


template <class T> inline
T Size2D<T>::width() const
{ return width_; }


template <class T> inline
T Size2D<T>::height() const
{ return height_; }


template <class T> inline
void Size2D<T>::setWidth( T val )
{ width_ = val; }


template <class T> inline
void Size2D<T>::setHeight( T val )
{ height_ = val; }


template <class T> inline
void Size2D<T>::set( T w, T h )
{ width_ = w; height_ = h; }


template <class T> inline
Size2D<T> Size2D<T>::operator+( T val ) const
{ return Size2D<T>( width_+val, height_+val); }


template <class T> inline
Size2D<T>& Size2D<T>::operator +=( T val )
{ width_ += val; height_ += val; return *this; }


template <class T> inline
Size2D<T>& Size2D<T>::operator -=( T val )
{ width_ -= val; height_ -= val; return *this; }


template <class T> inline
Size2D<T>& Size2D<T>::operator +=( const Size2D<T>& s )
{ width_+=s.width_; height_+=s.height_; return *this; }


template <class T> inline
Size2D<T>& Size2D<T>::operator -=( const Size2D<T>& s )
{
    width_ -= s.width_; height_ -= s.height_;
    if ( width_<0 ) width_ = -width_;
    if ( height_<0 ) height_ = -height_;
    return *this;
}


template <class T> inline
Rectangle<T>::Rectangle( T l, T t, T r, T b )
    : topleft_( Point2D<T>(l,t))
    , bottomright_( Point2D<T>(r,b) )
{}


template <class T> inline
Rectangle<T>::Rectangle( const Point2D<T>& tl, const Point2D<T>& br )
    : topleft_( tl ) , bottomright_( br )
{}


template <class T> inline
Rectangle<T>::Rectangle( const Point2D<T>& tl, const Size2D<T>& sz )
    : topleft_( tl ) , bottomright_( tl.x_+sz.width(), tl.y_+sz.height() )
{}


template <class T> inline
bool Rectangle<T>::operator ==( const Rectangle<T>& r ) const
{ return r.topleft_ == topleft_ && r.bottomright_ == bottomright_; }


template <class T> inline
Point2D<T> Rectangle<T>::topLeft() const
{ return topleft_; }


template <class T> inline
Point2D<T> Rectangle<T>::topRight() const
{ return Point2D<T>(right(),top()); }


template <class T> inline
Point2D<T> Rectangle<T>::bottomLeft() const
{ return Point2D<T>(left(),bottom()); }


template <class T> inline
Point2D<T> Rectangle<T>::bottomRight() const
{ return bottomright_; }


template <class T> inline
Point2D<T> Rectangle<T>::centre() const
{
    return Point2D<T>( (topleft_.x_+bottomright_.x_)/2,
		       (topleft_.y_+bottomright_.y_)/2 );
}


template <class T> inline
void Rectangle<T>::setTopLeft( Point2D<T> tl )
{ topleft_ = tl; }


template <class T> inline
void Rectangle<T>::setBottomRight( Point2D<T> br )
{ bottomright_ = br; }


template <class T> inline
void Rectangle<T>::setTopRight( Point2D<T> tr )
{ topleft_.y_ = tr.y_; bottomright_.x_ = tr.x_; }


template <class T> inline
void Rectangle<T>::setBottomLeft( Point2D<T> tr )
{ topleft_.x_ = tr.x_; bottomright_.y_ = tr.y_; }


template <class T> inline
void Rectangle<T>::setTopBottom( const Interval<T>& rg )
{ topleft_.y_ = rg.start; bottomright_.y_ = rg.stop; }


template <class T> inline
void Rectangle<T>::setLeftRight( const Interval<T>& rg )
{ topleft_.x_ = rg.start; bottomright_.x_ = rg.stop; }


template <class T> inline
Point2D<T> Rectangle<T>::moveInside( const Point2D<T>& pt ) const
{
    Point2D<T> res = pt;

    res.x_ = mMAX( res.x_, mMIN( left(), right() ) );
    res.x_ = mMIN( res.x_, mMAX( left(), right() ) );
    res.y_ = mMAX( res.y_, mMIN( bottom(), top() ) );
    res.y_ = mMIN( res.y_, mMAX( bottom(), top() ) );

    return res;
}


template <class T> inline
T Rectangle<T>::width() const
{ return revX() ? left()-right() : right() - left(); }


template <class T> inline
T Rectangle<T>::height() const
{ return revY() ? bottom()-top() : top()-bottom(); }


template <class T> inline
T Rectangle<T>::left() const
{ return topleft_.x_; }


template <class T> inline
T Rectangle<T>::top() const
{ return topleft_.y_; }


template <class T> inline
T Rectangle<T>::right() const
{ return bottomright_.x_; }


template <class T> inline
T Rectangle<T>::bottom() const
{ return bottomright_.y_; }


template <class T> inline
void Rectangle<T>::setLeft( T val )
{ topleft_.x_ = val; }


template <class T> inline
void Rectangle<T>::setTop( T val )
{ topleft_.y_ = val; }


template <class T> inline
void Rectangle<T>::setRight( T val )
{ bottomright_.x_ = val; }


template <class T> inline
void Rectangle<T>::setBottom( T val )
{ bottomright_.y_ = val; }


template <class T> inline
bool Rectangle<T>::checkCorners( bool leftislow, bool topislow ) const
{
    if ( leftislow == (left() > right()) ) return false;
    if ( topislow  == (top() > bottom()) ) return false;

    return true;
}


template <class T> inline
void Rectangle<T>::sortCorners( bool leftislow, bool topislow )
{
    if ( leftislow == (left() > right()) ) swapHor();
    if ( topislow  == (top() > bottom()) ) swapVer();
}


template <class T> inline
Size2D<T> Rectangle<T>::size() const
{ return Size2D<T>( width(), height() ); }


template <class T> inline
void Rectangle<T>::zero()
{ topleft_.zero(); bottomright_.zero(); }


template <class T> inline
bool Rectangle<T>::isDefined() const
{ return topleft_.isDefined() && bottomright_.isDefined(); }


template <class T> inline
Rectangle<T>& Rectangle<T>::operator +=( const Point2D<T>& p )
{ topleft_ += p; bottomright_ += p; return *this; }


template <class T> inline
Rectangle<T>& Rectangle<T>::operator -=( const Point2D<T>& p )
{ topleft_ -= p; bottomright_ -= p; return *this; }


template <class T> inline
Rectangle<T>& Rectangle<T>::operator +=( const Size2D<T>& sz )
{ bottomright_.x_ += sz.width(); bottomright_.y_ += sz.height(); return *this; }


template <class T> inline
Rectangle<T>& Rectangle<T>::operator -=( const Size2D<T>& sz )
{ bottomright_.x_ -= sz.width(); bottomright_.y_ -= sz.height(); return *this; }


template <class T> inline
void Rectangle<T>::swapHor()
{
    T t = topleft_.x_;
    topleft_.x_ = bottomright_.x_;
    bottomright_.x_ =  t;
}


template <class T> inline
void Rectangle<T>::swapVer()
{
    T t = topleft_.y_;
    topleft_.y_ = bottomright_.y_;
    bottomright_.y_ =  t;
}


template <class T> inline
bool Rectangle<T>::revX() const
{ return left() > right(); }


template <class T> inline
bool Rectangle<T>::revY() const
{ return bottom() > top(); }


template <class T>
inline bool PixRectangle<T>::xOutside( T x ) const
{
    return this->revX() ? (x > this->left() || x < this->right())
			: (x < this->left() || x > this->right());
}


template <class T>
inline bool PixRectangle<T>::yOutside( T y ) const
{
    return this->revY() ? (y > this->bottom() || y < this->top())
			: (y < this->bottom() || y > this->top());
}


template <class T>
inline bool PixRectangle<T>::isOnSide( const Point2D<T>& pt ) const
{
    return (pt.x_ == this->left() || pt.x_ == this->right())
	&& (pt.y_ == this->top()  || pt.y_ == this->bottom());
}


template <class T>
inline bool PosRectangle<T>::xOutside( T x, T eps ) const
{
    return this->revX() ? (x-this->left() > eps || this->right()-x > eps)
			: (this->left()-x > eps || x-this->right() > eps);
}


template <class T>
inline bool PosRectangle<T>::yOutside( T y, T eps ) const
{
    return this->revY() ? (this->top()-y > eps || y-this->bottom() > eps)
			: (y-this->top() > eps || this->bottom()-y > eps);
}


template <class T>
inline bool PosRectangle<T>::isOnSide( const Point2D<T>& pt, T eps ) const
{
    if ( xOutside(pt.x_) || yOutside(pt.y_) ) return false;
    return fabs(pt.x_-this->left()) < eps || fabs(pt.x_-this->right()) < eps
	|| fabs(pt.y_-this->top()) < eps || fabs(pt.y_-this->bottom()) < eps;
}


template <class T>
inline bool PosRectangle<T>::isInside( const Point2D<T>& pt, T eps ) const
{
    return (this->revX()
		? (this->left()-pt.x_>eps && pt.x_-this->right()>eps)
		: (pt.x_-this->left()>eps && this->right()-pt.x_>eps))
	&& (this->revY()
		? (pt.y_-this->bottom()<-eps && this->top()-pt.y_<-eps)
		: (this->bottom()-pt.y_<-eps && pt.y_-this->top()<-eps));
}


template <class T>
inline T iwiderPos( int x1, int x2, double f )
{ return sCast(T,mNINT32(x1 + f * (x1 - x2))); }


template <class T>
inline T fwiderPos( T x1, T x2, T f )
{ return x1 + f * (x1 - x2); }


template <class T>
PixRectangle<T>::PixRectangle( T l , T t, T r, T b )
    : Rectangle<T>(l,t,r,b)
{}


template <class T>
PixRectangle<T>::PixRectangle( const Point2D<T>& tl, const Point2D<T>& br )
    : Rectangle<T>(tl,br)
{}


template <class T>
PixRectangle<T>::PixRectangle( const Point2D<T>& tl, const Size2D<T>& sz )
    : Rectangle<T>(tl,sz)
{}


template <class T>
PixRectangle<T>::PixRectangle( const Rectangle<T>& r )
    : Rectangle<T>( r )
{}


template <class T> inline
bool PixRectangle<T>::isInside( const Point2D<T>& p ) const
{ return !xOutside( p.x_ ) && !yOutside( p.y_ ) && !isOnSide( p ); }


template <class T> inline
bool PixRectangle<T>::isOutside( const Point2D<T>& p ) const
{ return xOutside(p.x_) || yOutside(p.y_); }


template <class T> inline
bool PixRectangle<T>::contains( const Point2D<T>& p ) const
{ return !isOutside(p); }


template <class T> inline
bool PixRectangle<T>::contains( const PixRectangle<T>& other ) const
{
    return contains(other.topleft_) && contains(other.bottomright_);
}


template <class T> inline
bool PixRectangle<T>::isInside( const PixRectangle<T>& other ) const
{
    return other.isInside(this->topleft_) && other.isInside(this->bottomright_);
}


template <class T> inline
PixRectangle<T> PixRectangle<T>::grownBy( double f ) const
{
    f *= .5;
    return PixRectangle<T>( iwiderPos<T>(this->left(),this->right(),f),
			    iwiderPos<T>(this->top(),this->bottom(),f),
			    iwiderPos<T>(this->right(),this->left(),f),
			    iwiderPos<T>(this->bottom(),this->top(),f) );
}


template <class T>
inline PosRectangle<T> PosRectangle<T>::grownBy( T f ) const
{
    f *= .5;
    return PosRectangle<T>( fwiderPos(this->left(),this->right(),f),
			    fwiderPos(this->top(),this->bottom(),f),
			    fwiderPos(this->right(),this->left(),f),
			    fwiderPos(this->bottom(),this->top(),f) );
}


template <class T>
inline bool Rectangle<T>::operator >( const Rectangle<T>& r ) const
{
    Size2D<T> diff( width()-r.width(), height()-r.height() );

    if ( diff.width() > 0 && diff.height() > 0 )
	return true;
    if ( diff.width() < 0 && diff.height() < 0 )
	return false;

    return diff.width() < 0 ? diff.height() < -diff.width()
			    : diff.width() > -diff.height();
}


template <class T>
inline void Rectangle<T>::limitTo( const Rectangle<T>& r )
{
    topleft_ = r.moveInside( topleft_ );
    bottomright_ = r.moveInside( bottomright_ );
}


template <class T>
inline void Rectangle<T>::translate(const Point2D<T> & trans )
{
    topleft_ += trans;
    bottomright_ += trans;
}


template <class T>
inline void Rectangle<T>::include( const Rectangle<T>& r )
{
    if ( !r.isDefined() ) return;
    if ( !isDefined() ) *this = r;

    if ( revX() )
    {
	if ( r.left() > left() ) topleft_.x_ = r.left();
	if ( r.right() < right() ) bottomright_.x_ = r.right();
    }
    else
    {
	if ( r.left() < left() ) topleft_.x_ = r.left();
	if ( r.right() > right() ) bottomright_.x_ = r.right();
    }
    if ( revY() )
    {
	if ( r.bottom() > bottom() ) bottomright_.y_ = r.bottom();
	if ( r.top() < top() ) topleft_.y_ = r.top();
    }
    else
    {
	if ( r.bottom() < bottom() ) bottomright_.y_ = r.bottom();
	if ( r.top() > top() ) topleft_.y_ = r.top();
    }
}


template <class T>
inline void Rectangle<T>::include( const Point2D<T>& p )
{
    if ( !p.isDefined() ) return;
    if ( !isDefined() ) topleft_ = bottomright_ = p;

    if ( revX() )
    {
	if ( p.x_ > left() ) topleft_.x_ = p.x_;
	if ( p.x_ < right() ) bottomright_.x_ = p.x_;
    }
    else
    {
	if ( p.x_ < left() ) topleft_.x_ = p.x_;
	if ( p.x_ > right() ) bottomright_.x_ = p.x_;
    }
    if ( revY() )
    {
	if ( p.y_ > bottom() ) bottomright_.y_ = p.y_;
	if ( p.y_ < top() ) topleft_.y_ = p.y_;
    }
    else
    {
	if ( p.y_ < bottom() ) bottomright_.y_ = p.y_;
	if ( p.y_ > top() ) topleft_.y_ = p.y_;
    }
}

}; // namespace Geom


namespace Conv
{

    template <>
    inline void set( Geom::Point2D<float>& _to, const Geom::Point2D<double>& f )
    { _to.setFrom( f ); }

    template <>
    inline void set( Geom::Point2D<double>& _to, const Geom::Point2D<float>& f )
    { _to.setFrom( f ); }

    template <>
    inline void set( Geom::Point3D<float>& _to, const Geom::Point3D<double>& f )
    { _to.setFrom( f ); }

    template <>
    inline void set( Geom::Point3D<double>& _to, const Geom::Point3D<float>& f )
    { _to.setFrom( f ); }
}
