#ifndef rowcol_H
#define rowcol_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		12-8-1997
 RCS:		$Id: rowcol.h,v 1.3 2002-09-25 05:37:25 kristofer Exp $
________________________________________________________________________

-*/


/*!\brief Object with row and col. */

class RowCol
{
public:
		RowCol( int r, int c ) : row(r), col(c)		{}
		RowCol() : row( 0 ), col ( 0 )			{}

    void	operator+=( const RowCol& rc )
		{ row += rc.row; col += rc.col; }
    void	operator-=( const RowCol& rc )
		{ row -= rc.row; col -= rc.col; }
    int		operator==( const RowCol& rc ) const
		{ return row == rc.row && col == rc.col; }
    int		operator!=( const RowCol& rc ) const
		{ return ! (rc == *this); }


    int		row;
    int		col;

};


#endif
