#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "earthmodelmod.h"

#include "multiid.h"
#include "paralleltask.h"
#include "positionlist.h"
#include "threadlock.h"
#include "trckeysampling.h"
#include "trigonometry.h"

namespace EM { class Fault; class Horizon; }
namespace Geometry { class ExplFaultStickSurface; }
namespace PosInfo { class Line2DData; }
class BinIDValueSet;

/*!
\brief Subclass of Coord3List that represents a fault trace.
*/

mExpClass(EarthModel) FaultTrace : public Coord3List
{ mODTextTranslationClass(FaultTrace);
public:
			FaultTrace();

    int			nextID(int) const override;
    int			add(const Coord3&) override;
    int			add(const Coord3&,float trcnr);
    Coord3		get(int) const override;
    const TypeSet<int>&	getIndices() const;
    float		getTrcNr(int) const;
    void		set(int,const Coord3&) override;
    void		set(int,const Coord3&,float);
    void		setIndices(const TypeSet<int>&);
    void		remove(int) override;
    void		remove(const TypeSet<int>&) override	{}
    bool		isDefined(int) const override;
    int			size() const override	{ return coords_.size(); }
    FaultTrace*		clone() const;

    bool		isInl() const			{ return isinl_; }
    bool		isEditedOnCrl() const		{ return editedoncrl_; }
    int			lineNr() const			{ return nr_; }
    const Interval<int>& trcRange() const		{ return trcrange_; }
    const Interval<float>& zRange() const		{ return zrange_; }
    void		setIsInl(bool yn)		{ isinl_ = yn; }
    void		setEditedOnCrl(bool yn)		{ editedoncrl_ = yn; }
    void		setLineNr(int nr)		{ nr_ = nr; }

    bool		isCrossing(const BinID&,float,const BinID&,float) const;
    bool		getIntersection(const BinID&,float,const BinID&,float,
					BinID&,float&,
					const StepInterval<int>* trcrg=0,
					bool snappositive=true) const;
    bool		getHorCrossings(const BinIDValueSet&,
					Interval<float>& topzvals,
					Interval<float>& botzvals) const;
    bool		getHorIntersection(const EM::Horizon&,BinID&) const;

    bool		getCoordsBetween(int trc0,float z0,int trc1,float z1,
					 TypeSet<Coord>& coords) const;
    bool		getIntersectionZs(int trcnr,TypeSet<float>& zs) const;
    bool		getIntersectionTraces(float zval,
					      TypeSet<int>& trcs) const;
    bool		getFaultTraceIntersection(const FaultTrace&,
						  int& trace,float& zval) const;

    bool		getHorizonIntersectionInfo(const EM::Horizon& hor,
				Pos::GeomID geomid,
				TypeSet<BinID>& pos1bids,TypeSet<float>& pos1zs,
				TypeSet<BinID>& pos2bids,TypeSet<float>& pos2zs,
				TypeSet<Coord>& intersections,
				bool findfirstonly=true,
				bool allowextention=false) const;
    bool		getImage(const BinID& srcbid,float srcz,
				 const Interval<float>& tophorzvals,
				 const Interval<float>& bothorzvals,
				 const StepInterval<int>& trcrg,BinID& imgbid,
				 float& imgz,bool forward) const;

    float		getZValFor(const BinID&) const;
    bool		isOnPosSide(const BinID&,float) const;
    void		addValue(int id,const Coord3&) override {}
    void		computeRange();
    bool		includes(const BinID&) const;
    bool		isOK() const;
    bool		isOnFault(const BinID&,float z,float threshold) const;
			// threshold dist in measured in BinID units

    enum Act		{ AllowCrossing, ForbidCrossing,
			  ForbidCrossHigher, ForbidCrossLower,
			  AllowMinTraceToFault, AllowMaxTraceToFault,
			  AllowMinInlToFault, AllowMaxInlToFault,
			  AllowMinCrlToFault, AllowMaxCrlToFault };
    static void		getAllActNames(BufferStringSet&);
    static void		getActNames(BufferStringSet&,bool is2d);
    static const char*	sKeyFaultAct()	{ return "Fault Act"; }

protected:
			~FaultTrace();

    void		computeTraceSegments();
    Coord		getIntersection(const BinID&,float,
					const BinID&,float) const;
    bool		handleUntrimmed(const BinIDValueSet&,Interval<float>&,
					const BinID&,const BinID&,bool) const;

    bool		isinl_;
    bool		editedoncrl_;
    int			nr_;
    TypeSet<Coord3>	coords_;
    TypeSet<int>	coordindices_;
    TypeSet<float>	trcnrs_;	// For 2D only;
    Interval<int>	trcrange_;
    Interval<float>	zrange_;
    TypeSet<Line2>	tracesegs_;

    mutable Threads::Lock	lock_;
};


