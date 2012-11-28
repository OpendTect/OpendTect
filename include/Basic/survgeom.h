#ifndef survgeom_h
#define survgeom_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		9-4-1996
 RCS:		$Id$
________________________________________________________________________

-*/

#include "position.h"
#include "refcount.h"

class MultiID;

namespace Survey
{

/* A Geometry which holds trace positions. */

mClass(Basic) Geometry
{ mRefCountImpl(Geometry);
public:
    int			getGeomID() const { return geomid_; }
    void		setGeomID(int id) { geomid_ = id; }
    Coord		toCoord(const TraceID& tid) const 
			{ return toCoord( tid.line_, tid.trcnr_ ); }
    virtual Coord	toCoord(int line, int tracenr) const		= 0;
    virtual TraceID	nearestTrace(const Coord&,float* distance) const= 0;
    virtual TraceID	getTrace(const Coord&,float maxdist) const;
			//!<returns undef if no trace found

    bool		includes(const TraceID& tid) const 
			{ return includes( tid.line_, tid.trcnr_ ); }
    virtual bool	includes(int line, int tracenr)	const		= 0;
    
protected:
			Geometry();
    int			geomid_;
};

/* Makes geometries accessible from a geometry id, or a multi id. */

mClass(Basic) GeometryManager
{
public:
			GeometryManager();
			~GeometryManager();
    const Geometry*	getGeomety(int geomid) const;
    const Geometry*	getGeomety(const MultiID&) const;
    
    Coord		transform(const TraceID&) const;
    
    static int		cDefault3DGeom() { return -1; }

protected:
    void		addGeometry(Geometry*);

    ObjectSet<Geometry> geometries_;
};

} //namespace Survey



#endif
