#ifndef binid_h
#define binid_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		21-6-1996
 Contents:	Positions: Inline/crossline and Coordinate
 RCS:		$Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "posidxpair.h"


/*!
\brief Positioning in a seismic survey: inline/crossline. Most functions are
identical to RowCol.
*/

mExpClass(Basic) BinID : public Pos::IdxPair
{
public:

    inline			BinID()				{}
    inline			BinID(IdxType i,IdxType x);
    inline			BinID(const Pos::IdxPair&);

    inline const BinID&		operator+=(const BinID&);
    inline const BinID&		operator-=(const BinID&);
    inline BinID	 	operator+(const BinID&) const;
    inline BinID	 	operator-(const BinID&) const;
    inline BinID		operator+() const;
    inline BinID		operator-() const;
    inline BinID		operator*(const BinID&) const;
    inline BinID		operator*(int) const;
    inline BinID		operator/(const BinID&) const;
    inline BinID		operator/(int) const;
    inline static BinID		fromInt64(od_int64);

    inline const char*		getUsrStr(bool is2d=false) const;
    inline bool			parseUsrStr(const char*);

};

// See remarks for IdxPair equivalents
typedef BinID BinIDDelta;
typedef BinID BinIDAbsDelta;
typedef BinID BinIDRelDelta;
typedef BinID BinIDDeltaStep;


inline BinID::BinID( BinID::IdxType i, BinID::IdxType c )
    : Pos::IdxPair(i,c)
{
}


inline BinID::BinID( const Pos::IdxPair& p )
    : Pos::IdxPair(p)
{
}


inline const BinID& BinID::operator+=( const BinID& bid )
{ inl() += bid.inl(); crl() += bid.crl(); return *this; }


inline const BinID& BinID::operator-=( const BinID& bid )
{ inl() -= bid.inl(); crl() -= bid.crl(); return *this; }


inline BinID BinID::operator+( const BinID& bid ) const
{ return BinID( inl()+bid.inl(), crl()+bid.crl() ); }


inline BinID BinID::operator-( const BinID& bid ) const
{ return BinID( inl()-bid.inl(), crl()-bid.crl() ); }


inline BinID BinID::operator+() const
{ return BinID( +inl(), +crl() ); }


inline BinID BinID::operator-() const
{ return BinID( -inl(), -crl() ); }


inline BinID BinID::operator*( const BinID& bid ) const
{ return BinID( inl()*bid.inl(), crl()*bid.crl() ); }


inline BinID BinID::operator*( int factor ) const
{ return BinID( inl()*factor, crl()*factor ); }


inline BinID BinID::operator/( const BinID& rc ) const
{ return BinID( inl()/rc.inl(), col()/rc.crl() ); }


inline BinID BinID::operator/( int denominator ) const
{ return BinID( inl()/denominator, crl()/denominator ); }


inline BinID BinID::fromInt64( od_int64 i64 )
{
    Pos::IdxPair p( Pos::IdxPair::fromInt64(i64) );
    return BinID( p.first, p.second );
}


inline const char* BinID::getUsrStr( bool is2d ) const
{
    return Pos::IdxPair::getUsrStr( "", "/", "", is2d );
}


inline bool BinID::parseUsrStr( const char* str )
{
    return Pos::IdxPair::parseUsrStr( str, "", "/", "" );
}



#endif