/*!
\brief FaultTrace holder
*/

mExpClass(EarthModel) FaultTrcHolder
{ mODTextTranslationClass(FaultTrcHolder);
public:
			FaultTrcHolder();
    virtual		~FaultTrcHolder();

    const FaultTrace*	getTrc(int linenr,bool isinl) const;
    int			indexOf(int linenr,bool isinl) const;

    bool		isEditedOnCrl() const;

    ObjectSet<FaultTrace>	traces_;
			    /*For 3D: one for each inline followed by
				      one for each crossline.
			      For 2D: One for each stick.*/

    TrcKeySampling		hs_;
};


/*!
\brief FaultTrace extractor
*/

mExpClass(EarthModel) FaultTraceExtractor : public ParallelTask
{ mODTextTranslationClass(FaultTraceExtractor);
public:
			~FaultTraceExtractor();

    uiString		uiMessage() const override;

protected:
			FaultTraceExtractor(const EM::Fault&,FaultTrcHolder&);

    virtual bool	extractFaultTrace(int)			= 0;
    virtual od_int64	nrIterations() const override;
    virtual bool	doPrepare(int) override;
    virtual bool	doWork(od_int64,od_int64,int) override;
    virtual bool	doFinish(bool) override { return true; }

    const EM::Fault&	fault_;
    FaultTrcHolder&	holder_;
    bool		editedoncrl_;
    od_int64		totalnr_;
};


mExpClass(EarthModel) FaultTraceExtractor3D : public FaultTraceExtractor
{ mODTextTranslationClass(FaultTraceExtractor3D);
public:
			FaultTraceExtractor3D(const EM::Fault&,FaultTrcHolder&);
			~FaultTraceExtractor3D();

protected:

    bool		extractFaultTrace(int) override;

    virtual bool	doPrepare(int) override;

    Geometry::ExplFaultStickSurface* fltsurf_;

};


mExpClass(EarthModel) FaultTraceExtractor2D : public FaultTraceExtractor
{ mODTextTranslationClass(FaultTraceExtractor2D);
public:
			FaultTraceExtractor2D(const EM::Fault&,FaultTrcHolder&,
					      Pos::GeomID);
			~FaultTraceExtractor2D();

protected:

    bool		extractFaultTrace(int) override;
    virtual bool	doPrepare(int) override;
    virtual bool	doFinish(bool) override;

    Pos::GeomID		geomid_;
};


/*!
\brief FaultTrace data provider
*/

mExpClass(EarthModel) FaultTrcDataProvider
{ mODTextTranslationClass(FaultTrcDataProvider);
public:
			FaultTrcDataProvider();
			FaultTrcDataProvider(Pos::GeomID);
    virtual		~FaultTrcDataProvider();
			mOD_DisableCopy(FaultTrcDataProvider)

    bool		init(const TypeSet<MultiID>&,const TrcKeySampling&,
			     TaskRunner* =0);

    bool		is2D() const		{ return is2d_; }
    int			nrFaults() const;
    TrcKeySampling	range(int) const;
    int			nrSticks(int fltidx) const;
    bool		isEditedOnCrl(int fltidx) const;

    bool		hasFaults(const BinID&) const;
    const FaultTrace*	getFaultTrace(int fltidx,int trcnr,bool isinl) const;
    const FaultTrace*	getFaultTrace2D(int fltidx,int stickidx) const;
    bool		isCrossingFault(const BinID& b1,float z1,
					const BinID& b2,float z2) const;
    bool		getFaultZVals(const BinID&,TypeSet<float>&) const;
    bool		isOnFault(const BinID&,float z,float threshold) const;

    void		clear();
    bool		isEmpty() const;
    uiString		errMsg() const;

protected:

    bool		calcFaultBBox(const EM::Fault&,TrcKeySampling&) const;
    bool		get2DTraces(const TypeSet<MultiID>&,TaskRunner*);

    ObjectSet<FaultTrcHolder>	holders_;

    Pos::GeomID		geomid_;
    uiString		errmsg_;
    bool		is2d_;
};
