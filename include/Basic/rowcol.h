#ifndef rowcol_H
#define rowcol_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		12-8-1997
 RCS:		$Id: rowcol.h,v 1.6 2003-06-17 13:52:50 kristofer Exp $
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
    void	operator*=( const RowCol& rc )
		{ row *= rc.row; col *= rc.col; }
    void	operator/=( const RowCol& rc )
		{ row /= rc.row; col /= rc.col; }
    RowCol	operator+( const RowCol& rc ) const
		{ return RowCol( row+rc.row, col+rc.col ); }
    RowCol	operator-( const RowCol& rc ) const
		{ return RowCol( row-rc.row, col-rc.col ); }
    RowCol	operator*( const RowCol& rc ) const
		{ return RowCol( row*rc.row, col*rc.col ); }
    RowCol	operator/( const RowCol& rc ) const
		{ return RowCol( row/rc.row, col/rc.col ); }
    int		operator==( const RowCol& rc ) const
		{ return row == rc.row && col == rc.col; }
    int		operator!=( const RowCol& rc ) const
		{ return ! (rc == *this); }

    int		row;
    int		col;

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
    return RowCol( ll>>32, ll& 0xFFFFFFFF );
}


inline RowCol long2rc( const long& ll )
{
    return RowCol( ll>>16, ll& 0xFFFF );
}


#endif
