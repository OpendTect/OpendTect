#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2007
________________________________________________________________________

-*/

#include "seismod.h"
#include "seisselection.h"
#include "binid.h"
#include "objectset.h"

class TrcKeySampling;
class TrcKeyZSampling;
class BinIDValueSet;
template <class T> class ODPolygon;


namespace Seis
{

/*!\brief selection data in simple ranges */

mExpClass(Seis) RangeSelData : public SelData
{
public:

    Type		type() const override		{ return Range; }

			RangeSelData(bool settosurvinfo=false);
			RangeSelData(const TrcKeySampling&);
			RangeSelData(const TrcKeyZSampling&);
			RangeSelData(const RangeSelData&);
			~RangeSelData();
    RangeSelData&	operator =( const RangeSelData& rsd )
						{ copyFrom(rsd); return *this; }

    TrcKeyZSampling&	cubeSampling()		{ return tkzs_; }
    const TrcKeyZSampling& cubeSampling() const	{ return tkzs_; }

    SelData*		clone() const override
			{ return new RangeSelData(*this); }

    void		copyFrom(const SelData&) override;

    Interval<int>	inlRange() const override;
    Interval<int>	crlRange() const override;
    Interval<float>	zRange() const override;
    bool		setInlRange(const Interval<int>&) override;
    bool		setCrlRange(const Interval<int>&) override;
    bool		setZRange(const Interval<float>&) override;

    void		fillPar(IOPar&) const override;
    void		usePar(const IOPar&) override;

    void		extendZ(const Interval<float>&) override;
    void		include(const SelData&) override;

    int			expectedNrTraces(bool for2d,
					 const BinID*) const override;
    void		setGeomID(Pos::GeomID) override;

protected:

    TrcKeyZSampling&	tkzs_;

    void		doExtendH(BinID,BinID) override;
    int			selRes3D(const BinID&) const override;
    int			selRes2D(Pos::GeomID,int trcnr) const override;
    void		testIsAll2D();
};


/*!\brief selection data in a table */

mExpClass(Seis) TableSelData : public SelData
{
public:

    Type		type() const override		{ return Table; }

			TableSelData();
			TableSelData(const BinIDValueSet&,
				     const Interval<float>* extraz=0);
			TableSelData(const TableSelData&);
			~TableSelData();
    TableSelData&	operator =( const TableSelData& tsd )
						{ copyFrom(tsd); return *this; }

    BinIDValueSet&	binidValueSet()	{ return bvs_; }
    const BinIDValueSet& binidValueSet() const	{ return bvs_; }
    Interval<float>	extraZ() const		{ return extraz_; }

    SelData*		clone() const override
			{ return new TableSelData(*this); }

    void		copyFrom(const SelData&) override;

    Interval<int>	inlRange() const override;
    Interval<int>	crlRange() const override;
    Interval<float>	zRange() const override;
    bool		setZRange(const Interval<float>&) override;

    void		fillPar(IOPar&) const override;
    void		usePar(const IOPar&) override;

    void		extendZ(const Interval<float>&) override;
    void		include(const SelData&) override;

    int			expectedNrTraces(bool for2d,
					 const BinID*) const override;

protected:

    BinIDValueSet&	bvs_;
    Interval<float>	extraz_;
    Interval<float>	fixedzrange_; // used only if no z vals in bidvalset

    void		doExtendH(BinID,BinID) override;
    int			selRes3D(const BinID&) const override;
    int			selRes2D(Pos::GeomID,int trcnr) const override;
			//<! false (not implemented)
};


/*!\brief selection data by polygon or polyline.

  The polygon provided is in inline crossline - in float it should still
  be inline crossline fractions.

 */

mExpClass(Seis) PolySelData : public SelData
{
public:

    Type		type() const override		{ return Polygon; }

			PolySelData();
			PolySelData(const ODPolygon<float>&,
				    const Interval<float>* zrange=0);
			PolySelData(const ODPolygon<int>&,
				    const Interval<float>* zrange=0);
			PolySelData(const PolySelData&);
			~PolySelData();
    PolySelData&	operator =( const PolySelData& tsd )
						{ copyFrom(tsd); return *this; }

    SelData*		clone() const override
			{ return new PolySelData(*this); }
    void		copyFrom(const SelData&) override;

    Interval<int>	inlRange() const override;
    Interval<int>	crlRange() const override;
    Interval<float>	zRange() const override;
    bool		setZRange( const Interval<float>& zrg ) override
			{ zrg_ = zrg; return true; }

    void		fillPar(IOPar&) const override;
    void		usePar(const IOPar&) override;

    void		extendZ(const Interval<float>&) override;
    void		include(const SelData&) override;

    int			expectedNrTraces(bool for2d,
					 const BinID*) const override;

protected:

    void		initZrg(const Interval<float>*);

    ObjectSet<ODPolygon<float> > polys_;

    float		midz_;
    Interval<float>	zrg_;

    BinID		stepoutreach_;

    void		doExtendH(BinID,BinID) override;
    int			selRes3D(const BinID&) const override;
};


} // namespace Seis
