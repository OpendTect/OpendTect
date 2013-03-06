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
class TaskRunner;
class IOObj;

namespace Survey
{

/*!
\brief A Geometry which holds trace positions.
*/

mExpClass(Basic) Geometry
{ mRefCountImpl(Geometry);
public:
    virtual bool	is2D() const					= 0;
    TraceID::GeomID	getGeomID() const { return geomid_; }
    void		setGeomID( TraceID::GeomID id ) { geomid_ = id; }
    Coord		toCoord(const TraceID& tid) const;
    virtual Coord	toCoord(int linenr,int tracenr) const		= 0;
    virtual TraceID	nearestTrace(const Coord&,float* distance) const= 0;
    virtual TraceID	getTrace(const Coord&,float maxdist) const;
			//!<returns undef if no trace found

    bool		includes(const TraceID& tid) const;
    virtual bool	includes(int linenr,int tracenr) const		= 0;
    
protected:
			Geometry();
    TraceID::GeomID	geomid_;
};


/*!
\brief Makes geometries accessible from a geometry id, or a multi id.
*/

mExpClass(Basic) GeometryManager
{
public:
				GeometryManager();
				~GeometryManager();
    const Geometry*		getGeometry(TraceID::GeomID) const;
    const Geometry*		getGeometry(const MultiID&) const;

    TraceID::GeomID		getGeomID(const char* linename) const;
    const char*			getName(TraceID::GeomID) const;
    
    Coord			toCoord(const TraceID&) const;

    bool			fetchFrom2DGeom();
				//converts od4 geometries to od5 geometries.

    bool			write(Geometry&);

    IOObj*			createEntry(const char* name,const bool is2d);
				// returns new GeomID.
    
    void			removeGeometry(TraceID::GeomID);    
    
    bool			fillGeometries(TaskRunner*);
    static TraceID::GeomID	cDefault3DGeom() { return -1; }
    static TraceID::GeomID	cUndefGeomID() { return mUdf(TraceID::GeomID); }

protected:
    void			addGeometry(Geometry&);
    bool			hasDuplicateLineNames();

    int				indexOf(TraceID::GeomID) const;

    ObjectSet<Geometry>		geometries_;
};


mGlobal(Basic) GeometryManager& GMAdmin();


inline mGlobal(Basic) const GeometryManager& GM()
{ return const_cast<GeometryManager&>( Survey::GMAdmin() ); }


/*!
\brief Geometry Reader
*/

mExpClass(Basic) GeometryReader
{
public:
			GeometryReader(){};
			mDefineFactoryInClass(GeometryReader,factory);

    virtual bool	read(ObjectSet<Geometry>&,TaskRunner*) const
							{ return true; }
};


/*!
\brief Geometry Writer
*/

mExpClass(Basic) GeometryWriter
{
public:
			GeometryWriter(){};
			mDefineFactoryInClass(GeometryWriter,factory);

    virtual bool	write(Geometry&) const		{ return true; }
    virtual IOObj*	createEntry(const char*) const	{ return 0; }
};

} //namespace Survey


#endif
