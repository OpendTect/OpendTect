#ifndef rowcol_H
#define rowcol_H

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		12-8-1997
 RCS:		$Id: rowcol.h,v 1.13 2004-06-02 09:21:35 kristofer Exp $
________________________________________________________________________

-*/


/*!\brief Object with row and col. */

template <class T> class TypeSet;

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
    RowCol	operator+() const { return RowCol( +row, +col ); }
    RowCol	operator-() const { return RowCol( -row, -col ); }
    RowCol	operator*( const RowCol& rc ) const
		{ return RowCol( row*rc.row, col*rc.col ); }
    RowCol	operator/( const RowCol& rc ) const
		{ return RowCol( row/rc.row, col/rc.col ); }
    int		operator==( const RowCol& rc ) const
		{ return row == rc.row && col == rc.col; }
    int		operator!=( const RowCol& rc ) const
		{ return ! (rc == *this); }

    float	angleTo(const RowCol&) const;
    		/*!<\returns the smallest angle between the vector going from
		     0,0 to the object and the vector going from 0,0 to rc.*/
    float	clockwiseAngleTo(const RowCol& rc) const;
    		/*!<\returns the angle between the vector going from 0,0 to the
		     object and the vector going from 0,0 to rc in the
		     clockwise direction.*/
    float	counterClockwiseAngleTo(const RowCol&) const;
    		/*!<\returns the angle between the vector going from 0,0 to the
		     object and the vector going from 0,0 to rc in the
		     counterclockwise direction.*/

    RowCol	getDirection() const;
    		/*!\returns a rowcol where row/col are either -1, 0 or 1 where
		    depending on if row/col of the object is negative, zero or
		    positive. 
		*/

    bool	isNeighborTo( const RowCol&,
	    		      const RowCol& step=RowCol(1,1),
			      bool eightconnectivity=true ) const;
    		/*!<\returns true if the object is a neighbor with the 
		     provided RowCol. The neighborhood is defined with either
		     eight- or four-connectivity */

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
