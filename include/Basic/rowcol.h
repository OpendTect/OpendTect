#ifndef rowcol_H
#define rowcol_H

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		12-8-1997
 RCS:		$Id: rowcol.h,v 1.1.1.1 1999-09-03 10:11:41 dgb Exp $
________________________________________________________________________

@$*/


class RowCol
{
public:
		RowCol( int r=0, int c=0 ) : row(r), col(c)	{}

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


/*$-*/
#endif
