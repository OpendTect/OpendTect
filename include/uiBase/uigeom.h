#ifndef uigeom_h
#define uigeom_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          01/02/2000
 RCS:           $Id: uigeom.h,v 1.13 2006-09-19 19:00:46 cvskris Exp $
________________________________________________________________________

-*/

#include "geometry.h"

typedef Geom::Point2D<int> uiPoint;
typedef Geom::Point2D<double> uiWorldPoint;
typedef Geom::PosRectangle<double> uiWorldRect;

class uiSize : public Geom::Size2D<int>
{
public:
			uiSize() : Geom::Size2D<int>( 0, 0 )	{}
			uiSize( const Geom::Size2D<int>& gs )
			    : Geom::Size2D<int>( gs )	{}

			//! inpixels=true : w and h are in pixels
			uiSize( int wdt , int hgt, bool inpixels )
			    : Geom::Size2D<int>( wdt, hgt )
			    { if ( !inpixels ) { width_++; height_++; } }

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
                        uiRect( int l = 0 , int t = 0, int r = 0 , int b = 0 )
			    : Geom::PixRectangle<int>( l, t, r, b )	{}

                        uiRect( const uiPoint& tl, const uiPoint& br )
			    : Geom::PixRectangle<int>( tl, br )	{}

    inline uiRect	selectArea( const uiRect& other ) const
			{
			    int hOffset = other.left() - left();
			    int vOffset = other.top() - top();
			    return uiRect( hOffset, vOffset,
					   other.width(), other.height() );
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

    void		expandTo( const uiRect& oth )
			{
			    checkCorners();
			    topLeft_.x = mMIN( topLeft_.x, oth.topLeft_.x );
			    topLeft_.y = mMIN( topLeft_.y, oth.topLeft_.y );
			    bottomRight_.x = mMAX( bottomRight_.x,
				    		   oth.bottomRight_.x );
			    bottomRight_.y = mMAX( bottomRight_.y,
				    		   oth.bottomRight_.y );
			}

    inline int          hNrPics() const		{ return width() + 1; }
    inline int          vNrPics() const		{ return height()+ 1; }
			//! nr of pics should be > 0 
    inline void		setHNrPics( int np )	
			    { setRight( left() + mMAX( 1, np ) - 1 ); }
			//! nr of pics should be > 0 
    inline void		setVNrPics( int np )	
			    { setBottom( top() + mMAX( 1, np ) - 1 ); }
};


#endif
