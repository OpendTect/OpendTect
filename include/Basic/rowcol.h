#ifndef rowcol_h
#define rowcol_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		12-8-1997
 RCS:		$Id: rowcol.h,v 1.21 2005-01-13 11:59:26 nanne Exp $
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
		RowCol( const int64& ser ) { setSerialized(ser); }
		RowCol() : row( 0 ), col ( 0 )			{}

    int&	r() { return row; }
    int		r() const { return row; }
    int&	c() { return col; }
    int		c() const { return col; }

		/* Implements +, -, * and other operators. See the documentation
		   for details */
    		mRowColFunctions( RowCol, row, col );

    RowCol	getDirection() const;
    		/*!<\returns a rowcol where row/col are either -1, 0 or 1 where
		    depending on if row/col of the object is negative, zero or
		    positive. 
		*/

    int		row;
    int		col;

    static const TypeSet<RowCol>&	clockWiseSequence();

private:
};


inline int rc2int( const RowCol& rc )
{
    return (((unsigned int) rc.row)<<16)+
	    ((unsigned int) rc.col & 0xFFFF);
}


inline int64 rc2int64( const RowCol& rc )
{
    return (((uint64) rc.row)<<32)+
	    ((uint64) rc.col & 0xFFFFFFFF);
}


inline RowCol int642rc( const int64& ll )
{
    return RowCol( ll>>32, ((int) (ll& 0xFFFFFFFF)) );
}


inline RowCol int2rc( const int& ll )
{
    return RowCol( ll>>16, ((short)(ll&0xFFFF)) );
}


#endif
