#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "geometrymod.h"

#include "bufstringset.h"
#include "geometry.h"
#include "posgeomid.h"
#include "paralleltask.h"
#include "threadlock.h"

namespace Survey { class Geometry2D; }

mExpClass(Geometry) BendPoints
{
public:
			BendPoints();
    virtual		~BendPoints();

    Pos::GeomID		geomid_;
    TypeSet<int>	idxs_;
};


mExpClass(Geometry) BendPointFinder2DGeomSet : public ParallelTask
{ mODTextTranslationClass(BendPointFinder2DGeomSet)
public:
			BendPointFinder2DGeomSet(const TypeSet<Pos::GeomID>&,
						 ObjectSet<BendPoints>&);
			~BendPointFinder2DGeomSet();

    od_int64		nrIterations() const override;
    uiString		uiMessage() const override;
    uiString		uiNrDoneText() const override;

protected:

    bool		doWork(od_int64,od_int64,int) override;

    Threads::Lock	lock_;
    const TypeSet<Pos::GeomID>&	geomids_;
    ObjectSet<BendPoints>&	bendptset_;
};


mExpClass(Geometry) Line2DInterSection
{
public:

    mExpStruct(Geometry) Point
    {
			Point(const Pos::GeomID& id,int mynr,int linenr);
			Point(const Pos::GeomID& myid,const Pos::GeomID& lineid,
			      int mynr,int linenr);
			Point(const Point&);
			~Point();

	bool		operator==(const Point& oth) const
			{ return mytrcnr == oth.mytrcnr; }
	bool		operator>(const Point& oth) const
			{ return mytrcnr > oth.mytrcnr; }
	bool		operator<(const Point& oth) const
			{ return mytrcnr < oth.mytrcnr; }

	bool		isOpposite(const Point&) const;

	Pos::GeomID	line;	// Intersecting line.
	Pos::GeomID	mygeomids_;
	int		mytrcnr;
	int		linetrcnr;
    };

			Line2DInterSection(const Pos::GeomID&);
    virtual		~Line2DInterSection();

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


mExpClass(Geometry) Line2DInterSectionSet :
		public ManagedObjectSet<Line2DInterSection>
{
public:
				Line2DInterSectionSet();
				~Line2DInterSectionSet();

    const Line2DInterSection*	getByGeomID(Pos::GeomID) const;
    void		getAll(TypeSet<Line2DInterSection::Point>&) const;
};


mExpClass(Geometry) Line2DInterSectionFinder : public ParallelTask
{ mODTextTranslationClass(Line2DInterSectionFinder)
public:
			Line2DInterSectionFinder(const ObjectSet<BendPoints>&,
						 Line2DInterSectionSet&);
			~Line2DInterSectionFinder();

    od_int64		nrIterations() const override;
    uiString		uiMessage() const override;
    uiString		uiNrDoneText() const override;

protected:

    ObjectSet<const Survey::Geometry2D>	geoms_;
    const ObjectSet<BendPoints>&	bendptset_;
    Line2DInterSectionSet&		lsintersections_;
    TypeSet<Geom::Rectangle<double>>	bboxs_;

    bool		doWork(od_int64 start,
			       od_int64 stop,int threadid) override;
    bool		doFinish(bool success) override;
    Threads::Lock	lock_;
    int			counter_;
};


mExpClass(Basic) Line2DIntersectionManager final
{
public:
    virtual			~Line2DIntersectionManager();
//				mOD_DisableCopy(Line2DIntersectionManager)

    static const Line2DIntersectionManager&	instance();
    static Line2DIntersectionManager&	instanceAdmin();

    bool				computeBendpoints(TaskRunner* =nullptr);
    bool				compute(TaskRunner* =nullptr);
    const Line2DInterSectionSet&	intersections() const;
    const ObjectSet<BendPoints>&	bendpoints() const;

    int				indexOf(const Pos::GeomID&) const;
    const BendPoints*		getBendPoints(const Pos::GeomID&) const;
    const Line2DInterSection*	getIntersection(const Pos::GeomID&) const;

private:
					Line2DIntersectionManager();

    Line2DInterSectionSet		intersections_;
    ManagedObjectSet<BendPoints>	bendpointset_;
    std::unordered_map<int,int>		geomidmap_;
};
