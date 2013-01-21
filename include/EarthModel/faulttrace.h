#ifndef faulttrace_h
#define faulttrace_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		October 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "executor.h"
#include "multiid.h"
#include "positionlist.h"
#include "sets.h"
#include "surv2dgeom.h"
#include "trigonometry.h"

namespace EM { class Fault; class Horizon; }
class IOObj;
class MultiID;
class BinIDValueSet;
class HorSampling;

/*!
\brief Subclass of Coord3List that represents a fault trace.
*/

mExpClass(EarthModel) FaultTrace : public Coord3List
{
public:

    int			nextID(int) const;
    int			add(const Coord3&);
    int			add(const Coord3&,float trcnr);
    Coord3		get(int) const;
    const TypeSet<int>&	getIndices() const;
    float		getTrcNr(int) const;
    void		set(int,const Coord3&);
    void		set(int,const Coord3&,float);
    void		setIndices(const TypeSet<int>&);
    void		remove(int);
    bool		isDefined(int) const;
    int			getSize() const	{ return coords_.size(); }
    FaultTrace*		clone();

    bool		isInl() const			{ return isinl_; }
    int			lineNr() const			{ return nr_; }
    const Interval<int>& trcRange() const		{ return trcrange_; }
    const Interval<float>& zRange() const		{ return zrange_; }
    void		setIsInl(bool yn)		{ isinl_ = yn; }
    void		setLineNr(int nr)		{ nr_ = nr; }

    bool		isCrossing(const BinID&,float,const BinID&,float) const;
    bool		getIntersection(const BinID&,float,const BinID&,float,
	    				BinID&,float&,
					const StepInterval<int>* trcrg=0) const;
    bool		getHorCrossings(const BinIDValueSet&,
	    				Interval<float>& topzvals,
					Interval<float>& botzvals) const;
    bool		getHorIntersection(const EM::Horizon&,BinID&) const;
    bool		getHorTerminalPos( const EM::Horizon& hor,
					   BinID& pos1bid, float& pos1z,
					   BinID& pos2bid, float& pos2z ) const;
    bool		getImage(const BinID& srcbid,float srcz,
	    			 const Interval<float>& tophorzvals,
				 const Interval<float>& bothorzvals,
				 const StepInterval<int>& trcrg,BinID& imgbid,
				 float& imgz,bool forward) const;

    float		getZValFor(const BinID&) const;
    bool                isOnPosSide(const BinID&,float) const;
    void		addValue(int id,const Coord3&)	{}
    void		computeRange();
    bool                includes(const BinID&) const;
    bool		isOK() const;
    bool		isOnFault(const BinID&,float z,float threshold) const;
    			// threshold dist in measured in BinID units

protected:

    void		computeTraceSegments();
    Coord		getIntersection(const BinID&,float,
	    				const BinID&,float) const;
    bool		handleUntrimmed(const BinIDValueSet&,Interval<float>&,
	    				const BinID&,const BinID&,bool) const;

    bool		isinl_;
    int			nr_;
    TypeSet<Coord3>	coords_;
    TypeSet<int>	coordindices_;
    TypeSet<float>	trcnrs_;	// For 2D only;
    Interval<int>	trcrange_;
    Interval<float>	zrange_;
    TypeSet<Line2>	tracesegs_;

    Threads::Mutex	mutex_;
public:

};


/*!
\brief FaultTrace extractor 
*/

mExpClass(EarthModel) FaultTraceExtractor
{
public:
    				FaultTraceExtractor(EM::Fault*,int,bool);
    				FaultTraceExtractor(EM::Fault*,
						    const PosInfo::GeomID&);
				~FaultTraceExtractor();

    bool			execute();
    ObjectSet<FaultTrace>&	getFaultTraces()	{ return flttrcs_; }

protected:

    bool			get2DFaultTrace();

    bool			isinl_;
    int				nr_;
    PosInfo::GeomID		geomid_;	// For 2D
    EM::Fault*			fault_;
    ObjectSet<FaultTrace>	flttrcs_;
    bool			is2d_;
};


/*!
\brief FaultTrace calculator
*/

mExpClass(EarthModel) FaultTraceCalc : public Executor
{
public:
			FaultTraceCalc(EM::Fault*,const HorSampling&,
				       ObjectSet<FaultTrace>&);
			~FaultTraceCalc();

    od_int64		nrDone() const;
    od_int64		totalNr() const;
    const char*		message() const;
    int			nextStep();

protected:

    HorSampling&		hs_;
    int				curnr_;
    bool			isinl_;
    EM::Fault*			flt_;
    ObjectSet<FaultTrace>&	flttrcs_;
    od_int64			nrdone_;
};


/*!
\brief FaultTrace data provider
*/

mExpClass(EarthModel) FaultTrcDataProvider
{
public:
			FaultTrcDataProvider()
			    : is2d_(false)	{}
			FaultTrcDataProvider(const PosInfo::GeomID& geomid)
			    : geomid_(geomid),is2d_(geomid.isOK()) {}
			~FaultTrcDataProvider();

    bool		init(const TypeSet<MultiID>&,const HorSampling&,
	    		     TaskRunner* tr=0);

    bool		is2D() const		{ return is2d_; }
    int			nrFaults() const;
    const HorSampling&	range(int) const;
    bool		hasFaults(const BinID&) const;
    const FaultTrace*	getFaultTrace(int,int,bool) const;
    int			nrSticks(int fltidx) const;
    const FaultTrace*	getFaultTrace2D(int fltidx,int stickidx) const;
    bool		isCrossingFault(const BinID& b1,float z1,
	    				const BinID& b2,float z2) const;
    bool		getFaultZVals(const BinID&,TypeSet<float>&) const;
    bool		isOnFault(const BinID&,float z,float threshold) const;

    void		clear();
    bool		isEmpty() const;
    const char*		errMsg() const;

    const TypeSet<MultiID>& getFaultIds() const { return multiids_; }
    				
protected:

    bool		calcFaultBBox(const EM::Fault&,HorSampling&) const;
    bool		get2DTraces(const TypeSet<MultiID>&);

    bool				is2d_;
    ObjectSet<ObjectSet<FaultTrace> >   flttrcs_;
    TypeSet<HorSampling>                flths_;
    TypeSet<MultiID> 			multiids_;
    PosInfo::GeomID			geomid_;
    BufferString			errmsg_;
};

#endif
