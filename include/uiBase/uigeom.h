#ifndef uigeom_h
#define uigeom_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          01/02/2000
 RCS:           $Id: uigeom.h,v 1.1 2000-11-27 10:19:27 bert Exp $
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

    inline void 	setWidth( int ref ) 
			{ setRight( left() + ref ); }
    inline void 	setHeight( int ref ) 
			{ setBottom( top() + ref ); }

};


#endif
