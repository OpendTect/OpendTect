#ifndef binidvalue_h
#define binidvalue_h

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

    bool		operator==(const BinID&) const;
    bool		operator!=(const BinID&) const;

};


#endif
