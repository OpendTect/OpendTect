#ifndef rowcol_h
#define rowcol_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		12-8-1997
 RCS:		$Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "posidxpair.h"
class RowCol;


typedef RowCol RowColDelta;


/*!\brief IdxPair used for its row() and col().  */

mExpClass(Basic) RowCol : public Pos::IdxPair
{
public:

    inline			RowCol()			{}
    inline			RowCol(int r,int c);
				RowCol(const Pos::IdxPair&);

    inline RowCol		operator+(const RowCol&) const;
    inline RowCol	 	operator-(const RowCol&) const;
    inline RowCol		operator+() const;
    inline RowCol		operator-() const;
    inline RowCol		operator*(const RowCol&) const;
    inline RowCol		operator*(int) const;
    inline RowCol		operator/(const RowCol&) const;
    inline RowCol		operator/(int) const;
    inline const RowCol&	operator+=(const RowCol&);
    inline const RowCol&	operator-=(const RowCol&);
    inline const RowCol&	operator*=(const RowCol&);
    inline const RowCol&	operator*=(int);
    inline const RowCol&	operator/=(const RowCol&);

    inline const char*		getUsrStr(bool is2d=false) const;
    inline bool			parseUsrStr(const char*);
    static inline RowCol	fromInt64(od_int64);
    inline int			toInt32() const;
    static inline RowCol	fromInt32(int);

    RowCol			getDirection() const;
    		/*!<\returns a rowcol where row/col are either -1, 0 or 1 where
		    depending on if row/col of the object is negative, zero or
		    positive. */

    float			angleTo(const RowCol&) const;
		/*!<\returns the smallest angle between the vector
		      going from 0,0 to the object and the vector
		      going from 0,0 to rc.*/
    float			clockwiseAngleTo(const RowCol& rc) const;
    		/*!<\returns the angle between the vector going from
		     0,0 to the object and the vector going from 0,0
		     to rc in the clockwise direction.*/
    float			counterClockwiseAngleTo(const RowCol&) const;
		/*!<\returns the angle between the vector going from
		      0,0 to the object and the vector going from 0,0
		      to rc in the counterclockwise direction.*/


    static const TypeSet<RowCol>&	clockWiseSequence();

};


inline RowCol::RowCol( RowCol::IdxType i, RowCol::IdxType c )
    : Pos::IdxPair(i,c)
{
}


inline RowCol::RowCol( const Pos::IdxPair& p )
    : Pos::IdxPair(p)
{
}


inline RowCol RowCol::fromInt64( od_int64 i64 )
{
    Pos::IdxPair p( Pos::IdxPair::fromInt64(i64) );
    return RowCol( p.first, p.second );
}



inline RowCol RowCol::operator+( const RowCol& rc ) const
	{ return RowCol( row()+rc.row(), col()+rc.col() ); }
inline RowCol RowCol::operator-( const RowCol& rc ) const
	{ return RowCol( row()-rc.row(), col()-rc.col() ); }
inline RowCol RowCol::operator+() const
	{ return RowCol( +row(), +col() ); }
inline RowCol RowCol::operator-() const
	{ return RowCol( -row(), -col() ); }
inline RowCol RowCol::operator*( const RowCol& rc ) const
	{ return RowCol( row()*rc.row(), col()*rc.col() ); }
inline RowCol RowCol::operator*( int factor ) const
	{ return RowCol( row()*factor, col()*factor ); }
inline RowCol RowCol::operator/( const RowCol& rc ) const
	{ return RowCol( row()/rc.row(), col()/rc.col() ); }
inline RowCol RowCol::operator/( int denominator ) const
	{ return RowCol( row()/denominator, col()/denominator ); }
inline const RowCol& RowCol::operator+=( const RowCol& rc )
	{ row() += rc.row(); col() += rc.col(); return *this; }
inline const RowCol& RowCol::operator-=( const RowCol& rc )
	{ row() -= rc.row(); col() -= rc.col(); return *this; }
inline const RowCol& RowCol::operator*=( const RowCol& rc )
	{ row() *= rc.row(); col() *= rc.col(); return *this; }
inline const RowCol& RowCol::operator*=( int factor )
	{ row() *= factor; col() *= factor;  return *this; } 
inline const RowCol& RowCol::operator/=( const RowCol& rc )
	{ row() /= rc.row(); col() /= rc.col();  return *this; } 
inline int RowCol::toInt32() const
	{ return (((unsigned int) row())<<16)+ ((unsigned int) col() & 0xFFFF);}
inline RowCol RowCol::fromInt32(int ll)
	{ return RowCol ( ll>>16, ((short)(ll&0xFFFF)) ); }



inline const char* RowCol::getUsrStr( bool is2d ) const
{
    return Pos::IdxPair::getUsrStr( "", "/", "", is2d );
}


inline bool RowCol::parseUsrStr( const char* str )
{
    return Pos::IdxPair::parseUsrStr( str, "", "/", "" );
}



#endif

