#ifndef uigeom_h
#define uigeom_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          01/02/2000
 RCS:           $Id: uigeom.h,v 1.15 2007-02-14 10:13:13 cvsbert Exp $
________________________________________________________________________

-*/

#include "geometry.h"

typedef Geom::Point2D<int> uiPoint;
typedef Geom::Point2D<double> uiWorldPoint;
typedef Geom::PosRectangle<double> uiWorldRect;

class uiSize : public Geom::Size2D<int>
{
public:
    			uiSize( int wdt=0 , int hgt=0 )
			    : Geom::Size2D<int>(wdt,hgt)		{}

    inline int          hNrPics() const		{ return width_; }
    inline int          vNrPics() const		{ return height_; }
			//! nr of pics should be > 0 
    inline void		setHNrPics( int np )	{ width_ = mMAX(np,1); }
			//! nr of pics should be > 0 
    inline void		setVNrPics( int np )	{ height_ = mMAX(np,1); }
};


class uiRect  : public Geom::PixRectangle<int>
{
public:
    inline		uiRect( int l = 0 , int t = 0, int r = 0 , int b = 0 );
    inline		uiRect( const uiPoint& tl, const uiPoint& br );
    inline uiSize	getPixelSize() const;

    inline uiRect	selectArea( const uiRect& other ) const;
    inline bool 	topToAtLeast( int ref );
    inline void 	topTo( int ref );
    inline bool 	bottomToAtLeast( int ref );
    inline void 	bottomTo( int ref );
    inline bool 	leftToAtLeast( int ref );
    inline void 	leftTo( int ref );
    inline void 	rightTo( int ref );
    inline bool 	rightToAtLeast( int ref );
    inline void		expandTo( const uiRect& oth );
    inline int          hNrPics() const;
    inline int          vNrPics() const;
    inline void		setHNrPics( int np );
    inline void		setVNrPics( int np );
};


inline uiRect::uiRect( int l, int t, int r, int b )
    : Geom::PixRectangle<int>( l, t, r, b )
{}


inline uiRect::uiRect( const uiPoint& tl, const uiPoint& br )
    : Geom::PixRectangle<int>( tl, br )
{}


uiSize	uiRect::getPixelSize() const
{ return uiSize( hNrPics(),vNrPics() ); }


inline uiRect uiRect::selectArea( const uiRect& other ) const
{
    int hOffset = other.left() - left();
    int vOffset = other.top() - top();
    return uiRect( hOffset, vOffset,
		   other.width(), other.height() );
} 


inline bool uiRect::topToAtLeast( int ref )
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


inline void uiRect::topTo( int ref )
{
    int shift = ref - top();
    setTop( top() + shift ); 
    setBottom( bottom() + shift);
} 


inline bool uiRect::bottomToAtLeast( int ref )
{
    int shift = ref - bottom();
    if ( shift > 0 ) 
    { 
	setTop( top() + shift ); 
	setBottom( bottom() + shift);
    }
    return false;
} 


inline void uiRect::bottomTo( int ref )
{
    int shift = ref - bottom();
    setTop( top() + shift ); 
    setBottom( bottom() + shift);
} 


inline bool uiRect::leftToAtLeast( int ref )
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

inline void uiRect::leftTo( int ref )
{
    int shift = ref - left();
    setLeft( left() + shift ); 
    setRight( right() + shift );
} 


inline void uiRect::rightTo( int ref )
{
    int shift = ref - right();
    setLeft( left() + shift ); 
    setRight( right() + shift );
} 


inline bool uiRect::rightToAtLeast( int ref )
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


inline void uiRect::expandTo( const uiRect& oth )
{
    checkCorners();
    topLeft_.x = mMIN( topLeft_.x, oth.topLeft_.x );
    topLeft_.y = mMIN( topLeft_.y, oth.topLeft_.y );
    bottomRight_.x = mMAX( bottomRight_.x,
			   oth.bottomRight_.x );
    bottomRight_.y = mMAX( bottomRight_.y,
			   oth.bottomRight_.y );
}


inline int uiRect::hNrPics() const		{ return width() + 1; }


inline int uiRect::vNrPics() const		{ return height()+ 1; }


//! nr of pics should be > 0 
inline void uiRect::setHNrPics( int np )	
{ setRight( left() + mMAX( 1, np ) - 1 ); }


//! nr of pics should be > 0 
inline void uiRect::setVNrPics( int np )	
{ setBottom( top() + mMAX( 1, np ) - 1 ); }

#endif
