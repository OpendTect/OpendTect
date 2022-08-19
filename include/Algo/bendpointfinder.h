#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "algomod.h"
#include "paralleltask.h"

#include "coord.h"
#include "ranges.h"
#include "thread.h"
#include "posinfo2d.h"


/*!
\brief Base class that does the majority of the work finding bendpoints.
Adaptions to different data-types are done in subclasses.
*/

mExpClass(Algo) BendPointFinderBase : public ParallelTask
{ mODTextTranslationClass(BendPointFinderBase);
public:

    const TypeSet<int>&	bendPoints() const	{ return bendpts_; }
    uiString		uiNrDoneText() const override
			{ return tr("Positions done"); }

protected:
			BendPointFinderBase( int sz, float eps );
    od_int64		nrIterations() const override { return sz_; }
    bool		doWork(od_int64,od_int64,int) override;
    virtual float	getMaxSqDistToLine(int& idx,int start,
					   int stop) const		= 0;
			/*!<Give the index of the point that is furthest from
			    the line from start to stop.
			    \returns the squre of the largest distance */
    void		findInSegment( int, int );
    bool		doPrepare(int) override;
    bool		doFinish(bool) override;

    TypeSet<int>		bendpts_;
    TypeSet<Interval<int> >	queue_;
    Threads::ConditionVar	lock_;
    int				nrworking_	= 0;
    int				sz_;
    const float			epssq_;

    mDeprecatedObs
    bool			finished_;	/* Obsolete, will be removed. */
    mDeprecatedObs
    int				nrwaiting_;	/* Obsolete, will be removed. */
    mDeprecated("Use nrworking_")
    int&			nrthreads_;	/* Obsolete, will be removed. */
};


/*!
\brief Used to find bendpoints in two dimensional datasets.
*/

mExpClass(Algo) BendPointFinder2DBase : public BendPointFinderBase
{
public:
protected:
		BendPointFinder2DBase(int size,float eps);

    virtual const Coord&	coord(int idx) const		= 0;

    float	getMaxSqDistToLine(int& idx,int start,int stop) const override;
};


/*!
\brief Used to find bendpoints in set of XY Coordinates
*/

mExpClass(Algo) BendPointFinder2D : public BendPointFinder2DBase
{
public:
		BendPointFinder2D(const TypeSet<Coord>&,float eps);
		BendPointFinder2D(const Coord*,int size,float eps);

protected:

    const Coord&		coord(int idx) const override;
    const Coord*		coords_;
};


/*!
\brief Used to find bendpoints in set of TrcKeys
*/

mExpClass(Algo) BendPointFinderTrcKey : public BendPointFinder2DBase
{
public:
		BendPointFinderTrcKey(const TypeSet<TrcKey>&,float eps);

protected:

    const Coord&		coord(int idx) const override;
    const TypeSet<TrcKey>&	tks_;
    TypeSet<Coord>		coords_;
};


/*!
\brief Used to find bendpoints in Line 2D Geometry
*/

mExpClass(Algo) BendPointFinder2DGeom : public BendPointFinder2DBase
{
public:
		BendPointFinder2DGeom(const TypeSet<PosInfo::Line2DPos>&,
				      float eps);
protected:

    const Coord&			coord(int idx) const override;
    const TypeSet<PosInfo::Line2DPos>&	positions_;
};


/*!
\brief Used to find bendpoints in three dimensional datasets.
*/

mExpClass(Algo) BendPointFinder3D : public BendPointFinderBase
{
public:
		BendPointFinder3D(const TypeSet<Coord3>&,
				  const Coord3& scale,float eps);
protected:
    float	getMaxSqDistToLine(int& idx,int start,int stop) const override;

    const Coord3*	coords_;
    const Coord3	scale_;
};
