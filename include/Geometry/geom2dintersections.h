#ifndef geom2dintersections_h
#define geom2dintersections_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2006
 RCS:		$Id$
________________________________________________________________________

-*/

#include "geometrymod.h"
#include "paralleltask.h"
#include "bufstringset.h"
#include "threadlock.h"

namespace PosInfo { class LineSet2DData; }

mExpClass(Geometry) BendPoints
{
public:
			BendPoints();

    Pos::GeomID	geomid_;
    TypeSet<int>	idxs_;
};


mExpClass(Geometry) BendPointFinder : public ParallelTask
{ mODTextTranslationClass(BendPointFinder)
public:
			BendPointFinder(const PosInfo::LineSet2DData&,
					const TypeSet<Pos::GeomID>&);

    od_int64		nrIterations() const;
    uiString		uiMessage() const;
    uiString		uiNrDoneText() const;
    bool		doWork(od_int64 start,od_int64 stop,int threadid);

    const ObjectSet<BendPoints>& bendPoints() const	{ return bendptset_; }

protected:

    const PosInfo::LineSet2DData&	geometry_;
    const TypeSet<Pos::GeomID>&	geomids_;
    ObjectSet<BendPoints>		bendptset_;
};


mExpClass(Geometry) InterSections
{
public:
			InterSections(Pos::GeomID geomid)
			    : geomid_(geomid)	{}

    void		sort();

    bool		getIntersectionTrcNrs(Pos::GeomID,int& mytrcnr,
					      int& crosstrcnr) const;
			//!<Returns false when not found

    Pos::GeomID	geomid_;
    TypeSet<int>	mytrcnrs_;

    TypeSet<Pos::GeomID>	crossgeomids_; // names of intersecting lines
    TypeSet<int>	crosstrcnrs_; // trnrs of intersecting line
};


mExpClass(Geometry) LineSetInterSections : public ObjectSet<InterSections>
{
public:
    const InterSections*	get(Pos::GeomID) const;
};


mExpClass(Geometry) InterSectionFinder : public ParallelTask
{ mODTextTranslationClass(InterSectionFinder)
public:
			InterSectionFinder(const PosInfo::LineSet2DData&,
					   const ObjectSet<BendPoints>&,
					   LineSetInterSections&);

    od_int64		nrIterations() const;
    uiString		uiMessage() const;
    uiString		uiNrDoneText() const;

protected:
    const PosInfo::LineSet2DData&	geometry_;
    const ObjectSet<BendPoints>&	bendptset_;
    LineSetInterSections&		lsintersections_;

    bool		doWork(od_int64 start,od_int64 stop,int threadid);
    bool		doFinish(bool success);
    Threads::Lock	lock_;
};

#endif
