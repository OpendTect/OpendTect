#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
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
    const char*		type() const;	//!< sKey::Range()
    const char*		factoryKeyword() const { return type(); }
    virtual Provider*	clone() const	{ return new RangeProvider3D(*this); }

    virtual bool	initialize(TaskRunner* tr=0);
    virtual void	reset();

    virtual bool	toNextPos();
    virtual bool	toNextZ();

    virtual BinID	curBinID() const	{ return curbid_; }
    virtual float	curZ() const;
    virtual bool	includes(const BinID&,float z=mUdf(float)) const;
    virtual void	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;
    virtual void	getSummary(BufferString&) const;

    virtual void	getExtent(BinID& start,BinID& stop) const;
    virtual void	getZRange(Interval<float>&) const;
    virtual od_int64	estNrPos() const;
    virtual int		estNrZPerPos() const;
    void		getTrcKeyZSampling(TrcKeyZSampling&) const;

    const TrcKeyZSampling&	sampling() const	{ return tkzs_; }
    void		setSampling( const TrcKeyZSampling& tkzs );
    void		setHSampling( const TrcKeySampling& tks ) const
						{ tkzs_.hsamp_ = tks; }

    virtual bool	includes( const Coord& c, float z=mUdf(float) ) const
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
    const char*		type() const;	//!< sKey::Range()
    const char*		factoryKeyword() const { return type(); }
    virtual Provider*	clone() const	{ return new RangeProvider2D(*this); }

    virtual void	reset();

    virtual bool	toNextPos();
    virtual bool	toNextZ();

    virtual int		curNr() const;
    virtual float	curZ() const;
    virtual Coord	curCoord() const;
    virtual TrcKey	curTrcKey() const;
    virtual bool	includes(int,float z=mUdf(float),int lidx=0) const;
    virtual bool	includes(const Coord&,float z=mUdf(float)) const;
    virtual void	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;
    virtual void	getSummary(BufferString&) const;

    virtual void	getExtent( Interval<int>& rg, int lidx=-1 ) const;
    virtual void	getZRange( Interval<float>& rg, int lidx ) const;
    virtual od_int64	estNrPos() const;
    virtual int		estNrZPerPos() const;

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

} // namespace

