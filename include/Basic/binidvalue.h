#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "posidxpairvalue.h"
#include "binid.h"
class BinIDValues;

typedef Pos::ValueIdxPair<BinID,float> BinIDValueIdxPair;
typedef Pos::IdxPairValues<BinID,float> BinIDIdxPairValues;

/*!
\brief BinID and a value.

Can be used as a BinID, also contains a val().

*/

mExpClass(Basic) BinIDValue : public BinIDValueIdxPair
{
public:

    inline		BinIDValue( BinID::IdxType i=0, BinID::IdxType c=0,
				float v=mUdf(float) )
			    : BinIDValueIdxPair(i,c,v)	{}
    inline		BinIDValue( const BinID& b, float v=mUdf(float) )
			    : BinIDValueIdxPair(b,v)	{}
			BinIDValue(const BinIDValues&,int idx);


    inline bool		operator==( const BinIDValueIdxPair& oth ) const
			{ return BinIDValueIdxPair::operator==(oth); }
    inline bool		operator!=( const BinIDValueIdxPair& oth ) const
			{ return BinIDValueIdxPair::operator!=(oth); }
    bool		operator==(const BinID&) const;
    bool		operator!=(const BinID&) const;

};


/*!
\brief BinID and values. If one of the values is Z, make it the first one.

Can be used as a BinID, also contains various accesses to the TypeSet of values
contained.

*/

mExpClass(Basic) BinIDValues : public BinIDIdxPairValues
{
public:

    inline		BinIDValues( BinID::IdxType i=0, BinID::IdxType c=0,
	    				int n=2 )
			    : BinIDIdxPairValues(i,c,n)	{}
    inline		BinIDValues( const BinID& b, int n=2 )
			    : BinIDIdxPairValues(b,n)	{}
    inline		BinIDValues( const BinIDValue& biv )
			    : BinIDIdxPairValues(biv)	{}

    inline bool		operator==( const BinIDIdxPairValues& oth ) const
			{ return BinIDIdxPairValues::operator==(oth); }
    inline bool		operator!=( const BinIDIdxPairValues& oth ) const
			{ return BinIDIdxPairValues::operator!=(oth); }

    bool		operator==(const BinID&) const;
    bool		operator!=(const BinID&) const;

};
