#ifndef geometry_h
#define geometry_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          01/02/2000
 RCS:           $Id: geometry.h,v 1.19 2006-09-07 15:44:23 cvskris Exp $
________________________________________________________________________

-*/

#include <gendefs.h>

namespace Geom
{

/*!\brief basic point class */
template <class T>
class Point2D
{
public:
				Point2D ( T xx = 0, T yy = 0 );
    virtual			~Point2D()				{}

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
    
    T 				x;
    T 				y;
};


/*!\brief basic 2D sizes (width/height) class */

template <class T>
class Size2D
{
public:
			Size2D( T w = 0 , T h = 0 ) 
				{ width_ = w; height_ = h; }

    inline bool		operator ==( const Size2D<T>& s ) const
			{ return s.width_ == width_ && s.height_ == height_; }
    inline bool		operator !=( const Size2D<T>& s ) const
			{ return s.width_ != width_ || s.height_ != height_; }

    inline T		width() const		{ return width_; }
    inline T		height() const		{ return height_; }
    inline void		setWidth( T val )	{ width_ = val; }
    inline void		setHeight( T val )	{ height_ = val; }
    inline Size2D<T>&	operator +=( T val )
			{ width_ += val; height_ += val; return *this; }
    inline Size2D<T>&	operator -=( T val )
			{ width_ -= val; height_ -= val; return *this; }
    inline Size2D<T>&	operator +=( const Size2D<T>& s )
			{ width_+=s.width_; height_+=s.height_; return *this; }
    inline Size2D<T>&	operator -=( const Size2D<T>& s )
			{
			    width_ -= s.width_; height_ -= s.height_;
			    if ( width_ < 0 ) width_ = -width_;
			    if ( height_ < 0 ) height_ = -height_;
			    return *this;
			}

protected:

    T 	width_;
    T 	height_;

};


/*!\brief basic 2D rectangle class

This class is a bit more complicated than would be expected at first sight.
This is caused by the problem of coordinate system sign. For example, in
user interfaces, top is a lower number than bottom. But foor normal
coordinates, this is (of course) not the case.
*/

template <class T>
class Rect 
{
public:
			Rect ( T l = 0 , T t = 0, T r = 0 , T b = 0 ) 
			: topLeft_( Point2D<T>(l,t)) 
			, bottomRight_( Point2D<T>(r,b) ) {}
			Rect ( Point2D<T> tl, Point2D<T> br ) 
			: topLeft_( tl ) , bottomRight_( br ) {} 

    inline bool		operator ==( const Rect<T>& r ) const
			{ return   r.topLeft_ == topLeft_
				&& r.bottomRight_ == bottomRight_; }
    inline bool		operator !=( const Rect<T>& r ) const
			{ return   r.topLeft_ != topLeft_
				|| r.bottomRight_ != bottomRight_; }

    inline Point2D<T>	topLeft() const    { return topLeft_; }
    inline Point2D<T>	topRight() const   { return Point2D<T>(top(),right());}
    inline Point2D<T>	bottomLeft() const {return Point2D<T>(bottom(),left());}
    inline Point2D<T>	bottomRight() const { return bottomRight_; }
    inline Point2D<T>	centre() const 		
                        { return Point2D<T>( (topLeft_.x+bottomRight_.x)/2,
					   (topLeft_.y+bottomRight_.y)/2 ); 
                        }
    inline void		setTopLeft( Point2D<T> tl )	{ topLeft_ = tl; }
    inline void		setBottomRight( Point2D<T> br )	{ bottomRight_ = br; }
    inline void		setTopRight( Point2D<T> tr )
			{ topLeft_.y = tr.y; bottomRight_.setX(tr.x); }
    inline void		setBottomLeft( Point2D<T> tr )
			{ topLeft_.setX(tr.x); bottomRight_.y = tr.y; }

    inline bool		isInside(const Point2D<T>&) const;
    inline bool		isOutside( const Point2D<T>& p ) const
			{ return xOutside(p.x) || yOutside(p.y); }
    inline bool		isOnSide( const Point2D<T>& p ) const
			{ return !isInside(p) && !isOutside(p); }
    inline bool		contains( const Point2D<T>& p ) const
			{ return !isOutside(p); }

