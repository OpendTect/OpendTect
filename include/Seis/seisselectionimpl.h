#ifndef seisselectionimpl_h
#define seisselectionimpl_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2007
 RCS:		$Id: seisselectionimpl.h,v 1.5 2011/03/01 10:21:40 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "seisselection.h"
#include "position.h"

class HorSampling;
class CubeSampling;
class BinIDValueSet;
template <class T> class ODPolygon;


namespace Seis
{

/*!\brief selection data in simple ranges */

mClass RangeSelData : public SelData
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

mClass TableSelData : public SelData
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
    bool		setZRange(Interval<float>);

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    void		extendZ(const Interval<float>&);
    void		include(const SelData&);

    int			selRes(const BinID&) const;
    int			expectedNrTraces(bool for2d,const BinID*) const;

protected:

    BinIDValueSet&	bvs_;
    Interval<float>	extraz_;
    Interval<float>	fixedzrange_; // used only if no z vals in bidvalset

    void		doExtendH(BinID,BinID);
};


/*!\brief selection data by polygon or polyline.

  The polygon provided is in inline crossline - in float it should still
  be inline crossline fractions.

 */

mClass PolySelData : public SelData
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

    ObjectSet<ODPolygon<float> > polys_;

    float		midz_;
    Interval<float>	zrg_;

    BinID		stepoutreach_;

    void		doExtendH(BinID,BinID);
};


} // namespace

#endif
