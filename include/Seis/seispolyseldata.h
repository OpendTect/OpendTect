#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2007 / Feb 2019
________________________________________________________________________

-*/

#include "seisseldata.h"
template <class T> class ODPolygon;


namespace Seis
{

class PolySelDataPosIter;


/*!\brief selection data by polygon or polyline.

  The polygon provided is in inline crossline - in float it should still
  be inline crossline fractions.

  PolySelData works in 3D space; it does not select in trace numbers. In 2D,
  this will go via coordinates.

 */

mExpClass(Seis) PolySelData : public SelData
{
public:

    Type		type() const override	{ return Polygon; }

			PolySelData();
			PolySelData(const ODPolygon<float>&,
				    const z_steprg_type* zrange=0);
			PolySelData(const ODPolygon<int>&,
				    const z_steprg_type* zrange=0);
			PolySelData(const DBKey&);
			PolySelData(const PolySelData&);
			~PolySelData();
    PolySelData&	operator =( const PolySelData& tsd )
						{ copyFrom(tsd); return *this; }

    SelData*		clone() const	{ return new PolySelData(*this); }

    PosIter*		posIter() const override;
    pos_rg_type		inlRange() const override;
    pos_rg_type		crlRange() const override;
    z_steprg_type	zRange(idx_type i=0) const override;

    void		setZRange( const z_steprg_type& zrg,
					    idx_type i=0 ) override
					{ zrg_ = zrg; }
    void		merge(const PolySelData&);

    size_type		expectedNrTraces() const override;
    size_type		nrPolygons() const  { return polys_.size(); }

protected:

    void		initZrg(const z_steprg_type*);

    ObjectSet<ODPolygon<float> > polys_;
    z_steprg_type	zrg_;
    BufferString	polynm_;

    void		doCopyFrom(const SelData&) override;
    void		doFillPar(IOPar&) const override;
    void		doUsePar(const IOPar&,const SurveyInfo*) override;
    uiString		gtUsrSummary() const override;
    int			selRes3D(const BinID&) const override;

    friend class	PolySelDataPosIter;

};


/*!\brief SelDataPosIter for PolySelData */

mExpClass(Seis) PolySelDataPosIter : public SelDataPosIter
{
public:

			PolySelDataPosIter(const PolySelData&);
			PolySelDataPosIter(const PolySelDataPosIter&);

    SelDataPosIter*	clone() const override
			{ return new PolySelDataPosIter( *this ); }
    const PolySelData&	polySelData() const
			{ return static_cast<const PolySelData&>( sd_ ); }

    bool		next() override;
    void		reset() override	{ curbid_.setUdf(); }
    BinID		binID() const override	{ return curbid_; }

protected:

    BinID		firstbid_;
    BinID		lastbid_;
    BinID		curbid_;
    BinID		bidstep_;

};


} // namespace
