#ifndef uigeom_h
#define uigeom_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          01/02/2000
 RCS:           $Id: uigeom.h,v 1.5 2002-05-17 11:34:54 arend Exp $
________________________________________________________________________

-*/

#include <geometry.h>

typedef Point<int> uiPoint;
typedef Point<double> uiWorldPoint;
typedef Rect<double> uiWorldRect;

class uiSize 
{
public:
			uiSize()		{} 

			//! inpixels=true : w and h are in pixels
			uiSize( int wdt , int hgt, bool inpixels )
			    : hnp_( wdt ), vnp_( hgt )
			    { if( !inpixels ) { hnp_++; vnp_++; } }

    inline int          hNrPics() const		{ return hnp_; }
    inline int          vNrPics() const		{ return vnp_; }
			//! nr of pics should be > 0 
    inline void		setHNrPics( int np )	{ hnp_ = mMAX(np,1); }
			//! nr of pics should be > 0 
    inline void		setVNrPics( int np )	{ vnp_ = mMAX(np,1); }

    inline bool		operator ==( const uiSize& s ) const
			{ return s.hnp_ == hnp_ && s.vnp_ == vnp_; }
    inline bool		operator !=( const uiSize& s ) const
			{ return s.hnp_ != hnp_ || s.vnp_ != vnp_; }

    inline uiSize&	operator +=( int val )
			{ hnp_ += val; vnp_ += val; return *this; }
    inline uiSize&	operator -=( int val )
			{ hnp_ -= val; vnp_ -= val; return *this; }
    inline uiSize&	operator +=( const uiSize& s )
			{ hnp_+=s.hnp_; vnp_+=s.vnp_; return *this; }
    inline uiSize&	operator -=( const uiSize& s )
			{
			    hnp_ -= s.hnp_; vnp_ -= s.vnp_;
			    if ( hnp_ < 0 ) hnp_ = -hnp_;
			    if ( vnp_ < 0 ) vnp_ = -vnp_;
			    return *this;
			}

protected:

    int			hnp_;
    int			vnp_;

};

class uiRect 
{
public:
                        uiRect( int l = 0 , int t = 0, int r = 0 , int b = 0 )
			    : l_(l), t_(t), r_(r), b_(b)	{}

                        uiRect( Point<int> tl, Point<int> br )
			    : l_(tl.x()), t_(tl.y()), r_(br.x()), b_(br.y()) {}

                        uiRect( Point<int> tl, int wdt, int hgt, 
				bool hgtwdt_in_pics=true )
			    : l_(tl.x()), t_(tl.y())
			    , r_(tl.x() + wdt - (hgtwdt_in_pics ? 1 : 0))
			    , b_(tl.y() + hgt - (hgtwdt_in_pics ? 1 : 0))
			    {}

    inline int		left() const		{ return l_; }
    inline int		top() const		{ return t_; }
    inline int		right() const 		{ return r_; }
    inline int		bottom() const		{ return b_; }
    inline void		setLeft( int val )	{ l_ = val; }
    inline void		setTop( int val )	{ t_ = val; }
    inline void		setRight( int val )	{ r_ = val; }
    inline void		setBottom( int val )	{ b_ = val; }



    inline uiPoint	topLeft() const     { return uiPoint(l_,t_); }
    inline uiPoint	topRight() const    { return uiPoint(r_,t_); }
    inline uiPoint	bottomLeft() const  { return uiPoint(l_,b_); }
    inline uiPoint	bottomRight() const { return uiPoint(r_,b_); }
    inline uiPoint	centre() const 		
			    { return uiPoint( (l_+r_)/2, (t_+b_)/2 ); }

    inline uiRect&	zero()		{ t_= l_= r_= b_= 0; return *this; }

    inline uiSize	getsize() const 
			    { return uiSize( hNrPics(), vNrPics(), true ); }

