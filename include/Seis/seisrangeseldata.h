#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2007 / Feb 2019
________________________________________________________________________

-*/

#include "seisseldata.h"

class CubeHorSubSel;
class CubeSubSel;
class LineHorSubSel;
class LineHorSubSelSet;
class LineSubSel;
class LineSubSelSet;
class TrcKeySampling;
class TrcKeyZSampling;
namespace Survey { class FullSubSel; }


namespace Seis
{

class RangeSelDataPosIter;

/*!\brief Selection data based on survey subselection. Note that for 2D there
  is a range of trace numbers per GeomID and the inline range is meaningless. */

mExpClass(Seis) RangeSelData : public SelData
{
public:

    mUseType( Survey,	FullSubSel );

			RangeSelData();
			RangeSelData(GeomID);
			RangeSelData(const GeomIDSet&);
			RangeSelData(const CubeSubSel&);
			RangeSelData(const LineSubSel&);
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
    bool		is2D() const override	{ return !css_; }

    PosIter*		posIter() const override;
    pos_rg_type		inlRange() const override;
    pos_rg_type		crlRange() const override;
    pos_rg_type		trcNrRange(idx_type iln=0) const override;
    z_rg_type		zRange(idx_type i=0) const override;

    void		setInlRange(const pos_rg_type&);
    void		setCrlRange(const pos_rg_type&);
    void		setGeomID(GeomID);
    void		addGeomID(GeomID);
    void		setTrcNrRange(const pos_rg_type&,idx_type i=0);
    void		setTrcNrRange(GeomID,const pos_rg_type&);
    void		setZRange(const z_rg_type&,int i=0) override;

    bool		isAll() const override;
    void		setForceIsAll( bool yn=true )
						{ forceall_ = true; }
    size_type		expectedNrTraces() const override;
    const FullSubSel&	subSel(idx_type i=0) const;
    const CubeSubSel&	cubeSubSel() const	{ return *css_;}
    const LineSubSel&	lineSubSel(idx_type) const;
    CubeSubSel&		subSel3D()		{ return *css_; }
    const CubeSubSel&	subSel3D() const	{ return *css_; }
    LineSubSelSet&	subSel2D()		{ return lsss_; }
    const LineSubSelSet& subSel2D() const	{ return lsss_; }
    bool		hasFullZRange() const;
    const LineSubSel*	findLineSubSel(GeomID) const;
    void		merge(const RangeSelData&);

    void		set(const CubeSubSel&);
    void		set(const LineSubSel&);
    void		set(const LineSubSelSet&);

protected:

    bool		forceall_	= false;
    CubeSubSel*		css_		= 0;
    LineSubSelSet&	lsss_;

    idx_type		indexOf(GeomID) const;
    void		clearContents();
    void		set3D(bool yn=true);

    GeomID		gtGeomID(idx_type) const override;
    void		doCopyFrom(const SelData&) override;
    void		doExtendH(BinID,BinID) override;
    void		doExtendZ(const z_rg_type&) override;
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

    bool		next() override;
    void		reset() override    { lineidx_ = trcidx_ = -1; }
    bool		is2D() const override { return rangeSelData().is2D(); }

    GeomID		geomID() const override;
    trcnr_type		trcNr() const override;
    BinID		binID() const override;

protected:

    idx_type		lineidx_	    = -1;
    idx_type		trcidx_		    = -1;

};


} // namespace
