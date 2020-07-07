#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2018
________________________________________________________________________

-*/

#include "bin2d.h"
#include "posidxsubsel.h"
#include "survsubsel.h"
#include "manobjectset.h"
class CubeSubSel;
class HorSampling;
class LineSubSel;
namespace Survey { class Geometry; class Geometry2D; class Geometry3D; }
class TrcKeySampling;



mExpClass(Basic) LineHorSubSel : public Pos::IdxSubSel1D
			       , public Survey::HorSubSel
{
public:

    mUseType( Survey,		HorSubSel );
    mUseType( HorSubSel,	idx_type );
    mUseType( HorSubSel,	size_type );
    mUseType( HorSubSel,	pos_type );
    typedef Pos::IdxSubSelData	TrcNrSubSelData;

			LineHorSubSel(GeomID);
			LineHorSubSel(const Geometry2D&);
			LineHorSubSel(const pos_steprg_type&);
			LineHorSubSel(const TrcKeySampling&);
			LineHorSubSel(GeomID,trcnr_type);
			LineHorSubSel(const Bin2D&);
			LineHorSubSel(const LineSubSel&);
    bool		operator ==( const LineHorSubSel& oth ) const
			{ return equals( oth ); }
			mImplSimpleIneqOper(LineHorSubSel)
    bool		equals(const SubSel&) const override;

    bool		is2D() const override	   { return true; }
    GeomID		geomID() const override	   { return geomid_; }
    const Geometry2D&	geometry2D() const;
    totalsz_type	totalSize() const override { return ssdata_.size(); }
    bool		isAll() const override
			{ return IdxSubSel1D::isAll(); }
    bool		hasFullRange() const override
			{ return IdxSubSel1D::hasFullRange(); }
    void		clearSubSel()
			{ IdxSubSel1D::clearSubSel(); }

    void		setTrcNrRange( const pos_steprg_type& rg )
			{ setOutputPosRange( rg ); }
    pos_steprg_type	fullTrcNrRange() const
			{ return inputPosRange(); }
    idx_type		idx4TrcNr( trcnr_type trcnr ) const
			{ return idx4Pos( trcnr ); }
    trcnr_type		trcNr4Idx( idx_type idx ) const
			{ return pos4Idx( idx ); }
    size_type		nrTrcs() const		{ return ssdata_.size(); }

    bool		includes( trcnr_type tnr ) const
			{ return Pos::IdxSubSel1D::includes(tnr); }
    bool		includes( const LineHorSubSel& oth ) const
			{ return Pos::IdxSubSel1D::includes(oth); }
    bool		includes(const Bin2D&) const;
    void		merge(const LineHorSubSel&);
    void		limitTo(const LineHorSubSel&);
    bool		getIntersection(const LineHorSubSel&,
					LineHorSubSel&) const;
    void		normalise();
    void		addStepout( trcnr_type so )
			{ trcNrSubSel().addStepout(so); }

    void		setGeomID( GeomID gid )	{ geomid_ = gid; }

    static const LineHorSubSel&	empty();
    static LineHorSubSel&	dummy();

    totalsz_type	globIdx( trcnr_type tnr ) const
			{ return idx4TrcNr(tnr); }
    Bin2D		atGlobIdx( totalsz_type idx ) const
			{ return Bin2D( geomid_, (pos_type)trcNr4Idx(idx) ); }

protected:

    GeomID		geomid_;

			mImplArrRegSubSelClone(LineHorSubSel)

    bool		doUsePar(const IOPar&) override;
    void		doFillPar(IOPar&) const override;
    bool		doUseJSON(const OD::JSON::Object&) override;
    void		doFillJSON(OD::JSON::Object&) const override;

    IdxSubSelData&	gtTrcNrSubSel() const override
			{ return mNonConst(ssdata_); }

};


mExpClass(Basic) LineHorSubSelSet : public ManagedObjectSet<LineHorSubSel>
{
public:

    mUseType( Survey::SubSel,	totalsz_type );
    mUseType( Pos,		GeomID );
    mUseType( LineHorSubSel,	trcnr_type );


			LineHorSubSelSet()		{}
			LineHorSubSelSet(GeomID);
			LineHorSubSelSet(GeomID,trcnr_type);
    bool		operator ==(const LineHorSubSelSet&) const;
			mImplSimpleIneqOper(LineHorSubSelSet);
    bool		includes(const LineHorSubSelSet&) const;

    bool		isAll() const;
    totalsz_type	totalSize() const;
    bool		hasAllLines() const;
    bool		hasFullRange() const;
    void		merge(const LineHorSubSelSet&);
    void		limitTo(const LineHorSubSelSet&);
    void		addStepout(trcnr_type);
    void		setToAll();
    void		clearSubSel();

    LineHorSubSel*	find( GeomID gid )	{ return doFind( gid ); }
    const LineHorSubSel* find( GeomID gid ) const { return doFind( gid ); }

    totalsz_type	globIdx(const Bin2D&) const;
    Bin2D		atGlobIdx(totalsz_type) const;

protected:

    LineHorSubSel*	doFind(GeomID) const;

};



