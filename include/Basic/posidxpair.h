#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "idxpair.h"


namespace Pos
{

using Index_Type_Pair = ::Index_Type_Pair;
using IdxPairDelta = IdxPair;
using IdxPairStep = IdxPair;


/*!\brief IdxPair with position indices; base class for BinID et al. */

mExpClass(Basic) IdxPair : public ::IdxPair
{
public:
				IdxPair();
				IdxPair(IdxType _first,IdxType _second);
				IdxPair(const Pos::IdxPair&);
				~IdxPair();

    inline IdxPair&		operator=(const IdxPair&);
    inline bool			operator ==(const IdxPair&) const;
    inline bool			operator !=( const IdxPair& oth ) const
						{ return !(*this == oth); }
    inline bool			operator <(const IdxPair& oth) const;
    inline bool			operator >(const IdxPair& oth) const;

				// aliases for first
    inline IdxType&		row()		{ return first; }

				// aliases for second
    inline IdxType&		col()		{ return second; }

				// const versions of the aliases
    inline IdxType		row() const	{ return first; }
    inline IdxType		col() const	{ return second; }

    inline od_int64		toInt64() const;
    inline static IdxPair	fromInt64(od_int64);
    od_int64			sqDistTo(const IdxPair&) const;
    bool			isNeighborTo(const IdxPair&,
					     const IdxPairStep&,
					     bool conn8=true) const;

    static const IdxPair&	udf();
};



inline IdxPair& IdxPair::operator=( const IdxPair& oth )
{
    first = oth.first;
    second = oth.second;
    return *this;
}


inline bool IdxPair::operator ==( const IdxPair& oth ) const
{
    return first == oth.first && second == oth.second;
}


inline bool IdxPair::operator <( const IdxPair& oth ) const
{
    return first < oth.first || (first == oth.first && second < oth.second );
}


inline bool IdxPair::operator >( const IdxPair& oth ) const
{
    return first > oth.first || (first == oth.first && second > oth.second );
}


inline od_int64 IdxPair::toInt64() const
{
    return (((od_uint64)first) << 32) + (((od_uint64)second) & 0xFFFFFFFF);
}


inline IdxPair IdxPair::fromInt64( od_int64 i64 )
{
    return IdxPair( (IdxType)(i64 >> 32), (IdxType)(i64 & 0xFFFFFFFF) );
}


} // namespace Pos
