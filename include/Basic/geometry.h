#ifndef geometry_h
#define geometry_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          01/02/2000
 RCS:           $Id: geometry.h,v 1.34 2008-07-16 18:05:17 cvsnanne Exp $
________________________________________________________________________

-*/

#include "gendefs.h"
#include "ranges.h"
#include "math2.h"

namespace Geom
{

/*!\brief basic point class */
template <class T>
class Point2D
{
public:
				Point2D ( T xx = 0, T yy = 0 );
    virtual			~Point2D()				{}

    template <class TT>
    Point2D<T>&			setFrom(const Point2D<TT>& a);

    inline void			setXY( T xx, T yy );
    inline Point2D<T>&		zero();
    inline Point2D<T>		operator -();

    virtual inline T&		operator[]( int idx );
    virtual inline T		operator[]( int idx ) const;

    virtual inline bool		operator ==( const Point2D<T>& p ) const;
    virtual inline bool		operator !=( const Point2D<T>& p ) const;
    virtual inline Point2D<T>&	operator+=( T dist );
    virtual inline Point2D<T>&	operator*=( T factor );
    virtual inline Point2D<T>&	operator/=( T den );
    inline Point2D<T>&		operator +=( const Point2D<T>& p );
    inline Point2D<T>&		operator -=( const Point2D<T>& p );
    inline Point2D<T>		operator +( const Point2D<T>& p ) const;
    inline Point2D<T>		operator -( const Point2D<T>& p ) const;
    inline Point2D<T>		operator *( const T factor ) const;
    inline Point2D<T>		operator /( const T den ) const;

    virtual inline bool		isDefined() const;
    virtual inline double	abs() const;
    virtual inline T		sqAbs() const;
    virtual inline double	distTo(const Point2D<T>&) const;
    virtual inline T		sqDistTo(const Point2D<T>&) const;

    static Point2D<T>		udf() { return Point2D<T>( mUdf(T), mUdf(T) ); }
    
    T 				x;
    T 				y;
};


/*!\brief basic 2D sizes (width/height) class */

template <class T>
class Size2D
{
public:
			Size2D( T w = 0 , T h = 0 );

    inline bool		operator ==( const Size2D<T>& s ) const;
    inline bool		operator !=( const Size2D<T>& s ) const;

    inline T		width() const;
    inline T		height() const;
    inline void		setWidth( T val );
    inline void		setHeight( T val );
    inline Size2D<T>	operator +( T val ) const;
    inline Size2D<T>&	operator +=( T val );
    inline Size2D<T>&	operator -=( T val );
    inline Size2D<T>&	operator +=( const Size2D<T>& s );
    inline Size2D<T>&	operator -=( const Size2D<T>& s );

protected:

    T 	width_;
    T 	height_;

};


/*!\brief basic 2D rectangle class

This class is a bit more complicated than would be expected at first sight.
This is caused by the problem of coordinate system sign. For example, in
user interfaces, top is a lower number than bottom. But for normal
coordinates, this is (of course) not the case. Still, also for floating point
types, reverse axes are common.

*/

template <class T>
class Rectangle
{
public:
			Rectangle( T l = 0 , T t = 0, T r = 0 , T b = 0 ) ;
			Rectangle( const Point2D<T>& tl, const Point2D<T>& br);

    inline bool		operator ==( const Rectangle<T>& r ) const;
    inline bool		operator !=( const Rectangle<T>& r ) const;

    inline Point2D<T>	topLeft() const;
    inline Point2D<T>	topRight() const;
    inline Point2D<T>	bottomLeft() const;
    inline Point2D<T>	bottomRight() const;
    inline Point2D<T>	centre() const;
    inline void		setTopLeft( Point2D<T> tl );
    inline void		setBottomRight( Point2D<T> br );
    inline void		setTopRight( Point2D<T> tr );
    inline void		setBottomLeft( Point2D<T> tr );
    inline void		setTopBottom(const Interval<T>&);
    inline void		setLeftRight(const Interval<T>&);

    inline Point2D<T>	moveInside(const Point2D<T>&) const;

    inline void		fitIn(const Rectangle<T>&);

    inline bool		operator >(const Rectangle<T>&) const;

    inline T 		width() const;
    inline T 		height() const;

    inline T 		left() const;
    inline T 		top() const;
    inline T 		right() const;
    inline T 		bottom() const;
    inline void 	setLeft( T val );
    inline void 	setTop( T val );
    inline void 	setRight( T val );
    inline void 	setBottom( T val );

    void		checkCorners( bool leftislow=true, bool topislow=true );
    inline Size2D<T>	size() const;
    inline void 	zero();

    inline Rectangle<T>&	operator +=( const Point2D<T>& p );
    inline Rectangle<T>&	operator -=( const Point2D<T>& p );

