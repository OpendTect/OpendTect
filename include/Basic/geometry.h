#ifndef geometry_h
#define geometry_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          01/02/2000
 RCS:           $Id: geometry.h,v 1.4 2000-08-04 16:49:30 bert Exp $
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

    inline Point<T>	topLeft() const 		{ return topLeft_; }
    inline Point<T>	bottomRight() const 		{ return bottomRight_; }
    inline Point<T>	centre() const 		
                        { return Point<T>
				 (
				    ( ( topLeft_.x() + bottomRight_.x() ) / 2 ),
				    ( ( topLeft_.y() + bottomRight_.y() ) / 2 ) 
				 ); 
                        }
    inline void		setTopLeft( Point<T> tl )	{ topLeft_ = tl; }
    inline void		setBottomRight( Point<T> br )	{ bottomRight_ = br; }

    inline bool		contains( const Point<T>& pt ) const
			{
			    return (left()-pt.x())*(pt.x()-right()) >= 0
				&& (top()-pt.y())*(pt.y()-bottom()) >= 0;
			}
    inline bool		isInside( const Rect<T>& other ) const
			{
			    return other.contains(topLeft())
				&& other.contains(bottomRight());
			}

    inline T 		width() const
			{
			    return (right() > left()) ? right() - left() 
				    : left() - right(); 
			}
    inline T 		height() const
			{
			    return (bottom() > top()) ? bottom() - top()
				    : top() - bottom(); 
			}

    inline T 		left() const 		{ return topLeft_.x(); }
    inline T 		top() const 		{ return topLeft_.y(); }
    inline T 		right() const 		{ return bottomRight_.x(); }
    inline T 		bottom() const		{ return bottomRight_.y(); }
    inline void 	setLeft( T val )	{ topLeft_.setX( val ); }
    inline void 	setTop( T val )		{ topLeft_.setY( val ); }
    inline void 	setRight( T val )	{ bottomRight_.setX( val ); }
    inline void 	setBottom( T val )	{ bottomRight_.setY( val ); }

    inline Size2D<T>	size() const { return Size2D<T>( width(), height() ); }
    inline void 	zero()	{ topLeft_.zero(); bottomRight_.zero(); }

protected:

    Point<T> 	topLeft_;
    Point<T>	bottomRight_;

};


#endif
