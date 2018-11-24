#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2018
________________________________________________________________________

-*/

#include "basicmod.h"
#include "posidxsubsel.h"
#include "survsubsel.h"
class HorSampling;
namespace Survey { class Geometry; class Geometry2D; class Geometry3D; }
class TrcKeySampling;



mExpClass(Basic) LineHorSubSel : public Pos::IdxSubSel1D
			       , public Survey::HorSubSel
{
public:

    typedef Pos::IdxSubSelData	TrcNrSubSelData;

			LineHorSubSel(GeomID);
			LineHorSubSel(const Geometry2D&);
			LineHorSubSel(const pos_steprg_type&);
			LineHorSubSel(const TrcKeySampling&);

    bool		is2D() const override	   { return true; }
    GeomID		geomID() const override	   { return geomid_; }
    totalsz_type	totalSize() const override { return data_.size(); }
    bool		isAll() const override
			{ return IdxSubSel1D::isAll(); }
    bool		hasFullRange() const override
			{ return IdxSubSel1D::hasFullRange(); }

    const TrcNrSubSelData& trcNrSubSel() const	{ return data_; }
    TrcNrSubSelData&	trcNrSubSel()		{ return data_; }
    pos_steprg_type	trcNrRange() const
			{ return outputPosRange(); }
    void		setTrcNrRange( const pos_steprg_type& rg )
			{ setOutputPosRange( rg ); }
    pos_steprg_type	fullTrcNrRange() const
			{ return inputPosRange(); }
    idx_type		idx4TrcNr( pos_type trcnr ) const
			{ return idx4Pos( trcnr ); }
    pos_type		trcNr4Idx( idx_type idx ) const
			{ return pos4Idx( idx ); }
    size_type		nrTrcs() const		{ return data_.size(); }

    bool		includes( pos_type pos ) const
			{ return Pos::IdxSubSel1D::includes(pos); }
    bool		includes(const LineHorSubSel&) const;

    void		setGeomID( GeomID gid )	{ geomid_ = gid; }

protected:

    GeomID	geomid_;

};


/*!\brief Subselection of an existing inline/crossline range. Directly usable
  for array subselection. */


mExpClass(Basic) CubeHorSubSel : public Pos::IdxSubSel2D
			       , public Survey::HorSubSel
{
public:

    mUseType( Survey,	Geometry );
    mUseType( Survey,	Geometry3D );
    mUseType( Pos,	IdxSubSelData );

			CubeHorSubSel(OD::SurvLimitType slt=OD::FullSurvey);
			CubeHorSubSel(const Geometry3D&);
			CubeHorSubSel(const HorSampling&);
			CubeHorSubSel(const pos_steprg_type&,
				      const pos_steprg_type&);
			CubeHorSubSel(const TrcKeySampling&);

    bool		is2D() const override	   { return false; }
    GeomID		geomID() const override;
    totalsz_type	totalSize() const override;
    bool		isAll() const override
			{ return inlSubSel().isAll() && crlSubSel().isAll(); }
    bool		hasFullRange() const override
			{ return inlSubSel().hasFullRange()
			      && crlSubSel().hasFullRange(); }

    const IdxSubSelData& inlSubSel() const	{ return posData(0); }
    IdxSubSelData&	inlSubSel()		{ return posData(0); }
    const IdxSubSelData& crlSubSel() const	{ return posData(1); }
    IdxSubSelData&	crlSubSel()		{ return posData(1); }

    RowCol		arraySize() const
			{ return RowCol(inlSubSel().size(),crlSubSel().size());}

    pos_steprg_type	inlRange() const
			{ return inlSubSel().outputPosRange(); }
    pos_steprg_type	crlRange() const
			{ return crlSubSel().outputPosRange(); }
    void		setInlRange( const pos_steprg_type& rg )
			{ inlSubSel().setOutputPosRange( rg ); }
    void		setCrlRange( const pos_steprg_type& rg )
			{ crlSubSel().setOutputPosRange( rg ); }

    pos_steprg_type	fullInlRange() const
			{ return inlSubSel().inputPosRange(); }
    pos_steprg_type	fullCrlRange() const
			{ return crlSubSel().inputPosRange(); }

    pos_type		inl4Idx( idx_type idx ) const
			{ return inlSubSel().pos4Idx( idx ); }
    pos_type		crl4Idx( idx_type idx ) const
			{ return crlSubSel().pos4Idx( idx ); }
    pos_type		idx4Inl( pos_type inl ) const
			{ return inlSubSel().idx4Pos( inl ); }
    pos_type		idx4Crl( pos_type crl ) const
			{ return crlSubSel().idx4Pos( crl ); }
    BinID		binID4RowCol( const RowCol& rc ) const
			{ return BinID( inlSubSel().pos4Idx(rc.row()),
					crlSubSel().pos4Idx(rc.col()) ); }
    RowCol		rowCol4BinID( const BinID& bid ) const
			{ return BinID( inlSubSel().idx4Pos(bid.inl()),
					crlSubSel().idx4Pos(bid.crl()) ); }

    bool		includes( const BinID& bid ) const
			{ return Pos::IdxSubSel2D::includes( bid ); }
    bool		includes(const CubeHorSubSel&) const;

    pos_type		inlStart() const { return inlRange().start; }
    pos_type		inlStop() const	{ return inlRange().stop; }
    pos_type		inlStep() const	{ return inlRange().step; }
    pos_type		crlStart() const { return crlRange().start; }
    pos_type		crlStop() const	{ return crlRange().stop; }
    pos_type		crlStep() const	{ return crlRange().step; }
    size_type		nrInl() const	{ return inlRange().nrSteps()+1; }
    size_type		nrCrl() const	{ return crlRange().nrSteps()+1; }

    void		limitTo(const CubeHorSubSel&);

    BinID		atGlobIdx( od_int64 gidx ) const
			{
			    const auto nrinl = nrInl();
			    return BinID( inl4Idx(gidx/nrinl),
					  crl4Idx(gidx%nrinl) );
			}

};
