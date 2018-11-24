#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Salil Agarwal / Bert
 Date:		Dec 2012 / Nov 2018
________________________________________________________________________

*/

#include "generalmod.h"
#include "survgeom2d.h"
#include "survgeommgr.h"
class IOObj;
class TaskRunnerProvider;


namespace Survey
{

/*!\brief creates entries and writes geometries to the 2D geometry repository */

mExpClass(General) ODGeometry2DWriter : public Geometry2DWriter
{
public:

    static Geometry2DWriter* createNew() { return new ODGeometry2DWriter; }
    static void		initClass();

    bool	write(const Geometry2D&,uiString&,const char*) const override;
    GeomID	getGeomIDFor(const char*) const override;

    IOObj*	getEntry(const char*) const;

};


/*!\brief gets info from the 2D geometry repository */

mExpClass(General) ODGeometry2DReader : public Geometry2DReader
{
public:

    static Geometry2DReader* createNew() { return new ODGeometry2DReader; }
    static void		initClass();

    bool		read(ObjectSet<Geometry2D>&,
				const TaskRunnerProvider&) const override;
    bool		updateGeometries(ObjectSet<Geometry2D>&,
				 const TaskRunnerProvider&) const override;

};


} // namespace Survey
