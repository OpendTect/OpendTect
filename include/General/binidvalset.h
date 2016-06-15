#ifndef binidvalset_h
#define binidvalset_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		July 2004
________________________________________________________________________

-*/

#include "generalmod.h"
#include "posidxpairvalset.h"
#include "trckey.h"


/*!\brief A Pos::IdxPairValueSet with BinIDs. */


mExpClass(General) BinIDValueSet : public Pos::IdxPairValueSet
{
public:

			BinIDValueSet(int nrvals,bool allowdup,
				      Pos::SurvID survid=TrcKey::std3DSurvID())
			    : Pos::IdxPairValueSet(nrvals,allowdup)
			    , survid_(survid)				{}
			BinIDValueSet(const BinIDValueSet& bvs)
			    : Pos::IdxPairValueSet(bvs)
			    , survid_(bvs.survID())			{}
    BinIDValueSet&	operator =( const BinIDValueSet& oth )
			{
			    Pos::IdxPairValueSet::operator=(oth);
			    survid_ = oth.survID();
			    return *this;
			}

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

    inline Pos::SurvID	survID() const	{ return survid_; }
    inline void		setSurvID(Pos::SurvID survid)	{  survid_ = survid; }

protected:

    friend class	DataPointSet;
    friend class	PosVecDataSet;

    Pos::SurvID		survid_;

    inline static BinID	mkBinID( const Pos::IdxPair& ip )
			{ return BinID( ip.first, ip.second ); }

};


#endif
