#ifndef flthortools_h
#define flthortools_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		October 2008
 RCS:		$Id: flthortools.h,v 1.13 2010-01-21 11:44:01 raman Exp $
________________________________________________________________________

-*/

#include "arrayndimpl.h"
#include "cubesampling.h"
#include "multiid.h"
#include "positionlist.h"
#include "sets.h"

namespace EM { class Fault; }
class IOObj;
class BinIDValueSet;

namespace SSIS
{

mClass FaultTrace : public Coord3List
{
public:

    int			nextID(int) const;
    int			add(const Coord3&);
    int			add(const Coord3&,int trcnr);
    Coord3		get(int) const;
    int			getTrcNr(int) const;
    void		set(int,const Coord3&);
    void		set(int,const Coord3&,int);
    void		remove(int);
    bool		isDefined(int) const;
    int			getSize() const	{ return coords_.size(); }
    void		sortZ();
    FaultTrace*		clone();

    bool		isInl() const			{ return isinl_; }
    int			lineNr() const			{ return nr_; }
    void		setIsInl(bool yn)		{ isinl_ = yn; }
    void		setLineNr(int nr)		{ nr_ = nr; }
    bool		isCrossing(const BinID&,float,const BinID&,float) const;
    float		getZValFor(const BinID&) const;

protected:

    bool		isinl_;
    int			nr_;
    TypeSet<Coord3>	coords_;
    TypeSet<int>	trcnrs_;	// For 2D only;
    Threads::Mutex	lock_;
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


}; // namespace SSIS

#endif
