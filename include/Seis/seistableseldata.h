#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2007 / Feb 2019
________________________________________________________________________

-*/

#include "seisseldata.h"
#include "binnedvalueset.h"


namespace Seis
{

class TableSelDataPosIter;

/*!\brief Selection data from a BinnedValueSet.

  TableSelData works in 3D space; it does not select in trace numbers. In 2D,
  this will go via coordinates.
 */

mExpClass(Seis) TableSelData : public SelData
{
public:

    typedef Pos::Distance_Type	dist_type;


    Type		type() const override	{ return Table; }

			TableSelData();
			TableSelData(const BinnedValueSet&);
			TableSelData(const DBKey&);
			TableSelData(const TableSelData&);
			~TableSelData();
    TableSelData&	operator =( const TableSelData& oth )
						{ copyFrom(oth); return *this; }
    SelData*		clone() const	{ return new TableSelData(*this); }
    bool		is2D() const override	{ return bvs_.is2D(); }
    size_type		nrGeomIDs() const override;

    void		updateAfterBVSChange();
    BinnedValueSet&	binidValueSet()		{ return bvs_; }
    const BinnedValueSet& binidValueSet() const	{ return bvs_; }
    dist_type		searchRadius() const	{ return searchradius_; }
    void		merge(const TableSelData&);

    PosIter*		posIter() const override;
    pos_rg_type		inlRange() const override;
    pos_rg_type		crlRange() const override;
    z_steprg_type	zRange(idx_type i=0) const override;

    void		setSearchRadius( dist_type r ) { searchradius_ = r; }
    void		setZRange( const z_steprg_type& zrg,
						idx_type i=0 ) override
						{ fixedzrange_ = zrg; }

    size_type		expectedNrTraces() const override;

protected:

    BinnedValueSet&	bvs_;
    z_steprg_type	fixedzrange_; // used only if no z vals in bidvalset
    dist_type		searchradius_		= 0;

    GeomID		gtGeomID(idx_type) const override;
    void		doCopyFrom(const SelData&) override;
    void		doFillPar(IOPar&) const override;
    void		doUsePar(const IOPar&,const SurveyInfo*) override;
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

    BinnedValueSet::SPos spos_;

};


} // namespace
