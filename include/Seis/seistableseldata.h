#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2007 / Feb 2019
________________________________________________________________________

-*/

#include "seisseldata.h"
#include "binidvalset.h"


namespace Seis
{

class TableSelDataPosIter;

/*!\brief Selection data from a BinIDValueSet.

  TableSelData works in 3D space; it does not select in trace numbers. In 2D,
  this will go via coordinates.
 */

mExpClass(Seis) TableSelData : public SelData
{
public:

    typedef Pos::Distance_Type	dist_type;


    Type		type() const override	{ return Table; }

			TableSelData();
			TableSelData(const BinIDValueSet&,
				     const z_rg_type* extraz=0);
			TableSelData(const DBKey&);
			TableSelData(const TableSelData&);
			~TableSelData();
    TableSelData&	operator =( const TableSelData& oth )
						{ copyFrom(oth); return *this; }
    SelData*		clone() const	{ return new TableSelData(*this); }
    bool		is2D() const override	{ return bvs_.is2D(); }

    void		updateAfterBVSChange();
    BinIDValueSet&	binidValueSet()		{ return bvs_; }
    const BinIDValueSet& binidValueSet() const	{ return bvs_; }
    z_rg_type		extraZ() const		{ return extraz_; }
    dist_type		searchRadius() const	{ return searchradius_; }
    void		merge(const TableSelData&);

    PosIter*		posIter() const override;
    pos_rg_type		inlRange() const override;
    pos_rg_type		crlRange() const override;
    z_rg_type		zRange(idx_type i=0) const override;

    void		setSearchRadius( dist_type r ) { searchradius_ = r; }
    void		setExtraZ( z_rg_type zr ) { extraz_ = zr; }
    void		setZRange( const z_rg_type& zrg, idx_type i=0 ) override
						{ fixedzrange_ = zrg; }

    size_type		expectedNrTraces() const override;

protected:

    BinIDValueSet&	bvs_;
    z_rg_type		extraz_;
    z_rg_type		fixedzrange_; // used only if no z vals in bidvalset
    dist_type		searchradius_		= 0;

    void		doCopyFrom(const SelData&) override;
    void		doExtendH(BinID,BinID) override;
    void		doExtendZ(const z_rg_type&) override;
    void		doFillPar(IOPar&) const override;
    void		doUsePar(const IOPar&) override;
    uiString		gtUsrSummary() const override;
    int			selRes2D(GeomID,pos_type) const override;
    int			selRes3D(const BinID&) const override;

    friend class	TableSelDataPosIter;

};


/*!\brief SelDataPosIter for TableSelData */

mExpClass(Seis) TableSelDataPosIter : public SelDataPosIter
{
public:

    mUseType( TableSelData,	idx_type );

			TableSelDataPosIter(const TableSelData&);
			TableSelDataPosIter(const TableSelDataPosIter&);

    SelDataPosIter*	clone() const override
			{ return new TableSelDataPosIter( *this ); }
    const TableSelData&	tableSelData() const
			{ return static_cast<const TableSelData&>( sd_ ); }

    bool		next() override
			{ return tableSelData().bvs_.next(spos_,true); }
    void		reset() override    { spos_.reset(); }
    BinID		binID() const override;

protected:

    BinIDValueSet::SPos	spos_;

};


} // namespace
