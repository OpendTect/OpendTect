#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "gendefs.h"
#include "ranges.h"
#include "math2.h"

namespace Geom
{

/*!
\brief %Basic point class.
*/

template <class T>
mClass(Basic) Point2D
{
public:
				Point2D(T xx=0,T yy=0);
				Point2D(const Point2D<T>&);
    virtual			~Point2D();

    template <class TT>
    Point2D<T>&			setFrom(const Point2D<TT>&);

    template <class TT>
    inline void			setXY(TT xx,TT yy);
    inline void			setXY(T xx,T yy);
    inline Point2D<T>&		zero();
    inline Point2D<T>		operator-();

    inline T&			operator[](int idx);
    inline T			operator[](int idx) const;

    inline bool			operator==(const Point2D<T>&) const;
    inline bool			operator!=(const Point2D<T>&) const;
    inline Point2D<T>&		operator+=(T dist);
    inline Point2D<T>&		operator*=(T factor);
    inline Point2D<T>&		operator/=(T den);
    inline Point2D<T>&		operator+=(const Point2D<T>&);
    inline Point2D<T>&		operator-=(const Point2D<T>&);
    inline Point2D<T>		operator+(const Point2D<T>&) const;
    inline Point2D<T>		operator-(const Point2D<T>&) const;
    inline Point2D<T>		operator*(const T factor) const;
    inline Point2D<T>		operator/(const T den) const;
    inline Point2D<T>&		operator=(const Point2D<T>&);

    inline void			swapXY();

    inline bool			isDefined() const;
    inline double		abs() const;
    inline T			sqAbs() const;
    inline double		distTo(const Point2D<T>&) const;
    inline T			sqDistTo(const Point2D<T>&) const;
    template <class TT>
    inline Point2D<T>		scale(TT xx,TT yy) const;

    static Point2D<T>		udf() { return Point2D<T>(mUdf(T),mUdf(T)); }

    T				x_;
    T				y_;

    mDeprecated("Use x_")
    T&				x;
    mDeprecated("Use y_")
    T&				y;
};

typedef Point2D<int> PointI;
typedef Point2D<float> PointF;
typedef Point2D<double> PointD;

/*!
\brief %Basic 2D sizes (width/height) class.
*/

template <class T>
mClass(Basic) Size2D
{
public:
			Size2D(T w=0 ,T h=0);

    inline bool		operator==(const Size2D<T>&) const;
    inline bool		operator!=(const Size2D<T>&) const;

    inline T		width() const;
    inline T		height() const;
    inline void		setWidth(T val);
    inline void		setHeight(T val);
    inline Size2D<T>	operator+(T val) const;
    inline Size2D<T>&	operator+=(T val);
    inline Size2D<T>&	operator-=(T val);
    inline Size2D<T>&	operator+=(const Size2D<T>&);
    inline Size2D<T>&	operator-=(const Size2D<T>&);

    inline bool		isUdf() const
			{ return mIsUdf(width_) || mIsUdf(height_); }
    inline void		setUdf()
			{ width_ = height_ = mUdf(T); }
    static Size2D	udf()
			{ return Size2D(mUdf(T),mUdf(T)); }


protected:

    T			width_;
    T			height_;

};


/*!
\brief %Basic 2D rectangle class.

  This class is a bit more complicated than would be expected at first sight.
  This is caused by the problem of coordinate system sign. For example, in
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

    inline bool		operator==(const Rectangle<T>&) const;
    inline bool		operator!=(const Rectangle<T>&) const;

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
    inline bool		intersects(const Rectangle<T>&) const;

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


/*!
\brief Integer rectangle class.

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


/*!
\brief Floating-point rectangle class.

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

    template <class TT>
    inline PosRectangle<T> scale(TT xx,TT yy) const;

protected:

    inline bool		xOutside(T,T) const;
    inline bool		yOutside(T,T) const;
};


typedef PosRectangle<int> RectI;
typedef PosRectangle<float> RectF;
typedef PosRectangle<double> RectD;


mStartAllowDeprecatedSection

template <class T> inline
Point2D<T>::Point2D( T xx, T yy )
    : x_(xx)
    , y_(yy)
    , x(x_)
    , y(y_)
{}


template <class T> inline
Point2D<T>::Point2D( const Point2D<T>& pt )
    : x_(pt.x_)
    , y_(pt.y_)
    , x(x_)
    , y(y_)
{}

mStopAllowDeprecatedSection


template <class T> inline
Point2D<T>::~Point2D()
{}


template <class T> inline
Point2D<T>& Point2D<T>::operator=( const Point2D<T>& pt )
{
     x_ = pt.x_;
     y_ = pt.y_;
     return *this;
}


template <class T> template <class TT> inline
Point2D<T>& Point2D<T>::setFrom( const Point2D<TT>& a )
{ x_=a.x_; y_=a.y_; return *this;}

template <class T> inline
void Point2D<T>::setXY( T xx, T yy )
{ x_ = xx ; y_ = yy; }

template <class T> template <class TT> inline
void Point2D<T>::setXY( TT xx, TT yy )
{ x_ = sCast(T,xx); y_ = sCast(T,yy); }

template <class T> inline
Point2D<T>& Point2D<T>::zero()
{ x_ = y_ = 0; return *this; }

template <class T> inline
Point2D<T> Point2D<T>::operator -()
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
bool Point2D<T>::operator !=( const Point2D<T>& p ) const
{ return !(*this==p); }

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


template <class T> inline
void Point2D<T>::swapXY()
{
    Swap( x_, y_ );
}


template <class T> inline
double Point2D<T>::abs() const
{ return ::Math::Sqrt( sCast(double,sqAbs()) ); }


template <class T> inline
T Point2D<T>::sqAbs() const
{ return x_*x_ + y_*y_; }


template <class T> inline
double Point2D<T>::distTo( const Point2D<T>& pt ) const
{ return ::Math::Sqrt( sCast(double,sqDistTo(pt)) ); }


template <class T> inline
T Point2D<T>::sqDistTo( const Point2D<T>& pt ) const
{
    const T xdiff = x_-pt.x_;
    const T ydiff = y_-pt.y_;
    return xdiff*xdiff + ydiff*ydiff;
}


template <class T> template <class TT> inline
Point2D<T> Point2D<T>::scale( TT xx, TT yy ) const
{ return Point2D<T>( x_*sCast(T,xx), y_*sCast(T,yy) ); }


// Size2D
template <class T> inline
Size2D<T>::Size2D( T w , T h )
{ width_=w; height_=h; }


template <class T> inline
bool Size2D<T>::operator ==( const Size2D<T>& s ) const
{ return s.width_ == width_ && s.height_ == height_; }


template <class T> inline
bool Size2D<T>::operator !=( const Size2D<T>& s ) const
{ return s.width_ != width_ || s.height_ != height_; }


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


// Rectangle
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
bool Rectangle<T>::operator !=( const Rectangle<T>& r ) const
{ return r.topleft_ != topleft_ || r.bottomright_ != bottomright_; }


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
    { topleft_.y_ = rg.start_; bottomright_.y_ = rg.stop_; }


template <class T> inline
void Rectangle<T>::setLeftRight( const Interval<T>& rg )
    { topleft_.x_ = rg.start_; bottomright_.x_ = rg.stop_; }


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


// PixRectangle
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


// PosRectangle
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
    return fabs(pt.x_ - this->left()) < eps ||
	   fabs(pt.x_ - this->right()) < eps ||
	   fabs(pt.y_ - this->top()) < eps ||
	   fabs(pt.y_ - this->bottom()) < eps;
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


template <class T> template <class TT> inline
PosRectangle<T> PosRectangle<T>::scale( TT xx, TT yy ) const
{
    return PosRectangle<T>( this->topleft_.scale(xx,yy),
			    this->bottomright_.scale(xx,yy) );
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
    if ( !p.isDefined() )
	return;

    if ( !isDefined() )
    {
	topleft_ = bottomright_ = p;
	return;
    }

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

template <class T>
bool Rectangle<T>::intersects( const Rectangle<T>& oth ) const
{
    if ( left() > oth.right() || right() < oth.left() )
	return false;

    if ( top() < oth.bottom() || bottom() > oth.top() )
	return false;

    return true;
}


} // namespace Geom