    inline void		swapHor();
    inline void		swapVer();

protected:

    inline bool		revX() const;
    inline bool		revY() const;

    Point2D<T>		topLeft_;
    Point2D<T>		bottomRight_;

};


/*!\brief Integer rectangle class.

  The difference with the floating point type rectangle is in range handling.
  In the float world, everything must be epsiloned. Integer rectangles are
  more straightforward.

*/


template <class T>
class PixRectangle : public Rectangle<T>
{
public:
			PixRectangle( T l = 0 , T t = 0, T r = 0 , T b = 0 ) ;
			PixRectangle( Point2D<T> tl, Point2D<T> br ) ;
			PixRectangle( const Rectangle<T>& );

    inline bool		isInside( const Point2D<T>& p ) const;
    inline bool		isOutside( const Point2D<T>& p ) const;
    inline bool		isOnSide(const Point2D<T>&) const;
    inline bool		contains( const Point2D<T>& p ) const;
    inline bool		contains( const PixRectangle<T>& other ) const;
    inline bool		isInside( const PixRectangle<T>& other ) const;

    inline PixRectangle<T> grownBy(double sidesincreasebyfactor=1) const;

protected:

    inline bool		xOutside(T) const;
    inline bool		yOutside(T) const;
};


/*!\brief Floating-point rectangle class.

  The difference with the integer type rectangle is in range handling. In the
  float world, everything must be epsiloned.
  with inside and outside.

*/

template <class T>
class PosRectangle : public Rectangle<T>
{
public:
			PosRectangle( T l = 0 , T t = 0, T r = 0 , T b = 0 ) 
			: Rectangle<T>(l,t,r,b)		{}
			PosRectangle( Point2D<T> tl, Point2D<T> br ) 
			: Rectangle<T>(tl,br)		{}

    inline bool		isOutside( const Point2D<T>& p, T eps ) const
			{ return xOutside(p.x,eps) || yOutside(p.y,eps); }
    inline bool		isInside(const Point2D<T>&,T eps) const;
    inline bool		isOnSide(const Point2D<T>& p,T eps) const;

    inline bool		contains( const Point2D<T>& p, T eps ) const
			{ return !isOutside(p,eps); }

    inline bool		contains( const PosRectangle<T>& other, T eps ) const
			{
			    return contains(other.topLeft_,eps)
				&& contains(other.bottomRight_,eps);
			}
    inline bool		isInside( const PosRectangle<T>& other, T eps ) const
			{
			    return other.isInside(this->topLeft_,eps)
				&& other.isInside(this->bottomRight_,eps);
			}

    inline PosRectangle<T> grownBy(T sidesincreasebyfactor=1) const;

protected:

    inline bool		xOutside(T,T) const;
    inline bool		yOutside(T,T) const;
};


template <class T> inline
Point2D<T>::Point2D ( T xx , T yy )
    : x(xx), y(yy)
{}

template <class T> template <class TT> inline
Point2D<T>& Point2D<T>::setFrom( const Point2D<TT>& a )
{ x=a.x; y=a.y; return *this;}
    				
template <class T> inline
void Point2D<T>::setXY( T xx, T yy )
{ x = xx ; y = yy; }  


template <class T> inline
Point2D<T>& Point2D<T>::zero()
{ x = y = 0; return *this; }


template <class T> inline
Point2D<T> Point2D<T>::operator -()
{ return Point2D<T>( -x, -y ); }


template <class T> inline
T& Point2D<T>::operator[]( int idx )
{ return idx ? y : x; }


template <class T> inline
T Point2D<T>::operator[]( int idx ) const
{ return idx ? y : x; }


template <class T> inline
bool Point2D<T>::operator ==( const Point2D<T>& p ) const
{ return p.x == x && p.y == y; }


template <class T> inline
bool Point2D<T>::operator !=( const Point2D<T>& p ) const
{ return !(*this==p); }

template <class T> inline
Point2D<T>& Point2D<T>::operator+=( T dist )
{ x += dist; y += dist; return *this; }


template <class T> inline
Point2D<T>& Point2D<T>::operator*=( T factor )
{ x *= factor; y *= factor; return *this; }


template <class T> inline
Point2D<T>& Point2D<T>::operator/=( T den )
{ x /= den; y /= den; return *this; }


template <class T> inline
Point2D<T>& Point2D<T>::operator +=( const Point2D<T>& p )
{ x += p.x; y += p.y; return *this; }


template <class T> inline
Point2D<T>& Point2D<T>::operator -=( const Point2D<T>& p )
{ x -= p.x; y -= p.y; return *this; }


template <class T> inline
Point2D<T> Point2D<T>::operator +( const Point2D<T>& p ) const
{ return Point2D<T>(x+p.x,y+p.y); }


template <class T> inline
Point2D<T> Point2D<T>::operator -( const Point2D<T>& p ) const
{ return Point2D<T>(x-p.x,y-p.y); }


template <class T> inline
Point2D<T> Point2D<T>::operator *( const T factor ) const
{ return Point2D<T>(factor*x,factor*y); }


template <class T> inline
Point2D<T> Point2D<T>::operator /( const T den ) const
{ return Point2D<T>(x/den,y/den); }


template <class T> inline
bool Point2D<T>::isDefined() const
{ return !mIsUdf(x) && !mIsUdf(y); }


template <class T> inline
double Point2D<T>::abs() const
{ return Math::Sqrt( (double)sqAbs() ); }


template <class T> inline
T Point2D<T>::sqAbs() const
{ return x*x + y*y; }


template <class T> inline
double Point2D<T>::distTo( const Point2D<T>& pt ) const
{ return Math::Sqrt( (double)sqDistTo(pt) ); }


template <class T> inline
T Point2D<T>::sqDistTo( const Point2D<T>& pt ) const
{
    const T xdiff = x-pt.x;
    const T ydiff = y-pt.y;
    return xdiff*xdiff + ydiff*ydiff;
}


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


template <class T> inline
Rectangle<T>::Rectangle( T l, T t, T r, T b ) 
    : topLeft_( Point2D<T>(l,t)) 
    , bottomRight_( Point2D<T>(r,b) )
{}


template <class T> inline
Rectangle<T>::Rectangle( const Point2D<T>& tl, const Point2D<T>& br ) 
    : topLeft_( tl ) , bottomRight_( br )
{} 


template <class T> inline
bool Rectangle<T>::operator ==( const Rectangle<T>& r ) const
{ return r.topLeft_ == topLeft_ && r.bottomRight_ == bottomRight_; }


template <class T> inline
bool Rectangle<T>::operator !=( const Rectangle<T>& r ) const
{ return r.topLeft_ != topLeft_ || r.bottomRight_ != bottomRight_; }


template <class T> inline
Point2D<T> Rectangle<T>::topLeft() const
{ return topLeft_; }


template <class T> inline
Point2D<T> Rectangle<T>::topRight() const
{ return Point2D<T>(right(),top()); }


template <class T> inline
Point2D<T> Rectangle<T>::bottomLeft() const
{ return Point2D<T>(left(),bottom()); }


template <class T> inline
Point2D<T> Rectangle<T>::bottomRight() const
{ return bottomRight_; }


template <class T> inline
Point2D<T> Rectangle<T>::centre() const 		
{
    return Point2D<T>( (topLeft_.x+bottomRight_.x)/2,
		       (topLeft_.y+bottomRight_.y)/2 ); 
}


template <class T> inline
void Rectangle<T>::setTopLeft( Point2D<T> tl )
{ topLeft_ = tl; }


template <class T> inline
void Rectangle<T>::setBottomRight( Point2D<T> br )
{ bottomRight_ = br; }


template <class T> inline
void Rectangle<T>::setTopRight( Point2D<T> tr )
{ topLeft_.y = tr.y; bottomRight_.setX(tr.x); }


template <class T> inline
void Rectangle<T>::setBottomLeft( Point2D<T> tr )
{ topLeft_.setX(tr.x); bottomRight_.y = tr.y; }


template <class T> inline
void Rectangle<T>::setTopBottom( const Interval<T>& rg )
{ topLeft_.y = rg.start; bottomRight_.y = rg.stop; }


template <class T> inline
void Rectangle<T>::setLeftRight( const Interval<T>& rg )
{ topLeft_.x = rg.start; bottomRight_.x = rg.stop; }


template <class T> inline
Point2D<T> Rectangle<T>::moveInside( const Point2D<T>& pt ) const
{
    Point2D<T> res = pt;

    res.x = mMAX( res.x, mMIN( left(), right() ) );
    res.x = mMIN( res.x, mMAX( left(), right() ) );
    res.y = mMAX( res.y, mMIN( bottom(), top() ) );
    res.y = mMIN( res.y, mMAX( bottom(), top() ) );

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
{ return topLeft_.x; }


template <class T> inline
T Rectangle<T>::top() const
{ return topLeft_.y; }


template <class T> inline
T Rectangle<T>::right() const
{ return bottomRight_.x; }


template <class T> inline
T Rectangle<T>::bottom() const
{ return bottomRight_.y; }


template <class T> inline
void Rectangle<T>::setLeft( T val )
{ topLeft_.x = val; }


template <class T> inline
void Rectangle<T>::setTop( T val )
{ topLeft_.y = val; }


template <class T> inline
void Rectangle<T>::setRight( T val )
{ bottomRight_.x = val; }


template <class T> inline
void Rectangle<T>::setBottom( T val )
{ bottomRight_.y = val; }


template <class T> inline
void Rectangle<T>::checkCorners( bool leftislow, bool topislow )
{ 
    if ( leftislow == (left() > right()) ) swapHor(); 
    if ( topislow  == (top() > bottom()) ) swapVer(); 
}


template <class T> inline
Size2D<T> Rectangle<T>::size() const
{ return Size2D<T>( width(), height() ); }


template <class T> inline
void Rectangle<T>::zero()
{ topLeft_.zero(); bottomRight_.zero(); }


template <class T> inline
Rectangle<T>& Rectangle<T>::operator +=( const Point2D<T>& p )
{ topLeft_ += p; bottomRight_ += p; return *this; }


template <class T> inline
Rectangle<T>& Rectangle<T>::operator -=( const Point2D<T>& p )
{ topLeft_ -= p; bottomRight_ -= p; return *this; }


template <class T> inline
void Rectangle<T>::swapHor() 
{ 
    T t = topLeft_.x; 
    topLeft_.x = bottomRight_.x;
    bottomRight_.x =  t;
}


template <class T> inline
void Rectangle<T>::swapVer() 
{ 
    T t = topLeft_.y; 
    topLeft_.y = bottomRight_.y;
    bottomRight_.y =  t;
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
    return (pt.x == this->left() || pt.x == this->right())
	&& (pt.y == this->top()  || pt.y == this->bottom());
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
    if ( xOutside(pt.x) || yOutside(pt.y) ) return false;
    return fabs(pt.x - this->left()) < eps || fabs(pt.x - this->right()) < eps
        || fabs(pt.y - this->top()) < eps || fabs(pt.y - this->bottom()) < eps;
}


template <class T>
inline bool PosRectangle<T>::isInside( const Point2D<T>& pt, T eps ) const
{
    return (this->revX() ? (this->left()-pt.x>eps && pt.x-this->right()>eps)
			 : (pt.x-this->left()>eps && this->right()-pt.x>eps))
	&& (this->revY() ? (pt.y-this->bottom()<-eps && this->top()-pt.y<-eps)
			 : (this->bottom()-pt.y<-eps && pt.y-this->top()<-eps));
}


template <class T>
inline T iwiderPos( int x1, int x2, double f )
{ return (T)mNINT(x1 + f * (x1 - x2)); }


template <class T>
inline T fwiderPos( T x1, T x2, T f )
{ return x1 + f * (x1 - x2); }


template <class T>
PixRectangle<T>::PixRectangle( T l , T t, T r, T b ) 
    : Rectangle<T>(l,t,r,b)		
{}


template <class T>
PixRectangle<T>::PixRectangle( Point2D<T> tl, Point2D<T> br ) 
    : Rectangle<T>(tl,br)
{}


template <class T>
PixRectangle<T>::PixRectangle( const Rectangle<T>& r ) 
    : Rectangle<T>( r )
{}


template <class T> inline
bool PixRectangle<T>::isInside( const Point2D<T>& p ) const
{ return !xOutside( p.x ) && !yOutside( p.y ) && !isOnSide( p ); }


template <class T> inline
bool PixRectangle<T>::isOutside( const Point2D<T>& p ) const
{ return xOutside(p.x) || yOutside(p.y); }


template <class T> inline
bool PixRectangle<T>::contains( const Point2D<T>& p ) const
{ return !isOutside(p); }


template <class T> inline
bool PixRectangle<T>::contains( const PixRectangle<T>& other ) const
{
    return contains(other.topLeft_) && contains(other.bottomRight_);
}


template <class T> inline
bool PixRectangle<T>::isInside( const PixRectangle<T>& other ) const
{
    return other.isInside(this->topLeft_) && other.isInside(this->bottomRight_);
}


template <class T> inline
PixRectangle<T> PixRectangle<T>::grownBy( double f ) const
{
    f *= .5;
    return PixRectangle<T>( iwiderPos(this->left(),this->right(),f),
			    iwiderPos(this->top(),this->bottom(),f),
			    iwiderPos(this->right(),this->left(),f),
			    iwiderPos(this->bottom(),this->top(),f) );
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
inline void Rectangle<T>::fitIn( const Rectangle<T>& r )
{
    if ( revX() )
    {
	if ( r.left() < left() ) topLeft_.x = r.left();
	if ( r.right() > right() ) bottomRight_.x = r.right();
    }
    else
    {
	if ( r.left() > left() ) topLeft_.x = r.left();
	if ( r.right() < right() ) bottomRight_.x = r.right();
    }
    if ( revY() )
    {
	if ( r.bottom() < bottom() ) bottomRight_.y = r.bottom();
	if ( r.top() > top() ) topLeft_.y = r.top();
    }
    else
    {
	if ( r.bottom() > bottom() ) bottomRight_.y = r.bottom();
	if ( r.top() < top() ) topLeft_.y = r.top();
    }
}

}; // namespace Geom

#endif
