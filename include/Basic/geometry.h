#ifndef geometry_h
#define geometry_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          01/02/2000
 RCS:           $Id: geometry.h,v 1.7 2000-08-11 15:20:04 bert Exp $
________________________________________________________________________

-*/

#include <gendefs.h>


template <class T>
class Point
{
public:
			Point ( T xx = 0, T yy = 0 ) { x_ = xx; y_ = yy; }

    inline T		x() const		{ return x_; }
    inline T		y() const		{ return y_; }
    inline void		setX( T xx )		{ x_ = xx ; }  
    inline void		setY( T yy )		{ y_ = yy ; }  
    inline void		setXY( T xx, T yy )	{ x_ = xx ; y_ = yy; }  
    inline void		zero()			{ x_ = y_ = 0; }

    inline bool		operator ==( const Point<T>& p ) const
			{ return p.x_ == x_ && p.y_ == y_; }
    inline bool		operator !=( const Point<T>& p ) const
			{ return p.x_ != x_ || p.y_ != y_; }
    Point<T>&		operator +=( const Point<T>& p )
			{ x_ += p.x_; y_ += p.y_; }
    
protected:

    T 	x_;
    T 	y_;

};


template <class T>
class Size2D
{
public:
			Size2D( T w = 0 , T h = 0 ) 
				{ width_ = w; height_ = h; }

    inline bool		operator ==( const Size2D<T>& s ) const
			{ return s.width_ == width_ && s.height_ == height_; }
    inline bool		operator !=( const Size2D<T>& p ) const
			{ return s.width_ != width_ || s.height_ != height_; }

    inline T		width() const		{ return width_; }
    inline T		height() const		{ return height_; }
    inline void		setWidth( T val )	{ width_ = val; }
    inline void		setHeight( T val )	{ height_ = val; }

protected:

    T 	width_;
    T 	height_;

};


template <class T>
class Rect 
{
public:
			Rect ( T l = 0 , T t = 0, T r = 0 , T b = 0 ) 
			: topLeft_( Point<T>(l,t)) 
			, bottomRight_( Point<T>(r,b) ) {}
			Rect ( Point<T> tl, Point<T> br ) 
			: topLeft_( tl ) , bottomRight_( br ) {} 

    inline bool		operator ==( const Rect<T>& r ) const
			{ return   r.topLeft_ == topLeft_
				&& r.bottomRight_ == bottomRight_; }
    inline bool		operator !=( const Rect<T>& p ) const
			{ return   r.topLeft_ != topLeft_
				|| r.bottomRight_ != bottomRight_; }

    inline Point<T>	topLeft() const     { return topLeft_; }
    inline Point<T>	topRight() const    { return uiPoint(top(),right()); }
    inline Point<T>	bottomLeft() const  { return uiPoint(bottom(),left()); }
    inline Point<T>	bottomRight() const { return bottomRight_; }
    inline Point<T>	centre() const 		
                        { return Point<T>( (topLeft_.x()+bottomRight_.x())/2,
					   (topLeft_.y()+bottomRight_.y())/2 ); 
                        }
    inline void		setTopLeft( Point<T> tl )	{ topLeft_ = tl; }
    inline void		setBottomRight( Point<T> br )	{ bottomRight_ = br; }
    inline void		setTopRight( Point<T> tr )
			{ topLeft_.setY(tr.y()); bottomRight_.setX(tr.x()); }
    inline void		setBottomLeft( Point<T> tr )
			{ topLeft_.setX(tr.x()); bottomRight_.setY(tr.y()); }

    inline bool		contains(const Point<T>&) const;
    inline bool		isInside( const Rect<T>& other ) const
			{ return other.contains(topLeft())
			      && other.contains(bottomRight());
			}

    inline T 		width() const
			{ return right() - left(); }
    inline T 		height() const
			{ return revZ() ? bottom()-top() : top()-bottom(); }

    inline T 		left() const 		{ return topLeft_.x(); }
    inline T 		top() const 		{ return topLeft_.y(); }
    inline T 		right() const 		{ return bottomRight_.x(); }
    inline T 		bottom() const		{ return bottomRight_.y(); }
    inline void 	setLeft( T val )	{ topLeft_.setX( val ); }
    inline void 	setTop( T val )		{ topLeft_.setY( val ); }
    inline void 	setRight( T val )	{ bottomRight_.setX( val ); }
    inline void 	setBottom( T val )	{ bottomRight_.setY( val ); }

    inline Size2D<T>	size() const { return Size2D<T>( width(), height() ); }
    inline		operator Size2D<T>() const	{ return size(); }
    inline void 	zero()	{ topLeft_.zero(); bottomRight_.zero(); }

protected:

    Point<T> 	topLeft_;
    Point<T>	bottomRight_;

    inline bool	revZ() const		{ return bottom() > top(); }

};


template <class T>
inline bool Rect<T>::contains( const Point<T>& pt ) const
{
    return revZ()
	 ? ( pt.y() >= top()    && pt.y() <= bottom()
	  && pt.x() >= left()   && pt.x() <= right() )
	 : ( pt.y() >= bottom() && pt.y() <= top()
	  && pt.x() >= left()   && pt.x() <= right() );
}


#endif
