#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		July 2004
________________________________________________________________________

-*/

#include "generalmod.h"
#include "posidxpairvalset.h"
#include "binid.h"
#include "bin2d.h"
namespace PosInfo { class LineCollData; }
namespace Survey { class FullSubSel; }


/*!\brief A Pos::IdxPairValueSet with ether BinIDs or Bin2Ds. */


mExpClass(General) BinnedValueSet : public Pos::IdxPairValueSet
{
public:

    mUseType( OD,	GeomSystem );
    mUseType( Pos,	GeomID );
    mUseType( Pos,	IdxPair );
    mUseType( Pos,	IdxPairValueSet );

			BinnedValueSet(int nrvals,bool allowdup,
				      GeomSystem gs=OD::VolBasedGeom)
			    : IdxPairValueSet(nrvals,allowdup)
			    , geomsystem_(gs)				{}
			BinnedValueSet( const BinnedValueSet& oth )
			    : IdxPairValueSet(oth)
			    , geomsystem_(oth.geomsystem_)		{}
			BinnedValueSet(const PosInfo::LineCollData&,GeomSystem);
    BinnedValueSet&	operator =( const BinnedValueSet& oth )
			{
			    IdxPairValueSet::operator=(oth);
			    geomsystem_ = oth.geomsystem_;
			    return *this;
			}

    inline void		allowDuplicatePositions( bool yn )
			{ allowDuplicateIdxPairs(yn); }
    inline bool		hasDuplicatePositions() const
			{ return hasDuplicateIdxPairs(); }
    inline bool		nrDuplicatePositions() const
			{ return nrDuplicateIdxPairs(); }

    inline BinID	getBinID( const SPos& spos ) const
			{ return mkBinID(getIdxPair(spos)); }
    inline Bin2D	getBin2D( const SPos& spos ) const
			{ return mkBin2D(getIdxPair(spos)); }
    inline BinID	firstBinID() const
			{ return mkBinID(firstIdxPair()); }
    inline Bin2D	firstBin2D() const
			{ return mkBin2D(firstIdxPair()); }
    inline bool		isValid( const BinID& bid ) const
			{ return IdxPairValueSet::isValid(bid); }
    inline bool		isValid( const Bin2D& b2d ) const
			{ return IdxPairValueSet::isValid(b2d.idxPair()); }
    inline bool		includes( const BinID& bid ) const
			{ return IdxPairValueSet::includes(bid); }
    inline bool		includes( const Bin2D& b2d ) const
			{ return IdxPairValueSet::includes(b2d.idxPair()); }

    inline void		get( const SPos& spos, DataRow& dr ) const
			{ IdxPairValueSet::get( spos, dr ); }
    inline void		get( const SPos& spos, PairVal& pv ) const
			{ IdxPairValueSet::get( spos, pv ); }
    inline void		get( const SPos& spos, float* v=0, int mxnrv=-1 ) const
			{ IdxPairValueSet::get( spos, v, mxnrv ); }
    inline void		get( const SPos& spos, BinID& bid, float* v=0,
			     int mxnrv=-1 ) const
			{ IdxPairValueSet::get( spos, bid, v, mxnrv ); }
    inline void		get( const SPos& spos, Bin2D& b2d, float* v=0,
			     int mxnrv=-1 ) const
			{
			    BinID bid( b2d.idxPair() );
			    IdxPairValueSet::get( spos, bid, v, mxnrv );
			    b2d = Bin2D::decode( bid );
			}
    inline void		get( const SPos& spos, BinID& bid, float& v ) const
			{ IdxPairValueSet::get( spos, bid, v ); }
    inline void		get( const SPos& spos, Bin2D& b2d, float& v ) const
			{
			    BinID bid( b2d.idxPair() );
			    IdxPairValueSet::get( spos, bid, v );
			    b2d = Bin2D::decode( bid );
			}
    inline void		get( const SPos& spos, BinID& bid,
				float& v1, float& v2 ) const
			{ IdxPairValueSet::get( spos, bid, v1, v2 ); }
    inline void		get( const SPos& spos, Bin2D& b2d,
				float& v1, float& v2 ) const
			{
			    BinID bid( b2d.idxPair() );
			    IdxPairValueSet::get( spos, bid, v1, v2 );
			    b2d = Bin2D::decode( bid );
			}
    inline void		get( const SPos& spos, BinID& bid, TypeSet<float>& tf,
			     int mxnrv=-1) const
			{ IdxPairValueSet::get( spos, bid, tf, mxnrv ); }
    inline void		get( const SPos& spos, Bin2D& b2d, TypeSet<float>& tf,
			     int mxnrv=-1) const
			{
			    BinID bid( b2d.idxPair() );
			    IdxPairValueSet::get( spos, bid, tf, mxnrv );
			    b2d = Bin2D::decode( bid );
			}
    inline void		get( const SPos& spos, TypeSet<float>& tf,
			     int mxnrv=-1) const
			{ IdxPairValueSet::get( spos, tf, mxnrv ); }

    inline SPos		add( const BinID& bid, const float* vs=0 )
			{ return IdxPairValueSet::add( bid, vs ); }
    inline SPos		add( const Bin2D& b2d, const float* vs=0 )
			{ return IdxPairValueSet::add( b2d.idxPair(), vs ); }
    inline SPos		add( const BinID& bid, float v )
			{ return IdxPairValueSet::add( bid, v ); }
    inline SPos		add( const Bin2D& b2d, float v )
			{ return IdxPairValueSet::add( b2d.idxPair(), v ); }
    inline SPos		add( const BinID& bid, double v )
			{ return IdxPairValueSet::add( bid, v ); }
    inline SPos		add( const Bin2D& b2d, double v )
			{ return IdxPairValueSet::add( b2d.idxPair(), v ); }
    inline SPos		add( const BinID& bid, float v1, float v2 )
			{ return IdxPairValueSet::add( bid, v1, v2 ); }
    inline SPos		add( const Bin2D& b2d, float v1, float v2 )
			{ return IdxPairValueSet::add( b2d.idxPair(), v1, v2 );}
    inline SPos		add( const BinID& bid, const TypeSet<float>& tf )
			{ return IdxPairValueSet::add( bid, tf ); }
    inline SPos		add( const Bin2D& b2d, const TypeSet<float>& tf )
			{ return IdxPairValueSet::add( b2d.idxPair(), tf ); }
    inline SPos		add( const DataRow& dr )
			{ return IdxPairValueSet::add( dr ); }
    inline SPos		add( const PairVal& pv )
			{ return IdxPairValueSet::add( pv ); }
    inline void		add( const PosInfo::LineCollData& lcd )
			{ IdxPairValueSet::add( lcd ); }

    inline SPos		find( const BinID& bid ) const
			{ return data_.find( bid ); }
    inline SPos		find( const Bin2D& b2d ) const
			{ return data_.find( Pos::IdxPair( b2d.lineNr(),
							   b2d.trcNr() ) ); }
    inline SPos		findNearest( const BinID& bid ) const
			{ return data_.findNearest( bid ); }
    inline SPos		findNearest( const Bin2D& b2d ) const
			{ return data_.findNearestOnFirst( b2d.lineNr(),
							   b2d.trcNr() ); }

    void		setStepout(pos_type); // 2D
			    //!< may add positions that are outside line range
    void		setStepout(const IdxPair&,const IdxPair&); // 3D
			    //!< may add positions that are outside survey

    inline GeomSystem	geomSystem() const
			{ return geomsystem_; }
    inline bool		is2D() const
			{ return geomsystem_ == OD::LineBasedGeom; }
    inline bool		is3D() const
			{ return geomsystem_ == OD::VolBasedGeom; }
    inline bool		isSynthetic() const
			{ return geomsystem_ == OD::SynthGeom; }
    inline void		setGeomSystem( GeomSystem gs )
			{ geomsystem_ = gs; }
    inline void		setIs2D( bool yn )
			{ geomsystem_ = yn ? OD::LineBasedGeom
					   : OD::VolBasedGeom; }

protected:

    friend class	DataPointSet;
    friend class	PosVecDataSet;

    GeomSystem		geomsystem_;

    inline static BinID	mkBinID( const Pos::IdxPair& ip )
			{ return BinID( ip.first(), ip.second() ); }
    inline static Bin2D	mkBin2D( const Pos::IdxPair& ip )
			{ return Bin2D( GeomID(ip.first()), ip.second() ); }

public:

    mDeprecated void	allowDuplicateBinIDs( bool yn )
			{ allowDuplicatePositions( yn ); }

};
