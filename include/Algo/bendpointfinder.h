#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		Aug 2009
________________________________________________________________________

*/

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
    uiString		uiNrDoneText() const	{ return tr("Positions done"); }

protected:
			BendPointFinderBase( int sz, float eps );
    od_int64		nrIterations() const { return sz_; }
    bool		doWork( od_int64, od_int64, int );
    virtual float	getMaxSqDistToLine(int& idx, int start,
					   int stop ) const		= 0;
			/*!<Give the index of the point that is furthest from
			    the line from start to stop.
			    \returns the squre of the largest distance */
    void		findInSegment( int, int );
    bool		doPrepare(int);
    bool		doFinish(bool);

    TypeSet<int>		bendpts_;
    TypeSet<Interval<int> >	queue_;
    Threads::ConditionVar	lock_;
    bool			finished_;	/* Obsolete, will be removed. */
    int				nrwaiting_;	/* Obsolete, will be removed. */
    int				nrthreads_;	/* Obsolete, will be removed,
						   but now used as nrworking_
						   variable to preserve ABI. */
    int				sz_;
    const float			epssq_;
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

    float	getMaxSqDistToLine(int& idx,int start,int stop) const;
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

    const Coord&		coord(int idx) const;
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

    const Coord&		coord(int idx) const;
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

    const Coord&			coord(int idx) const;
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
    float	getMaxSqDistToLine(int& idx,int start,int stop) const;

    const Coord3*	coords_;
    const Coord3	scale_;
};

