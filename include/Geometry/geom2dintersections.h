#ifndef geom2dintersections_h
#define geom2dintersections_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2006
________________________________________________________________________

-*/

#include "geometrymod.h"
#include "bufstringset.h"
#include "executor.h"
#include "geometrymod.h"
#include "paralleltask.h"
#include "threadlock.h"

namespace Survey { class Geometry2D; }

mExpClass(Geometry) BendPoints
{
public:
			BendPoints();

    Pos::GeomID	geomid_;
    TypeSet<int>	idxs_;
};


mExpClass(Geometry) BendPointFinder2DGeomSet : public Executor
{ mODTextTranslationClass(BendPointFinder2DGeomSet)
public:
			BendPointFinder2DGeomSet(const TypeSet<Pos::GeomID>&);

    od_int64		nrDone() const;
    od_int64		totalNr() const;
    uiString		uiMessage() const;
    uiString		uiNrDoneText() const;

    const ObjectSet<BendPoints>& bendPoints() const	{ return bendptset_; }

protected:

    int			nextStep();

    const TypeSet<Pos::GeomID>&	geomids_;
    ObjectSet<BendPoints>	bendptset_;
    int				curidx_;
};


mExpClass(Geometry) Line2DInterSection
{
public:

    struct Point
    {
			Point(Pos::GeomID id,int mynr,int linenr)
			    : line(id),mytrcnr(mynr),linetrcnr(linenr) {}

	bool		operator==(const Point& oth) const
			{ return mytrcnr == oth.mytrcnr; }
	bool		operator>(const Point& oth) const
			{ return mytrcnr > oth.mytrcnr; }
	bool		operator<(const Point& oth) const
			{ return mytrcnr < oth.mytrcnr; }

	Pos::GeomID	line;	// Intersecting line.
	int		mytrcnr;
	int		linetrcnr;
    };

			Line2DInterSection(Pos::GeomID geomid)
			    : geomid_(geomid)	{}

    Pos::GeomID		geomID() const		{ return geomid_; }
    bool		isEmpty() const		{ return points_.isEmpty(); }
    int			size() const		{ return points_.size(); }

    const Line2DInterSection::Point&
			getPoint(int idx) const	{ return points_[idx]; }

    bool		getIntersectionTrcNrs(Pos::GeomID,int& mytrcnr,
					      int& crosstrcnr) const;
			//!<Returns false when not found

    void		addPoint(Pos::GeomID id,int mynr,int linenr);
    void		sort();

protected:

    Pos::GeomID				geomid_;
    TypeSet<Line2DInterSection::Point>	points_;
};


mExpClass(Geometry) Line2DInterSectionSet : public ObjectSet<Line2DInterSection>
{
public:
    const Line2DInterSection*	get(Pos::GeomID) const;
};


mExpClass(Geometry) Line2DInterSectionFinder : public ParallelTask
{ mODTextTranslationClass(Line2DInterSectionFinder)
public:
			Line2DInterSectionFinder(const ObjectSet<BendPoints>&,
						 Line2DInterSectionSet&);

    od_int64		nrIterations() const;
    uiString		uiMessage() const;
    uiString		uiNrDoneText() const;

protected:

    ObjectSet<const Survey::Geometry2D>	geoms_;
    const ObjectSet<BendPoints>&	bendptset_;
    Line2DInterSectionSet&		lsintersections_;

    bool		doWork(od_int64 start,od_int64 stop,int threadid);
    bool		doFinish(bool success);
    Threads::Lock	lock_;
};

#endif