    inline bool		contains( const Rect<T>& other ) const
			{
			    return contains(other.topLeft())
				&& contains(other.bottomRight());
			}
    inline bool		isInside( const Rect<T>& other ) const
			{
			    return other.isInside(topLeft())
				&& other.isInside(bottomRight());
			}
    inline void		fitIn(const Rect<T>&);

    inline bool		operator >(const Rect<T>&) const;

    inline T 		width() const
			{ return revX() ? left()-right() : right() - left(); }
    inline T 		height() const
			{ return revY() ? bottom()-top() : top()-bottom(); }
    inline Rect<T>	grownBy(double sidesincreasebyfactor=1) const;

    inline T 		left() const 		{ return topLeft_.x; }
    inline T 		top() const 		{ return topLeft_.y; }
    inline T 		right() const 		{ return bottomRight_.x; }
    inline T 		bottom() const		{ return bottomRight_.y; }
    inline void 	setLeft( T val )	{ topLeft_.x = val; }
    inline void 	setTop( T val )		{ topLeft_.y = val; }
    inline void 	setRight( T val )	{ bottomRight_.x = val; }
    inline void 	setBottom( T val )	{ bottomRight_.y = val; }

    void		checkCorners( bool leftislow=true, bool topislow=true )
			{ 
			    if( leftislow == left() > right() )  swapHor(); 
			    if( topislow  == top()  > bottom() ) swapVer(); 
			}

    inline Size2D<T>	size() const { return Size2D<T>( width(), height() ); }
    inline		operator Size2D<T>() const	{ return size(); }
    inline void 	zero()	{ topLeft_.zero(); bottomRight_.zero(); }

    inline Rect<T>&	operator +=( const Point2D<T>& p )
			{ topLeft_ += p; bottomRight_ += p; return *this; }
    inline Rect<T>&	operator -=( const Point2D<T>& p )
			{ topLeft_ -= p; bottomRight_ -= p; return *this; }

protected:

    inline void		swapHor() 
			{ 
			    T t = topLeft_.x; 
			    topLeft_.setX( bottomRight_.x );
			    bottomRight_.setX( t );
			}
    inline void		swapVer() 
			{ 
			    T t = topLeft_.y; 
			    topLeft_.y = bottomRight_.y;
			    bottomRight_.y =  t;
			}

    Point2D<T>		topLeft_;
    Point2D<T>		bottomRight_;

    inline bool		revX() const		{ return left() > right(); }
    inline bool		revY() const		{ return bottom() > top(); }

    inline bool		xOutside(T) const;
    inline bool		yOutside(T) const;
};


template <class T> inline
Point2D<T>::Point2D ( T xx , T yy )
    : x(xx), y(yy)
{}


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


template <class T>
inline bool Rect<T>::isInside( const Point2D<T>& pt ) const
{
    return pt != topLeft_ && pt != bottomRight_
	&& ( (pt.x - left() > 0) == (right() - pt.x > 0) )
	&& ( (pt.y - bottom() > 0) == (top() - pt.y > 0) );
}


template <class T>
inline bool Rect<T>::xOutside( T x ) const
{
    return x != left() && x != right() && (x-left() > 0 == x-right() > 0);
}


template <class T>
inline bool Rect<T>::yOutside( T y ) const
{
    return y != bottom() && y != top() && (y-bottom() > 0 == y-top() > 0);
}


template <class T>
inline T widerPos( T x1, T x2, double f )
{ return (T)(x1 + f * (x1 - x2)); }

inline int widerPos( int x1, int x2, double f )
{ return mNINT(x1 + f * (x1 - x2)); }


template <class T>
inline Rect<T> Rect<T>::grownBy( double f ) const
{
    f *= .5;
    return Rect<T>( widerPos(left(),right(),f), widerPos(top(),bottom(),f),
		    widerPos(right(),left(),f), widerPos(bottom(),top(),f) );
}


template <class T>
inline bool Rect<T>::operator >( const Rect<T>& r ) const
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
inline void Rect<T>::fitIn( const Rect<T>& r )
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
