#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert/Salil
 Date:		Oct 2013
________________________________________________________________________

-*/

#include "basicmod.h"
#include "idxpair.h"


namespace Pos
{

typedef ::Index_Type_Pair Index_Type_Pair;
typedef IdxPair IdxPairDelta;
typedef IdxPair IdxPairStep;


/*!\brief IdxPair with position indices; base class for BinID et al. */

mExpClass(Basic) IdxPair : public ::IdxPair
{
public:

    inline		IdxPair() : ::IdxPair(0,0)	{}
    inline		IdxPair(pos_type,pos_type);
			mImplSimpleEqOpers2Memb(IdxPair,first(),second())
    inline bool		operator <(const IdxPair& oth) const;
    inline bool		operator >(const IdxPair& oth) const;

				// aliases for first
    inline pos_type&	inl()		{ return first(); }
    inline pos_type&	lineNr()	{ return first(); }
    inline pos_type&	row()		{ return first(); }

				// aliases for second
    inline pos_type&	crl()		{ return second(); }
    inline pos_type&	trcNr()		{ return second(); }
    inline pos_type&	col()		{ return second(); }

				// const versions of the aliases
    inline pos_type	inl() const	{ return first(); }
    inline pos_type	crl() const	{ return second(); }
    inline pos_type	lineNr() const	{ return first(); }
    inline pos_type	trcNr() const	{ return second(); }
    inline pos_type	row() const	{ return first(); }
    inline pos_type	col() const	{ return second(); }

    inline od_int64	toInt64() const;
    inline static IdxPair fromInt64(od_int64);
    od_int64		sqDistTo(const IdxPair&) const;
    bool		isNeighborTo(const IdxPair&,
			 const IdxPairStep&,bool conn8=true) const;

    static const IdxPair& udf();
    inline void		setUdf()	{ *this = udf(); }

};


inline IdxPair::IdxPair( pos_type f, pos_type s )
    : ::IdxPair(f,s)
{
}


inline bool IdxPair::operator <( const IdxPair& oth ) const
{
    return first() < oth.first() || (first() == oth.first()
				 && second() < oth.second());
}


inline bool IdxPair::operator >( const IdxPair& oth ) const
{
    return first() > oth.first() || (first() == oth.first()
				 && second() > oth.second() );
}


inline od_int64 IdxPair::toInt64() const
{
    return (((od_uint64)first()) << 32) + (((od_uint64)second()) & 0xFFFFFFFF);
}


inline IdxPair IdxPair::fromInt64( od_int64 i64 )
{
    return IdxPair( (pos_type)(i64 >> 32), (pos_type)(i64 & 0xFFFFFFFF) );
}


} // namespace Pos
