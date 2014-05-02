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

/*!\brief A Geometry which holds trace positions.

For 3D, a geometry is an Inl/Crl System.
For 2D, each line has its own Geometry.

Beware, the Geometry::ID != Survkey::ID for 2D geometries. The Geometry::ID
will end up in the lineNr() of the TrcKey.

*/

mExpClass(Basic) Geometry
{ mRefCountImpl(Geometry);
public:

    typedef Pos::GeomID	ID;

    virtual bool	is2D() const			= 0;
    static const Geometry& default3D();

    ID			getID() const			{ return id_; }
    void		setID( ID id )			{ id_ = id; }
    virtual const char*	getName() const			= 0;

    virtual Coord	toCoord(Pos::LineID,Pos::TraceID) const		= 0;
    inline Coord	toCoord( const BinID& b ) const
			{ return toCoord( b.lineNr(), b.trcNr() ); }

    virtual bool	includes(Pos::LineID,Pos::TraceID) const	= 0;
    bool		includes(const TrcKey&) const;
    inline bool		includes( const BinID& b ) const
			{ return includes( b.lineNr(), b.trcNr() ); }

    static bool		exists(const TrcKey&);
    static Coord	toCoord(const TrcKey&);

    virtual TrcKey	getTrace(const Coord&,float maxdist) const;
    virtual TrcKey	nearestTrace(const Coord&,float* distance=0) const = 0;

    virtual StepInterval<float> zRange() const				= 0;

    virtual float		averageTrcDist() const			= 0;

protected:

			Geometry();

    ID			id_;

};


/*!\brief Makes geometries accessible from a geometry ID, or a MultiID.  */

mExpClass(Basic) GeometryManager
{mODTextTranslationClass(GeometryManager);
public:

				GeometryManager();
				~GeometryManager();

    const Geometry*		getGeometry(Geometry::ID) const;
    const Geometry*		getGeometry(const char*) const;
    const Geometry*		getGeometry(const MultiID&) const;

    int 			nrGeometries() const;

    Geometry::ID		getGeomID(const TrcKey&) const;
    Geometry::ID		getGeomID(const char* linenm) const;
    const char*			getName(Geometry::ID) const;
    
    Coord			toCoord(const TrcKey&) const;

    TrcKey			traceKey(Geometry::ID,Pos::LineID,
					 Pos::TraceID) const;
				//!<For 3D
    TrcKey			traceKey(Geometry::ID,Pos::TraceID) const;
				//!<For 2D

    bool			fillGeometries(TaskRunner*);
    bool			getList(BufferStringSet& names,
					TypeSet<Geometry::ID>& ids,
					bool is2d) const;

    static TrcKey::SurvID	get2DSurvID()	{ return surv2did_; }
    TrcKey::SurvID		default3DSurvID() const;
    static Geometry::ID		cUndefGeomID()	{ return mUdf(Geometry::ID); }

protected:

    void			ensureSIPresent() const;
    void			addGeometry(Geometry&);

    int				indexOf(Geometry::ID) const;
    bool			hasDuplicateLineNames();

    ObjectSet<Geometry>		geometries_;
    static const TrcKey::SurvID	surv2did_;

    bool                        hasduplnms_;

public:

    /*! Admin functions:
      Use the following functions only when you know what you are doing. */

    Geometry*			getGeometry(Geometry::ID);
    bool			write(Geometry&,uiString&);
    IOObj*			createEntry(const char* name,const bool is2d);
				// returns new GeomID.
    bool			removeGeometry(Geometry::ID);

    Geometry::ID		getGeomID(const char* lsm,
					  const char* linenm) const;
				/*! Use only if you are converting 
				    od4 geometries to od5 geometries */
    bool			fetchFrom2DGeom(uiString& errmsg);
				//converts od4 geometries to od5 geometries.
};


mGlobal(Basic) const GeometryManager& GM();
inline mGlobal(Basic) GeometryManager& GMAdmin()
{ return const_cast<GeometryManager&>( Survey::GM() ); }



mExpClass(Basic) GeometryReader
{
public:

    virtual		~GeometryReader()		{};
			mDefineFactoryInClass(GeometryReader,factory);

    virtual bool	read(ObjectSet<Geometry>&,TaskRunner*) const
							{ return true; }
};



mExpClass(Basic) GeometryWriter
{
public:

    virtual		~GeometryWriter()		{};
			mDefineFactoryInClass(GeometryWriter,factory);

    virtual bool	write(Geometry&,uiString&) const { return true; }
    virtual IOObj*	createEntry(const char*) const	{ return 0; }
    virtual bool	removeEntry(const char*) const	{ return 0; }

};


} //namespace Survey


#endif
