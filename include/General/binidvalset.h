#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"
#include "posidxpairvalset.h"
#include "binid.h"


/*!\brief A Pos::IdxPairValueSet with BinIDs. */


mExpClass(General) BinIDValueSet : public Pos::IdxPairValueSet
{
public:

			BinIDValueSet(int nrvals,bool allowdup);
			BinIDValueSet(const BinIDValueSet&);
			~BinIDValueSet();

    BinIDValueSet&	operator =(const BinIDValueSet&);

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

    void		setStepout(int trcstepout,int trcstep=1);
    void		setStepout(const IdxPair& stepout,const IdxPair& step);

    void		setIs2D(bool yn);
    bool		is2D() const;

protected:

    friend class	DataPointSet;
    friend class	PosVecDataSet;

    inline static BinID	mkBinID( const Pos::IdxPair& ip )
			{ return BinID( ip.first, ip.second ); }

private:
    void		addHorPosIfNeeded(const Pos::IdxPair&);
    bool		is2d_		= false;
};
