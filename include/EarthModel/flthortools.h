#ifndef flthortools_h
#define flthortools_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		October 2008
 RCS:		$Id: flthortools.h,v 1.17 2010-11-15 03:32:31 raman Exp $
________________________________________________________________________

-*/

#include "executor.h"
#include "positionlist.h"
#include "sets.h"

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
    void		setIsInl(bool yn)		{ isinl_ = yn; }
    void		setLineNr(int nr)		{ nr_ = nr; }
    bool		isCrossing(const BinID&,float,const BinID&,float) const;
    float		getZValFor(const BinID&) const;
    void		addValue(int id,const Coord3&)	{}

protected:

    bool		isinl_;
    int			nr_;
    TypeSet<Coord3>	coords_;
    TypeSet<int>	coordindices_;
    TypeSet<int>	trcnrs_;	// For 2D only;
};


mClass FaultTraceExtractor
{
public:
    			FaultTraceExtractor(EM::Fault*,int,bool,
					    const BinIDValueSet* bvs=0);
    			FaultTraceExtractor(EM::Fault*,const char*,int sticknr,
					    const BinIDValueSet* bvs=0);
			~FaultTraceExtractor();

    bool		execute();
    FaultTrace*		getFaultTrace()		{ return flttrc_; }

protected:

    bool		isinl_;
    int			nr_;
    int			sticknr_;	// For 2D
    EM::Fault*		fault_;
    FaultTrace*		flttrc_;
    bool		is2d_;
    BufferString	linenm_;

    const BinIDValueSet*	bvset_;

    void		useHorizons();
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
