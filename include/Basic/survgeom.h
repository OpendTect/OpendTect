#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
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

namespace Survey
{
class Geometry2D;
class Geometry3D;

/*!\brief A Geometry which holds trace positions.

For 3D, a geometry is an Inl/Crl System.
For 2D, each line has its own Geometry.

Beware, the Pos::GeomID != Pos::SurvID for 2D geometries. The GeomID
will end up in the lineNr() of the TrcKey.
*/

mExpClass(Basic) Geometry
{ mRefCountImpl(Geometry)
public:

    enum RelationType	{ UnRelated=0, Related, SubSet, SuperSet, Identical };
			/*!< 'Related' means the two geometries have the same
			  transform but neither includes the other. */

    virtual bool	is2D() const			= 0;
    Pos::SurvID		getSurvID() const;
    static const Geometry& default3D();

    Pos::GeomID		getID() const			{ return id_; }
    void		setID( Pos::GeomID id )		{ id_ = id; }
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
    virtual TrcKey	nearestTrace(const Coord&,
				     float* distance=nullptr) const = 0;

    const TrcKeyZSampling&	sampling() const	{ return sampling_; }

    virtual float		averageTrcDist() const			= 0;
    virtual RelationType	compare(const Geometry&,bool usezrg) const
				{ return UnRelated; }

    //Convenience functions for the most commone geometries
    virtual Geometry2D*		as2D()			{ return nullptr; }
    const Geometry2D*		as2D() const;

    virtual Geometry3D*		as3D()			{ return nullptr; }
    const Geometry3D*		as3D() const;

protected:

				Geometry();

    TrcKeyZSampling		sampling_;
private:

    Pos::GeomID			id_;

};


/*!\brief Makes geometries accessible from a geometry ID, or a MultiID.  */

mExpClass(Basic) GeometryManager
{ mODTextTranslationClass(GeometryManager)
public:

				GeometryManager();
				~GeometryManager();

    const Geometry*		getGeometry(Pos::GeomID) const;
    const Geometry*		getGeometry(const char*) const;
    const Geometry*		getGeometry(const MultiID&) const;

    const Geometry3D*		getGeometry3D(Pos::SurvID) const;

    const Geometry2D&		get2D(Pos::GeomID) const;

    int				nrGeometries() const;
    bool			isUsable(Pos::GeomID) const;

    Pos::GeomID			getGeomID(const char* linenm) const;
    const char*			getName(Pos::GeomID) const;
    StepInterval<float>		zRange(Pos::GeomID) const;

    Coord			toCoord(const TrcKey&) const;

    TrcKey			traceKey(Pos::GeomID,Pos::LineID,
					 Pos::TraceID) const;
				//!<For 3D
    TrcKey			traceKey(Pos::GeomID,Pos::TraceID) const;
				//!<For 2D
    TrcKey			nearestTrace(const Coord&,bool is2d,
					     float* dist=nullptr) const;

    bool			fillGeometries(TaskRunner*);
    bool			getList(BufferStringSet& names,
					TypeSet<Pos::GeomID>& ids,
					bool is2d) const;
    Pos::GeomID			findRelated(const Geometry&,
					    Geometry::RelationType&,
					    bool usezrg) const;
				//!<Returns cUndefGeomID() if none found

    static TrcKey::SurvID	get2DSurvID()	{ return surv2did_; }
    static TrcKey::SurvID	get3DSurvID()	{ return surv3did_; }
    static Pos::GeomID		cUndefGeomID()	{ return mUdf(Pos::GeomID); }

    mDeprecated("Use get3DSurvID()")
    TrcKey::SurvID		default3DSurvID() const;

protected:

    void			ensureSIPresent() const;
    void			addGeometry(Geometry&);

    int				indexOf(Pos::GeomID) const;
    bool			hasDuplicateLineNames();

    Threads::Lock		lock_;
    ObjectSet<Geometry>		geometries_;
    static const TrcKey::SurvID	surv2did_;
    static const TrcKey::SurvID	surv3did_;

    bool			hasduplnms_;

public:

    /*! Admin functions:
      Use the following functions only when you know what you are doing. */

    Geometry*			getGeometry(Pos::GeomID);
    bool			write(Geometry&,uiString&);
    Pos::GeomID			addNewEntry(Geometry*,uiString&);
				/*! Returns new GeomID. */
    bool			removeGeometry(Pos::GeomID);

    Pos::GeomID			getGeomID(const char* lsm,
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

mGlobal(Basic) bool is2DGeom(Pos::GeomID);
mGlobal(Basic) bool is3DGeom(Pos::GeomID);
mGlobal(Basic) bool isSynthetic(Pos::GeomID);
mGlobal(Basic) Pos::GeomID default3DGeomID();
mGlobal(Basic) bool isValidGeomID(Pos::GeomID);


mExpClass(Basic) GeometryReader
{
public:

    virtual		~GeometryReader()		{}
			mDefineFactoryInClass(GeometryReader,factory)

    virtual bool	read(ObjectSet<Geometry>&,TaskRunner*) const
							{ return true; }
    virtual bool	updateGeometries(ObjectSet<Geometry>&,TaskRunner*) const
							{ return true; }
};



mExpClass(Basic) GeometryWriter
{
public:

    virtual		~GeometryWriter()		{}
			mDefineFactoryInClass(GeometryWriter,factory)

    virtual bool	write(Geometry&,uiString&,
			      const char* crfromstr=0) const { return true; }
    virtual IOObj*	createEntry(const char*) const	{ return 0; }
    virtual Pos::GeomID createNewGeomID(const char*) const { return 0; }
    virtual bool	removeEntry(const char*) const	{ return 0; }

};

} //namespace Survey


#define mUdfGeomID		Survey::GeometryManager::cUndefGeomID()
#define mIsUdfGeomID(geomid)	(geomid == mUdfGeomID)
//To cut the long story short.

