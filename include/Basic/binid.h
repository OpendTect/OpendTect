#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "bufstring.h"
#include "posidxpair.h"

/*! \brief Positioning in a seismic survey: inline/crossline or lineNr/trcNr. */


mExpClass(Basic) BinID : public Pos::IdxPair
{
public:

    inline			BinID()				{}
    inline			BinID(IdxType i,IdxType x);
    inline			BinID(const Pos::IdxPair&);
    inline			BinID(const BinID&);

				// aliases for first
    inline IdxType&		inl()		{ return first; }
    inline IdxType&		lineNr()	{ return first; }

				// aliases for second
    inline IdxType&		crl()		{ return second; }
    inline IdxType&		trcNr()		{ return second; }

				// const versions of the aliases
    inline IdxType		inl() const	{ return first; }
    inline IdxType		crl() const	{ return second; }
    inline IdxType		lineNr() const	{ return first; }
    inline IdxType		trcNr() const	{ return second; }

    inline const BinID&		operator+=(const BinID&);
    inline const BinID&		operator-=(const BinID&);
    inline BinID		operator+(const BinID&) const;
    inline BinID		operator-(const BinID&) const;
    inline BinID&		operator=(const BinID&);

    inline BinID		operator*(const Pos::Index_Type_Pair&) const;
    inline BinID		operator/(const Pos::Index_Type_Pair&) const;

    inline BinID		operator*(int) const;
    inline BinID		operator/(int) const;
    inline BinID		operator-() const;

    inline static BinID		noStepout()	{ return BinID(0,0); }

    inline static BinID		fromInt64(od_int64);

    inline const char*		toString(bool is2d=false) const;
    inline bool			fromString(const char*);

    inline BufferString		usrDispStr() const;
};


inline BinID::BinID( BinID::IdxType i, BinID::IdxType c )
    : Pos::IdxPair(i,c)
{
}


inline BinID::BinID( const Pos::IdxPair& p )
    : Pos::IdxPair(p)
{
}


BinID::BinID( const BinID& bid )
    : Pos::IdxPair(bid.inl(),bid.crl())
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

inline BinID BinID::operator-() const
{ return BinID( -inl(), -crl() ); }

BinID& BinID::operator=( const BinID& bid )
{ inl() = bid.inl(); crl() = bid.crl(); return *this; }

inline BinID BinID::operator*( const Pos::Index_Type_Pair& ip ) const
{ return BinID( first*ip.first, second*ip.second ); }

inline BinID BinID::operator/( const Pos::Index_Type_Pair& ip ) const
{ return BinID( first/ip.first, second/ip.second ); }

inline BinID BinID::operator*( int factor ) const
{ return BinID( inl()*factor, crl()*factor ); }

inline BinID BinID::operator/( int denominator ) const
{ return BinID( inl()/denominator, crl()/denominator ); }


inline BinID BinID::fromInt64( od_int64 i64 )
{
    Pos::IdxPair p( Pos::IdxPair::fromInt64(i64) );
    return BinID( p.first, p.second );
}


inline const char* BinID::toString( bool is2d ) const
{
    return IdxPair::getUsrStr( "", "/", "", is2d );
}


inline bool BinID::fromString( const char* str )
{
    return IdxPair::parseUsrStr( str, "", "/", "" );
}


BufferString BinID::usrDispStr() const
{
    BufferString ret;
    ret.set( inl() ).add( "/" ).add( crl() );
    return ret;
}
