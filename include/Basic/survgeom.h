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

#include "factory.h"
#include "position.h"
#include "refcount.h"

class MultiID;

namespace Survey
{

/* A Geometry which holds trace positions. */

mClass(Basic) Geometry
{ mRefCountImpl(Geometry);
public:
    virtual bool	is2D() const					= 0;
    int			getGeomID() const { return geomid_; }
    void		setGeomID(int id) { geomid_ = id; }
    virtual Coord	toCoord(const TraceID& tid) const
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
    const Geometry*	getGeometry(int geomid) const;
    const Geometry*	getGeometry(const MultiID&) const;

    const int		getGeomID(const char* linename) const;
    const char*		getName(const int geomid) const;
    
    Coord		toCoord(const TraceID&) const;

    bool		fetchFrom2DGeom();
				//converts od4 geometries to od5 geometries.

    bool		write(Geometry*);

    int			createEntry(const char* name,const bool is2d);
				// returns new GeomID.
    
    static int		cDefault3DGeom() { return -1; }

protected:
    void		addGeometry(Geometry*);

    ObjectSet<Geometry> geometries_;
};


mGlobal(Basic) GeometryManager& GMAdmin();


inline mGlobal(Basic) const GeometryManager& GM()
{ return const_cast<GeometryManager&>( Survey::GMAdmin() ); }


mClass(Basic) GeometryReader
{
public:
			GeometryReader(){};
			mDefineFactoryInClass(GeometryReader,factory);
};


mClass(Basic) GeometryWriter
{
public:
			GeometryWriter(){};
			mDefineFactoryInClass(GeometryWriter,factory);

    virtual bool	write(Geometry*)		    {return true;};
    virtual int		createEntry(const char*)	    {return 0;}
};

} //namespace Survey


#endif
