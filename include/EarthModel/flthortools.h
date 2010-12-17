#ifndef flthortools_h
#define flthortools_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		October 2008
 RCS:		$Id: flthortools.h,v 1.20 2010-12-17 06:39:33 raman Exp $
________________________________________________________________________

-*/

#include "executor.h"
#include "positionlist.h"
#include "sets.h"
#include "surv2dgeom.h"

namespace EM { class Fault; }
class IOObj;
class BinIDValueSet;
class HorSampling;


mClass FaultTrace : public Coord3List
{
public:

    int			nextID(int) const;
    int			add(const Coord3&);
    int			add(const Coord3&,int trcnr);
    Coord3		get(int) const;
    const TypeSet<int>&	getIndices() const;
    int			getTrcNr(int) const;
    void		set(int,const Coord3&);
    void		set(int,const Coord3&,int);
    void		setIndices(const TypeSet<int>&);
    void		remove(int);
    bool		isDefined(int) const;
    int			getSize() const	{ return coords_.size(); }
    FaultTrace*		clone();

    bool		isInl() const			{ return isinl_; }
    int			lineNr() const			{ return nr_; }
    const Interval<int>& range() const			{ return range_; }
    void		setIsInl(bool yn)		{ isinl_ = yn; }
    void		setLineNr(int nr)		{ nr_ = nr; }

    bool		isCrossing(const BinID&,float,const BinID&,float) const;
    bool		getHorCrossings(const BinIDValueSet&,
	    				Interval<float>& topzvals,
					Interval<float>& botzvals) const;
    bool		getImage(const BinID& srcbid,float srcz,
	    			 const Interval<float>& tophorzvals,
				 const Interval<float>& bothorzvals,
				 const StepInterval<int>& trcrg,BinID& imgbid,
				 float& imgz,bool forward) const;

    float		getZValFor(const BinID&) const;
    void		addValue(int id,const Coord3&)	{}
    void		computeRange();

protected:

    Coord		getIntersection(const BinID&,float,
	    				const BinID&,float) const;

    bool		isinl_;
    int			nr_;
    TypeSet<Coord3>	coords_;
    TypeSet<int>	coordindices_;
    TypeSet<int>	trcnrs_;	// For 2D only;
    Interval<int>	range_;
};


mClass FaultTraceExtractor
{
public:
    			FaultTraceExtractor(EM::Fault*,int,bool);
    			FaultTraceExtractor(EM::Fault*,const PosInfo::GeomID&);
			~FaultTraceExtractor();

    bool		execute();
    FaultTrace*		getFaultTrace()		{ return flttrc_; }

protected:

    bool		isinl_;
    int			nr_;
    PosInfo::GeomID	geomid_;	// For 2D
    EM::Fault*		fault_;
    FaultTrace*		flttrc_;
    bool		is2d_;

    bool		get2DFaultTrace();
};


mClass FaultTraceCalc : public Executor
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


#endif