/*!\brief Subselection of an existing inline/crossline range. Directly usable
  for array subselection. */


mExpClass(Basic) CubeHorSubSel : public Pos::IdxSubSel2D
			       , public Survey::HorSubSel
{
public:

    mUseType( Survey,		HorSubSel );
    mUseType( Survey,		Geometry );
    mUseType( Survey,		Geometry3D );
    mUseType( HorSubSel,	idx_type );
    mUseType( HorSubSel,	size_type );
    mUseType( HorSubSel,	pos_type );
    mUseType( Pos,		IdxSubSelData );

			CubeHorSubSel(OD::SurvLimitType slt=OD::FullSurvey);
			CubeHorSubSel(const SurveyInfo*);
			CubeHorSubSel(const Geometry3D&);
			CubeHorSubSel(const HorSampling&);
			CubeHorSubSel(const pos_steprg_type&,
				      const pos_steprg_type&);
			CubeHorSubSel(const BinID&);
			CubeHorSubSel(const CubeSubSel&);
			CubeHorSubSel(const TrcKeySampling&);
			CubeHorSubSel(const CubeHorSubSel&,int nrchunks,
							   int chunknr);
			CubeHorSubSel(const CubeHorSubSel&);
    bool		operator ==( const CubeHorSubSel& oth ) const
			{ return equals( oth ); }
			mImplSimpleIneqOper(CubeHorSubSel)
    bool		equals(const SubSel&) const override;
    bool		includes( const CubeHorSubSel& oth ) const
			{ return Pos::IdxSubSel2D::includes( oth ); }
    bool		includes( const BinID& bid ) const
			{ return Pos::IdxSubSel2D::includes( bid ); }

    bool		is2D() const override	   { return false; }
    GeomID		geomID() const override;
    totalsz_type	totalSize() const override;
    bool		isAll() const override
			{ return inlSubSel().isAll() && crlSubSel().isAll(); }
    bool		hasFullRange() const override
			{ return inlSubSel().hasFullRange()
			      && crlSubSel().hasFullRange(); }
    void		clearSubSel()
			{ IdxSubSel2D::clearSubSel(); }

    const IdxSubSelData& inlSubSel() const	{ return ssdata0_; }
    IdxSubSelData&	inlSubSel()		{ return ssdata0_; }
    const IdxSubSelData& crlSubSel() const	{ return ssdata1_; }
    IdxSubSelData&	crlSubSel()		{ return ssdata1_; }

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

    void		merge(const CubeHorSubSel&);
    void		limitTo(const CubeHorSubSel&);
    bool		getIntersection(const CubeHorSubSel&,
					CubeHorSubSel&) const;
    void		normalise();
    void		addStepout(pos_type,pos_type);

    pos_type		inlStart() const { return inlRange().start; }
    pos_type		inlStop() const	{ return inlRange().stop; }
    pos_type		inlStep() const	{ return inlRange().step; }
    pos_type		crlStart() const { return crlRange().start; }
    pos_type		crlStop() const	{ return crlRange().stop; }
    pos_type		crlStep() const	{ return crlRange().step; }
    size_type		nrInl() const	{ return inlRange().nrSteps()+1; }
    size_type		nrCrl() const	{ return crlRange().nrSteps()+1; }
    size_type		nrLines() const	override { return nrInl(); }

    totalsz_type	globIdx( const BinID& bid ) const
			{
			    return nrCrl() * idx4Inl(bid.inl())
				 + idx4Crl(bid.crl());
			}
    BinID		atGlobIdx( totalsz_type gidx ) const
			{
			    const auto nrcrl = nrCrl();
			    return BinID( inl4Idx((pos_type)(gidx/nrcrl)),
					  crl4Idx((pos_type)(gidx%nrcrl)) );
			}

protected:

			mImplArrRegSubSelClone(CubeHorSubSel)

    bool		doUsePar(const IOPar&) override;
    void		doFillPar(IOPar&) const override;
    bool		doUseJSON(const OD::JSON::Object&) override;
    void		doFillJSON(OD::JSON::Object&) const override;

    IdxSubSelData&	gtTrcNrSubSel() const override
			{ return mNonConst(ssdata1_); }

};
