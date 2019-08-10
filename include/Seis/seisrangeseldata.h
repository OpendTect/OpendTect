#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2007 / Feb 2019
________________________________________________________________________

-*/

#include "seisseldata.h"
#include "survsubsel.h"

namespace Seis
{

class RangeSelDataPosIter;

/*!\brief Selection data based on survey subselection. Note that for 2D there
  is a range of trace numbers per GeomID and the inline range is meaningless. */

mExpClass(Seis) RangeSelData : public SelData
{
public:

    mUseType( Pos,	ZSubSel );
    mUseType( Survey,	GeomSubSel );
    mUseType( Survey,	FullSubSel );

			RangeSelData()		    {}
			RangeSelData(GeomID);
			RangeSelData(const GeomIDSet&);
			RangeSelData(const CubeSubSel&);
			RangeSelData(const LineSubSel&);
			RangeSelData(const GeomSubSel&);
			RangeSelData(const FullSubSel&);
			RangeSelData(const CubeHorSubSel&);
			RangeSelData(const LineHorSubSel&);
			RangeSelData(const LineSubSelSet&);
			RangeSelData(const LineHorSubSelSet&);
			RangeSelData(const BinID&);
			RangeSelData(GeomID,trcnr_type);
			RangeSelData(const TrcKey&);
			RangeSelData(const TrcKeySampling&);
			RangeSelData(const TrcKeyZSampling&);
			RangeSelData(const RangeSelData&);
			RangeSelData(const IOPar&);
			~RangeSelData();
    RangeSelData&	operator =( const RangeSelData& rsd )
					{ copyFrom(rsd); return *this; }
    SelData*		clone() const override
					{ return new RangeSelData(*this); }

    Type		type() const override	{ return Range; }
    bool		is2D() const override	{ return fss_.is2D(); }

    PosIter*		posIter() const override;
    pos_rg_type		inlRange() const override;
    pos_rg_type		crlRange() const override;
    pos_rg_type		trcNrRange(idx_type iln=0) const override;
    z_rg_type		zRange(idx_type i=0) const override;

    void		setInlRange(const pos_rg_type&);
    void		setCrlRange(const pos_rg_type&);
    void		setGeomID(GeomID);
    void		addGeomID(GeomID);
    idx_type		indexOf(GeomID) const;
    void		setTrcNrRange(const pos_rg_type&,idx_type i=0);
    void		setTrcNrRange(GeomID,const pos_rg_type&);
    void		setZRange(const z_rg_type&,int i=0) override;

    bool		isAll() const override;
    void		setToAll();
    void		setForceIsAll( bool yn=true )
						{ forceall_ = true; }

    size_type		expectedNrTraces() const override;
    FullSubSel&		fullSubSel()		{ return fss_; }
    const FullSubSel&	fullSubSel() const	{ return fss_; }
    GeomSubSel&		geomSubSel(idx_type i=0);
    const GeomSubSel&	geomSubSel(idx_type i=0) const;
    CubeSubSel&		cubeSubSel()		{ return fss_.cubeSubSel();}
    const CubeSubSel&	cubeSubSel() const	{ return fss_.cubeSubSel();}
    LineSubSel&		lineSubSel(idx_type);
    const LineSubSel&	lineSubSel(idx_type) const;
    CubeSubSel&		subSel3D()		{ return fss_.subSel3D(); }
    const CubeSubSel&	subSel3D() const	{ return fss_.subSel3D(); }
    LineSubSelSet&	subSel2D()		{ return fss_.subSel2D(); }
    const LineSubSelSet& subSel2D() const	{ return fss_.subSel2D(); }
    ZSubSel&		zSubSel()		{ return fss_.zSubSel(); }
    const ZSubSel&	zSubSel() const		{ return fss_.zSubSel(); }
    bool		hasFullZRange() const;
    const LineSubSel*	findLineSubSel(GeomID) const;
    void		merge(const RangeSelData&);

    void		set(const CubeSubSel&);
    void		set(const LineSubSel&);
    void		set(const LineSubSelSet&);

protected:

    bool		forceall_	= false;
    FullSubSel		fss_;

    void		clearContents();
    void		set3D(bool yn=true);

    GeomID		gtGeomID(idx_type) const override;
    void		doCopyFrom(const SelData&) override;
    void		doFillPar(IOPar&) const override;
    void		doUsePar(const IOPar&) override;
    uiString		gtUsrSummary() const override;
    int			selRes3D(const BinID&) const override;
    int			selRes2D(GeomID,trcnr_type) const override;

    friend class	RangeSelDataPosIter;

};


/*!\brief SelDataPosIter for RangeSelData */

mExpClass(Seis) RangeSelDataPosIter : public SelDataPosIter
{
public:

    mUseType( RangeSelData,	idx_type );

			RangeSelDataPosIter(const RangeSelData&);
			RangeSelDataPosIter(const RangeSelDataPosIter&);

    SelDataPosIter*	clone() const override
			{ return new RangeSelDataPosIter( *this ); }
    const RangeSelData&	rangeSelData() const
			{ return static_cast<const RangeSelData&>( sd_ ); }

    bool		next() override		{ return iter_.next(); }
    void		reset() override	{ iter_.reset(); }
    bool		is2D() const override	{ return iter_.is2D(); }

    GeomID		geomID() const override	{ return iter_.geomID(); }
    trcnr_type		trcNr() const override	{ return iter_.trcNr(); }
    BinID		binID() const override	{ return iter_.binID(); }

protected:

    Survey::FullSubSelPosIter	iter_;

};


} // namespace
