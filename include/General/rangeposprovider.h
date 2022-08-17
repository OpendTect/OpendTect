#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Feb 2008
________________________________________________________________________


-*/

#include "generalmod.h"
#include "posprovider.h"
#include "transl.h"
#include "sortedlist.h"
#include <tuple>

namespace PosInfo { class Line2DData; }
namespace Stats { class RandGen; }

mDeclEmptyTranslatorBundle(General,PosProviders,dgb,"subsel")

namespace Pos
{

/*!\brief 3D provider based on TrcKeyZSampling */

typedef std::tuple<od_int64,int> postuple;

mExpClass(General) RangeProvider3D : public Provider3D
{
public:

			RangeProvider3D();
			RangeProvider3D(const RangeProvider3D&);
			~RangeProvider3D();
    RangeProvider3D&	operator =(const RangeProvider3D&);
    const char*		type() const override;	//!< sKey::Range()
    const char*		factoryKeyword() const override { return type(); }
    Provider*		clone() const override
			{ return new RangeProvider3D(*this); }

    bool		initialize(TaskRunner* tr=0) override;
    void		reset() override;

    bool		toNextPos() override;
    bool		toNextZ() override;

    BinID		curBinID() const override	{ return curbid_; }
    float		curZ() const override;
    bool		includes(const BinID&,
				 float z=mUdf(float)) const override;
    void		usePar(const IOPar&) override;
    void		fillPar(IOPar&) const override;
    void		getSummary(BufferString&) const override;

    void		getExtent(BinID& start,BinID& stop) const override;
    void		getZRange(Interval<float>&) const override;
    od_int64		estNrPos() const override;
    int			estNrZPerPos() const override;
    void		getTrcKeyZSampling(TrcKeyZSampling&) const override;

    const TrcKeyZSampling&	sampling() const	{ return tkzs_; }
    void		setSampling( const TrcKeyZSampling& tkzs );
    void		setHSampling( const TrcKeySampling& tks ) const
						{ tkzs_.hsamp_ = tks; }

    bool		includes( const Coord& c,
				  float z=mUdf(float) ) const override
			{ return Pos::Provider3D::includes(c,z); }

protected:

    TrcKeyZSampling&	tkzs_;
    BinID		curbid_;
    int			curzidx_;
    int			zsampsz_ = 0;
    int			nrsamples_;
    bool		dorandom_ = false;
    bool		enoughsamples_ = true;
    SortedList<postuple> posindexlst_;
    Stats::RandGen&	gen_;

public:

    static void		initClass();
    static Provider3D*	create()	{ return new RangeProvider3D; }

};


/*!\brief 2D provider based on StepInterval<int>.

Can only be used if Line2DData is filled.

 */

mExpClass(General) RangeProvider2D : public Provider2D
{
public:
			RangeProvider2D();
			RangeProvider2D(const RangeProvider2D&);
			~RangeProvider2D();
    RangeProvider2D&	operator =(const RangeProvider2D&);
    const char*		type() const override;	//!< sKey::Range()
    const char*		factoryKeyword() const override { return type(); }
    Provider*		clone() const override
			{ return new RangeProvider2D(*this); }

    void		reset() override;

    bool		toNextPos() override;
    bool		toNextZ() override;

    int			curNr() const override;
    float		curZ() const override;
    Coord		curCoord() const override;
    TrcKey		curTrcKey() const override;
    bool		includes(int,float z=mUdf(float),
				 int lidx=0) const override;
    bool		includes(const Coord&,
				 float z=mUdf(float)) const override;
    void		usePar(const IOPar&) override;
    void		fillPar(IOPar&) const override;
    void		getSummary(BufferString&) const override;

    void		getExtent( Interval<int>& rg,
				   int lidx=-1 ) const override;
    void		getZRange( Interval<float>& rg,
				   int lidx ) const override;
    od_int64		estNrPos() const override;
    int			estNrZPerPos() const override;

    void			setTrcRange(const StepInterval<int>&,int idx=0);
    const StepInterval<int>&	trcRange(int lidx) const
				{return trcrgs_[lidx];}

    void			setZRange(const StepInterval<float>&,int idx=0);
    const StepInterval<float>&	zRange(int lidx=0) const
				{return zrgs_[lidx];}

protected:

    TypeSet< StepInterval<int> > trcrgs_;
    TypeSet< StepInterval<float> > zrgs_;
    int			curtrcidx_;
    int			curlineidx_;
    int			curzidx_;
    mutable int		curlinezsampsz_;
    mutable StepInterval<int>	curtrcrg_;
    mutable StepInterval<float> curzrg_;

    const Survey::Geometry2D*	curgeom_;

    Pos::GeomID			curGeomID() const;
    const Survey::Geometry2D*	curGeom() const;
    StepInterval<float>		curZRange() const;
    StepInterval<int>		curTrcRange() const;
    void			getCurRanges() const;

public:

    static void		initClass();
    static Provider2D*	create()	{ return new RangeProvider2D; }

};

} // namespace Pos
