#ifndef geometry_h
#define geometry_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          01/02/2000
 RCS:           $Id: geometry.h,v 1.2 2000-07-14 14:59:53 arend Exp $
________________________________________________________________________

-*/

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

    inline bool		inside( const Rect<T>& other ) const
			{ 
			    return ( (left()   >= other.left() ) 
				  && (top()    >= other.top()  )
				  && (right()  <= other.right()  )
				  && (bottom() <= other.bottom() ) );
			}
    inline Rect<T>	selectArea( const Rect<T>& other ) const
			{
			    T hOffset = other.left() - left();
			    T vOffset = other.top() - top();

			    return Rect<T> (hOffset,
					    vOffset,
					    other.width() + hOffset,
					    other.height() + vOffset);
			} 

    inline bool 	topToAtLeast( T ref )
			{
			    if ( top() < ref ) 
			    { 
				T shift = ref - top();
				setTop( top() + shift ); 
				setBottom( bottom() + shift);
				return true; 
			    }
			    return false;
			} 

    inline void 	topTo( T ref )
			{
			    T shift = ref - top();
			    setTop( top() + shift ); 
			    setBottom( bottom() + shift);
			} 

    inline bool 	bottomToAtLeast( T ref )
			{
			    if ( bottom < ref ) 
			    { 
				T shift = ref - bottom();
				setTop( top() + shift ); 
				setBottom( bottom() + shift);
			    }
			    return false;
			} 

    inline bool 	leftToAtLeast( T ref )
			{
			    if ( left() < ref ) 
			    { 
				T shift = ref - left();
				setLeft( left() + shift ); 
				setRight( right() + shift );
				return true; 
			    }
			    return false;
			} 

    inline void 	leftTo( T ref )
			{
			    T shift = ref - left();
			    setLeft( left() + shift ); 
			    setRight( right() + shift );
			} 

    inline bool 	rightToAtLeast( T ref )
			{
			    if ( right() < ref ) 
			    { 
				T shift = ref - right();
				setLeft( left() + shift ); 
				setRight( right() + shift );
				return true; 
			    }
			    return false;
			} 

    inline void 	setWidth( T ref ) 
			{
			    setRight( left() + ref );
			}

    inline void 	setHeight( T ref ) 
			{
			    setBottom( top() + ref );
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
