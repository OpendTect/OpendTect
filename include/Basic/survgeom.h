#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		9-4-1996
________________________________________________________________________

-*/

#include "factory.h"
#include "coord.h"
#include "refcount.h"
#include "threadlock.h"
#include "trckey.h"
#include "trckeyzsampling.h"

class TaskRunner;
class IOObj;

// Need some shortcuts because GeomID is not a real object:
#define mUdfGeomID		Survey::GeometryManager::cUndefGeomID()
#define mIsUdfGeomID(geomid)	(geomid == mUdfGeomID)


namespace Survey
{
class Geometry2D;
class Geometry3D;

/*!\brief A Geometry which holds trace positions.

For 3D, a geometry is an Inl/Crl System.
For 2D, each line has its own Geometry.

Beware, the Geometry::ID != Survkey::ID for 2D geometries. The Geometry::ID
will end up in the lineNr() of the TrcKey.

*/

mExpClass(Basic) Geometry : public RefCount::Referenced
			  , public ObjectWithName
{
public:

    typedef Pos::GeomID	ID;

    enum RelationType	{ UnRelated=0, Related, SubSet, SuperSet, Identical };
			/*!< 'Related' means the two geometries have the same
			  transform but neither includes the other. */
    virtual RelationType compare(const Geometry&,bool usezrg) const = 0;
    bool		isCompatibleWith(const Geometry&) const;

    virtual bool	is2D() const			= 0;
    Pos::SurvID		getSurvID() const;
    static const Geometry& default3D();

    inline ID		id() const			{ return id_; }
    void		setID( ID id )			{ id_ = id; }

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

    TrcKeyZSampling&	sampling()		{ return sampling_; }
    const TrcKeyZSampling& sampling() const	{ return sampling_; }

    virtual float	averageTrcDist() const			= 0;

    // Convenience functions
    virtual Geometry2D*	as2D()			{ return 0; }
    const Geometry2D*	as2D() const;

    virtual Geometry3D*	as3D()			{ return 0; }
    const Geometry3D*	as3D() const;

protected:
			~Geometry();
			Geometry();

    TrcKeyZSampling	sampling_;

private:

    ID			id_;

public:

    mDeprecated ID	getID() const		{ return id(); }
    mDeprecated const char* getName() const	{ return name().str(); }

};


/*!\brief Makes geometries accessible from a geometry ID, or a DBKey.  */

mExpClass(Basic) GeometryManager
{ mODTextTranslationClass(GeometryManager)
public:

    typedef Geometry::ID	ID;

				GeometryManager();
				~GeometryManager();

    const Geometry*		getGeometry(ID) const;
    const Geometry*		getGeometry(const char*) const;
    const Geometry*		getGeometry(const DBKey&) const;

    const Geometry3D*		getGeometry3D(Pos::SurvID) const;

    int				nrGeometries() const;

    ID				getGeomID(const char* linenm) const;
    const char*			getName(ID) const;
    bool			is2D(ID) const;

    Coord			toCoord(const TrcKey&) const;

    TrcKey			traceKey(ID,Pos::LineID,
					 Pos::TraceID) const;
				//!<For 3D
    TrcKey			traceKey(ID,Pos::TraceID) const;
				//!<For 2D

    bool			fillGeometries(TaskRunner*);
    bool			getList(BufferStringSet& names,
					TypeSet<ID>& ids,
					bool is2d) const;
    ID				findRelated(const Geometry&,
					    Geometry::RelationType&,
					    bool usezrg) const;
				//!<Returns cUndefGeomID() if none found

    static TrcKey::SurvID	get2DSurvID()	{ return surv2did_; }
    TrcKey::SurvID		default3DSurvID() const;
    TrcKey::SurvID		synthSurvID() const;
    static ID			cUndefGeomID()	{ return mUdf(ID); }

protected:

    void			ensureSIPresent() const;
    void			addGeometry(Geometry&);

    int				indexOf(ID) const;
    bool			hasDuplicateLineNames();

    Threads::Lock		lock_;
    ObjectSet<Geometry>		geometries_;
    static const TrcKey::SurvID	surv2did_;

    bool			hasduplnms_;

public:

    /*! Admin functions:
      Use the following functions only when you know what you are doing. */

    Geometry*			getGeometry(ID);
    bool			write(Geometry&,uiString&);
    ID		addNewEntry(Geometry*,uiString&);
				/*! Returns new GeomID. */
    bool			removeGeometry(ID);

    ID		getGeomID(const char* lsm,
					  const char* linenm) const;
				/*!< Use only if you are converting
				    od4 geometries to od5 geometries */
    bool			fetchFrom2DGeom(uiString& errmsg);
				//converts od4 geometries to od5 geometries.
    bool			updateGeometries(TaskRunner*);
};


mGlobal(Basic) const GeometryManager& GM();
inline mGlobal(Basic) GeometryManager& GMAdmin()
{ return const_cast<GeometryManager&>( Survey::GM() ); }



mExpClass(Basic) GeometryReader
{
public:

    virtual		~GeometryReader()		{}
			mDefineFactoryInClass(GeometryReader,factory)

    virtual bool	read(ObjectSet<Geometry>&,TaskRunner*) const	= 0;
    virtual bool	updateGeometries(ObjectSet<Geometry>&,
					 TaskRunner*) const		= 0;
};



mExpClass(Basic) GeometryWriter
{
public:

    typedef Geometry::ID GeometryID;

    virtual		~GeometryWriter()		{}
			mDefineFactoryInClass(GeometryWriter,factory)

    virtual bool	write(const Geometry&,uiString&,
			      const char* crfromstr=0) const	= 0;
    virtual IOObj*	createEntry(const char*) const		= 0;
    virtual GeometryID	createNewGeomID(const char*) const	= 0;

};

} //namespace Survey
