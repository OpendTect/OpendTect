#ifndef seisselectionimpl_h
#define seisselectionimpl_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Nov 2007
 RCS:		$Id: seisselectionimpl.h,v 1.1 2007-11-23 11:59:06 cvsbert Exp $
________________________________________________________________________

-*/

#include "seisselection.h"
class HorSampling;
class CubeSampling;
class BinIDValueSet;
template <class T> class ODPolygon;


namespace Seis
{

/*!\brief selection data in simple ranges */

class RangeSelData : public SelData
{
public:

    Type		type() const		{ return Range; }

			RangeSelData(bool settosurvinfo=false);
			RangeSelData(const HorSampling&);
			RangeSelData(const CubeSampling&);
			RangeSelData(const RangeSelData&);
			~RangeSelData();
    RangeSelData&	operator =( const RangeSelData& rsd )
    						{ copyFrom(rsd); return *this; }

    CubeSampling&	cubeSampling() 		{ return cs_; }
    const CubeSampling&	cubeSampling() const	{ return cs_; }

    SelData*		clone() const	{ return new RangeSelData(*this); }
    virtual void	copyFrom(const SelData&);

    Interval<int>	inlRange() const;
    Interval<int>	crlRange() const;
    Interval<float>	zRange() const;
    bool		setInlRange(Interval<int>);
    bool		setCrlRange(Interval<int>);
    bool		setZRange(Interval<float>);

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    void		extendZ(const Interval<float>&);
    void		include(const SelData&);

    int			selRes(const BinID&) const;
    int			expectedNrTraces(bool for2d,const BinID*) const;

protected:

    CubeSampling&	cs_;

    void		doExtendH(BinID,BinID);
};


/*!\brief selection data in a table */

class TableSelData : public SelData
{
public:

    Type		type() const		{ return Table; }

			TableSelData();
			TableSelData(const BinIDValueSet&,
				     const Interval<float>* extraz=0);
			TableSelData(const TableSelData&);
			~TableSelData();
    TableSelData&	operator =( const TableSelData& tsd )
    						{ copyFrom(tsd); return *this; }

    BinIDValueSet&	binidValueSet() 	{ return bvs_; }
    const BinIDValueSet& binidValueSet() const	{ return bvs_; }
    Interval<float>	extraZ() const		{ return extraz_; }

    SelData*		clone() const	{ return new TableSelData(*this); }
    virtual void	copyFrom(const SelData&);

    Interval<int>	inlRange() const;
    bool		setInlRange(Interval<int>)	{ return false; }
    Interval<int>	crlRange() const;
    bool		setCrlRange(Interval<int>)	{ return false; }
    Interval<float>	zRange() const;
    bool		setZRange(Interval<float>)	{ return false; }

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    void		extendZ(const Interval<float>&);
    void		include(const SelData&);

    int			selRes(const BinID&) const;
    int			expectedNrTraces(bool for2d,const BinID*) const;

protected:

    BinIDValueSet&	bvs_;
    Interval<float>	extraz_;

    void		doExtendH(BinID,BinID);
};


/*!\brief selection data by polygon or polyline.

  The polygon provided is in inline crossline - in float it should still
  be inline crossline fractions.

 */

class PolySelData : public SelData
{
public:

    Type		type() const		{ return Polygon; }

			PolySelData();
			PolySelData(const ODPolygon<float>&,
				    const Interval<float>* zrange=0);
			PolySelData(const ODPolygon<int>&,
				    const Interval<float>* zrange=0);
			PolySelData(const PolySelData&);
			~PolySelData();
    PolySelData&	operator =( const PolySelData& tsd )
    						{ copyFrom(tsd); return *this; }

    SelData*		clone() const	{ return new PolySelData(*this); }
    virtual void	copyFrom(const SelData&);

    Interval<int>	inlRange() const;
    bool		setInlRange(Interval<int>)	{ return false; }
    Interval<int>	crlRange() const;
    bool		setCrlRange(Interval<int>)	{ return false; }
    Interval<float>	zRange() const;
    bool		setZRange( Interval<float> zrg )
    			{ zrg_ = zrg; return true; }

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    void		extendZ(const Interval<float>&);
    void		include(const SelData&);

    int			selRes(const BinID&) const;
    int			expectedNrTraces(bool for2d,const BinID*) const;

protected:

    void		initZrg(const Interval<float>*);

    ODPolygon<float>&	poly_;
    float		midz_;
    Interval<float>	zrg_;

    void		doExtendH(BinID,BinID);
};


} // namespace

#endif
