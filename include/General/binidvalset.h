#ifndef binidvalset_h
#define binidvalset_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		July 2004
 RCS:		$Id$
________________________________________________________________________

-*/

#include "generalmod.h"
#include "posidxpairvalset.h"
#include "binid.h"


/*!\brief A Pos::IdxPairValueSet with BinIDs. */


mExpClass(General) BinIDValueSet : public Pos::IdxPairValueSet
{
public:

    			BinIDValueSet( int nrvals, bool allowdup )
			    : Pos::IdxPairValueSet(nrvals,allowdup)	{}
			BinIDValueSet( const BinIDValueSet& bvs )
			    : Pos::IdxPairValueSet(bvs)			{}
    BinIDValueSet&	operator =( const BinIDValueSet& oth )
			{ Pos::IdxPairValueSet::operator=(oth); return *this; }

    inline void		allowDuplicateBinIDs( bool yn )
					{ allowDuplicateIdxPairs(yn); }
    inline bool		hasDuplicateBinIDs() const
					{ return hasDuplicateIdxPairs(); }
    inline bool		nrDuplicateBinIDs() const
					{ return nrDuplicateIdxPairs(); }

    inline BinID	getBinID( const SPos& spos ) const
					{ return mkBinID(getIdxPair(spos)); }
    inline BinID	firstBinID() const
					{ return mkBinID(firstIdxPair()); }

protected:

    friend class	DataPointSet;
    friend class	PosVecDataSet;

    inline static BinID	mkBinID( const Pos::IdxPair& ip )
			{ return BinID( ip.first, ip.second ); }

};


#endif

