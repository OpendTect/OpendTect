#ifndef rowcol_H
#define rowcol_H

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		12-8-1997
 RCS:		$Id: rowcol.h,v 1.17 2004-09-21 15:58:07 kristofer Exp $
________________________________________________________________________

-*/

#include "rcol.h"


/*!\brief Object with row and col. */

template <class T> class TypeSet;

class RowCol : public RCol
{
public:
		RowCol( int row_, int col_ ) : row(row_), col(col_)	{}
		RowCol( const RCol& rc ) : row( rc.r() ), col ( rc.c() )			{}
		RowCol() : row( 0 ), col ( 0 )			{}

    int&	r() { return row; }
    const int&	r() const { return row; }
    int&	c() { return col; }
    const int&	c() const { return col; }

    RowCol	operator+( const RowCol& rc ) const
		{ return RowCol( row+rc.row, col+rc.col ); }
    RowCol	operator-( const RowCol& rc ) const
		{ return RowCol( row-rc.row, col-rc.col ); }
    RowCol	operator+() const { return RowCol( +row, +col ); }
    RowCol	operator-() const { return RowCol( -row, -col ); }
    RowCol	operator*( const RowCol& rc ) const
		{ return RowCol( row*rc.row, col*rc.col ); }
    RowCol	operator*( int factor ) const
		{ return RowCol( row*factor, col*factor ); }
    RowCol	operator/( const RowCol& rc ) const
		{ return RowCol( row/rc.row, col/rc.col ); }

    RowCol	getDirection() const;
    		/*!\returns a rowcol where row/col are either -1, 0 or 1 where
		    depending on if row/col of the object is negative, zero or
		    positive. 
		*/

    int		row;
    int		col;

    static const TypeSet<RowCol>&	clockWiseSequence();

private:
};


inline long rc2long( const RowCol& rc )
{
    return (((unsigned long) rc.row)<<16)+
	    ((unsigned long) rc.col & 0xFFFF);
}


inline long long rc2longlong( const RowCol& rc )
{
    return (((unsigned long long) rc.row)<<32)+
	    ((unsigned long long) rc.col & 0xFFFFFFFF);
}


inline RowCol longlong2rc( const long long& ll )
{
    return RowCol( ll>>32, ((long) (ll& 0xFFFFFFFF)) );
}


inline RowCol long2rc( const long& ll )
{
    return RowCol( ll>>16, ((short)(ll&0xFFFF)) );
}


#endif
