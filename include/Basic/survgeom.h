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
#include "coord.h"
#include "trckey.h"
#include "refcount.h"

class TaskRunner;
class IOObj;

namespace Survey
{

/*!
\brief A Geometry which holds trace positions.

For 3D, a geometry is the InlCrlSystem.
For 2D, each line has a Geometry.

Beware, the Geometry::ID != Survkey::ID for 2D geometries. The Geometry::ID
will end up in the lineNr() of the TrcKey.

*/

mExpClass(Basic) Geometry
{			mRefCountImpl(Geometry);
public:

    typedef Pos::GeomID	ID;

    virtual bool	is2D() const			= 0;
    static TrcKey::SurvID get2DSurvID();

    ID			getID() const			{ return id_; }
    void		setID( ID id )			{ id_ = id; }
    virtual const char*	getName() const			= 0;

    virtual Coord	toCoord(Pos::LineID,Pos::TraceID) const		= 0;
    virtual bool	includes(Pos::LineID,Pos::TraceID) const	= 0;
    bool		includes(const TrcKey&) const;

    static Coord	toCoord(const TrcKey&);
    static bool		exists(const TrcKey&);

    inline Coord	toCoord( const BinID& b ) const
			{ return toCoord( b.lineNr(), b.trcNr() ); }
    inline bool		includes( const BinID& b ) const
			{ return includes( b.lineNr(), b.trcNr() ); }

    virtual TrcKey	nearestTrace(const Coord&,float* distance=0) const = 0;
    virtual TrcKey	getTrace(const Coord&,float maxdist) const;
				//!<returns undef if no trace found

    Geometry::ID	getID(const char* geomname) const;
    const char*		getName(Geometry::ID) const;
    
    virtual StepInterval<float> zRange() const				= 0;

protected:

			Geometry();

    ID			id_;

};


/*!
\brief Makes geometries accessible from a geometry id, or a multi id.
*/

mExpClass(Basic) GeometryManager
{
public:

				GeometryManager();
				~GeometryManager();

    const Geometry*		getGeometry(Geometry::ID) const;
    const Geometry*		getGeometry(const MultiID&) const;

    Geometry::ID		getGeomID(const TrcKey&) const;
    Geometry::ID		getGeomID(const char* survname) const;
    const char*			getName(Geometry::ID) const;
    
    Coord			toCoord(const TrcKey&) const;

    bool			fetchFrom2DGeom();
				//converts od4 geometries to od5 geometries.

    bool			write(Geometry&);

    IOObj*			createEntry(const char* name,const bool is2d);
				// returns new GeomID.
    
    void			removeGeometry(Geometry::ID);
    
    bool			fillGeometries(TaskRunner*);

    static TrcKey::SurvID	get2DSurvID()	{ return surv2did_; }
    TrcKey::SurvID		default3DSurvID() const;
    static Geometry::ID		cUndefGeomID()	{ return mUdf(Geometry::ID); }

protected:

    void			ensureSIPresent() const;
    void			addGeometry(Geometry&);
    bool			hasDuplicateLineNames();

    int				indexOf(Geometry::ID) const;

    ObjectSet<Geometry>		geometries_;
    static const TrcKey::SurvID	surv2did_;

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
    virtual		~GeometryReader()		{};
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
    virtual		~GeometryWriter()		{};
			mDefineFactoryInClass(GeometryWriter,factory);

    virtual bool	write(Geometry&) const		{ return true; }
    virtual IOObj*	createEntry(const char*) const	{ return 0; }
};

} //namespace Survey


#endif