    inline uiRect	selectArea( const uiRect& other ) const
			{
			    int hOffset = other.left() - left();
			    int vOffset = other.top() - top();
			    return uiRect( uiPoint(hOffset, vOffset),
					   other.hNrPics(), other.vNrPics() );
			} 
    inline bool 	topToAtLeast( int ref )
			{
			    int shift = ref - top();
			    if ( shift > 0 ) 
			    { 
				setTop( top() + shift ); 
				setBottom( bottom() + shift);
				return true; 
			    }
			    return false;
			} 
    inline void 	topTo( int ref )
			{
			    int shift = ref - top();
			    setTop( top() + shift ); 
			    setBottom( bottom() + shift);
			} 
    inline bool 	bottomToAtLeast( int ref )
			{
			    int shift = ref - bottom();
			    if ( shift > 0 ) 
			    { 
				setTop( top() + shift ); 
				setBottom( bottom() + shift);
			    }
			    return false;
			} 
    inline void 	bottomTo( int ref )
			{
			    int shift = ref - bottom();
			    setTop( top() + shift ); 
			    setBottom( bottom() + shift);
			} 
    inline bool 	leftToAtLeast( int ref )
			{
			    int shift = ref - left();
			    if ( shift > 0 )
			    { 
				setLeft( left() + shift ); 
				setRight( right() + shift );
				return true; 
			    }
			    return false;
			} 

    inline void 	leftTo( int ref )
			{
			    int shift = ref - left();
			    setLeft( left() + shift ); 
			    setRight( right() + shift );
			} 

    inline void 	rightTo( int ref )
			{
			    int shift = ref - right();
			    setLeft( left() + shift ); 
			    setRight( right() + shift );
			} 

    inline bool 	rightToAtLeast( int ref )
			{
			    int shift = ref - right();
			    if ( shift > 0 ) 
			    { 
				setLeft( left() + shift ); 
				setRight( right() + shift );
				return true; 
			    }
			    return false;
			} 


    inline int          hNrPics() const		{ return r_ - l_ + 1; }
    inline int          vNrPics() const		{ return b_ - t_ + 1; }
			//! nr of pics should be > 0 
    inline void		setHNrPics( int np )	
			    { setRight( left() + mMAX( 1, np ) - 1 ); }
			//! nr of pics should be > 0 
    inline void		setVNrPics( int np )	
			    { setBottom( top() + mMAX( 1, np ) - 1 ); }

    void                checkCorners( bool leftislow=true, bool topislow=true )
			{
			    if( leftislow == left() > right() )  swapHor();
			    if( topislow  == top()  > bottom() ) swapVer();
			}

    inline bool		isInside(const uiPoint& pt) const
	{
	    return pt.x() != t_ && pt.y() != l_
		&& pt.y() != r_ && pt.x() != b_
		&& ( (pt.x() - left() > 0) == (right() - pt.x() > 0) )
		&& ( (pt.y() - bottom() > 0) == (top() - pt.y() > 0) );
	}

    inline bool		isOutside( const uiPoint& p ) const
			{ return xOutside(p.x()) || yOutside(p.y()); }
    inline bool		isOnSide( const uiPoint& p ) const
			{ return !isInside(p) && !isOutside(p); }
    inline bool		contains( const uiPoint& p ) const
			{ return !isOutside(p); }

    inline bool		contains( const uiRect& other ) const
			{
			    return contains(other.topLeft())
				&& contains(other.bottomRight());
			}
    inline bool		isInside( const uiRect& other ) const
			{
			    return other.isInside(topLeft())
				&& other.isInside(bottomRight());
			}


    inline bool		xOutside( int x ) const
			{ return x != l_ && x != r_ && (x-l_ > 0 == x-r_ > 0); }
    inline bool		yOutside( int y ) const
			{ return y != b_ && y != t_ && (y-b_ > 0 == y-t_ > 0); }

protected:

    int			t_;
    int			l_;
    int			b_;
    int			r_;

    inline void         swapHor()
                        {
                            int t;
			    mSWAP(l_,r_,t);
                        }
    inline void         swapVer()
                        {
                            int t;
			    mSWAP(t_,b_,t);
                        }


};


#endif
