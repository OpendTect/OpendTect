#ifndef uigeom_h
#define uigeom_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          01/02/2000
 RCS:           $Id: uigeom.h,v 1.4 2001-10-17 11:53:08 arend Exp $
________________________________________________________________________

-*/

#include <geometry.h>

typedef Point<int> uiPoint;
typedef Point<double> uiWorldPoint;

typedef Size2D<int> uiSize;
typedef Rect<double> uiWorldRect;


class uiRect : public Rect<int>
{
public:
                        uiRect( int l = 0 , int t = 0, int r = 0 , int b = 0 )
                        : Rect<int>(l,t,r,b)	{}
                        uiRect( Point<int> tl, Point<int> br )
                        : Rect<int>(tl,br)	{}

    inline uiSize	pixelSize() const 
			    { return uiSize( hNrPics(), vNrPics() ); }

    inline uiRect	selectArea( const uiRect& other ) const
			{
			    int hOffset = other.left() - left();
			    int vOffset = other.top() - top();
			    return uiRect( hOffset, vOffset,
					   other.width() + hOffset,
					   other.height() + vOffset );
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


    inline int          hNrPics() const		{ return width()  + 1; }
    inline int          vNrPics() const		{ return height() + 1; }
			//! nr of pics should be > 0 
    inline void		setHNrPics( int np )	
			    { setRight( left() + mMAX( 1, np ) - 1 ); }
			//! nr of pics should be > 0 
    inline void		setVNrPics( int np )	
			    { setBottom( top() + mMAX( 1, np ) - 1 ); }


    inline void 	setWidth( int ref ) 
			{ setRight( left() + mMAX( 0,ref ) ); }
    inline void 	setHeight( int ref ) 
			{ setBottom( top() + mMAX( 0,ref ) ); }
};


#endif
