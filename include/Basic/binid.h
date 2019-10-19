#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		21-6-1996
________________________________________________________________________

-*/

#include "posidxpair.h"
#include "bufstring.h"


/*! \brief Positioning in a seismic survey: inline/crossline or lineNr/trcNr. */


mExpClass(Basic) BinID : public Pos::IdxPair
{
public:

    typedef Pos::IdxPair	IdxPair;

    inline			BinID()				{}
    inline			BinID( pos_type i, pos_type c )
				    : IdxPair(i,c)		{}
    explicit inline		BinID( const IdxPair& ip )
				    : IdxPair(ip)		{}

    inline const BinID&		operator+=(const IdxPair&);
    inline const BinID&		operator-=(const IdxPair&);
    inline BinID		operator+(const IdxPair&) const;
    inline BinID		operator-(const IdxPair&) const;
    inline BinID		operator-() const;

    inline BinID		operator*(const IdxPair&) const;
    inline BinID		operator/(const IdxPair&) const;

    inline BinID		operator*(int) const;
    inline BinID		operator/(int) const;

    inline static BinID		fromInt64(od_int64);
    Coord			coord() const;

    inline const char*		toString(bool is2d=false) const;
    inline bool			fromString(const char*);
    inline BufferString		usrDispStr() const;
    BufferString		usrDispStr(bool is2d) const;

    static BinID		udf()	{ return BinID(IdxPair::udf()); }

};


inline const BinID& BinID::operator+=( const IdxPair& bid )
	{ inl() += bid.inl(); crl() += bid.crl(); return *this; }
inline const BinID& BinID::operator-=( const IdxPair& bid )
	{ inl() -= bid.inl(); crl() -= bid.crl(); return *this; }
inline BinID BinID::operator+( const IdxPair& bid ) const
	{ return BinID( inl()+bid.inl(), crl()+bid.crl() ); }
inline BinID BinID::operator-( const IdxPair& bid ) const
	{ return BinID( inl()-bid.inl(), crl()-bid.crl() ); }
inline BinID BinID::operator*( const IdxPair& ip ) const
	{ return BinID( first()*ip.first(), second()*ip.second() ); }
inline BinID BinID::operator/( const IdxPair& ip ) const
	{ return BinID( first()/ip.first(), second()/ip.second() ); }
inline BinID BinID::operator*( int fac ) const
	{ return BinID( inl()*fac, crl()*fac ); }
inline BinID BinID::operator/( int fac ) const
	{ return BinID( inl()/fac, crl()/fac ); }
inline BinID BinID::operator-() const
	{ return BinID( -inl(), -crl() ); }


inline BinID BinID::fromInt64( od_int64 i64 )
{
    IdxPair p( IdxPair::fromInt64(i64) );
    return BinID( p.first(), p.second() );
}


inline const char* BinID::toString( bool is2d ) const
{
    return IdxPair::getUsrStr( "", "/", "", is2d );
}


inline BufferString BinID::usrDispStr() const
{
    BufferString ret;
    ret.set( inl() ).add( "/" ).add( crl() );
    return ret;
}


inline bool BinID::fromString( const char* str )
{
    return IdxPair::parseUsrStr( str, "", "/", "" );
}
