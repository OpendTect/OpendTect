#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"

#include "coord.h"
#include "factory.h"
#include "refcount.h"
#include "trckey.h"
#include "trckeyzsampling.h"
#include "threadlock.h"

#include <unordered_map>

class TaskRunner;
class IOObj;

namespace Survey
{
class Geometry2D;
class Geometry3D;

/*!\brief A Geometry which holds trace positions.

For 3D, a geometry is an Inl/Crl System.
For 2D, each line has its own Geometry.

Beware, the Pos::GeomID != OD::GeomSytem for 2D geometries. The GeomID
will end up in the lineNr() of the TrcKey.
*/

mExpClass(Basic) Geometry : public ReferencedObject
{
public:

    enum RelationType	{ UnRelated=0, Related, SubSet, SuperSet, Identical };
			/*!< 'Related' means the two geometries have the same
			  transform but neither includes the other. */

    virtual bool	is2D() const			= 0;
    OD::GeomSystem	geomSystem() const;

    mDeprecated("Use Survey::Geometry3D::instance()")
    static const Geometry& default3D();

    Pos::GeomID		getID() const			{ return id_; }
    void		setID( const Pos::GeomID& id )	{ id_ = id; }
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

    //Convenience functions for the most common geometries
    virtual Geometry2D*		as2D()			{ return nullptr; }
    const Geometry2D*		as2D() const;

    virtual Geometry3D*		as3D()			{ return nullptr; }
    const Geometry3D*		as3D() const;

protected:
				Geometry();
    virtual			~Geometry();

    TrcKeyZSampling		sampling_;

private:

    Pos::GeomID			id_;

public:

    mDeprecated("use geomSystem")
    OD::GeomSystem	getSurvID() const;

};


/*!\brief Makes geometries accessible from a geometry ID, or a MultiID.  */

mExpClass(Basic) GeometryManager : public CallBacker
{
mODTextTranslationClass(GeometryManager)
public:
				GeometryManager();
				~GeometryManager();

    const Geometry*		getGeometry(const Pos::GeomID&) const;
    const Geometry*		getGeometry(const char*) const;
    const Geometry*		getGeometry(const MultiID&) const;

    mDeprecated("Use Survey::Geometry3D::instance()")
    const Geometry3D*		getGeometry3D(OD::GeomSystem) const;

    const Geometry2D&		get2D(const Pos::GeomID&) const;

    int				nrGeometries() const;
    bool			has2D() const;
    bool			isUsable(const Pos::GeomID&) const;

    Pos::GeomID			getGeomID(const char* linenm) const;
    const char*			getName(const Pos::GeomID&) const;
    Pos::GeomID			default2DGeomID() const;
    StepInterval<float>		zRange(const Pos::GeomID&) const;

    Coord			toCoord(const TrcKey&) const;

    TrcKey			nearestTrace(const Coord&,bool is2d,
					     float* dist=nullptr) const;
    TrcKey			nearestTrace(const TypeSet<Pos::GeomID>&,
					const Coord&,float* dist=nullptr) const;

    bool			getList(BufferStringSet& names,
					TypeSet<Pos::GeomID>& ids,
					bool is2d) const;
    Pos::GeomID			findRelated(const Geometry&,
					    Geometry::RelationType&,
					    bool usezrg) const;
				//!<Returns GeomID::udf() if none found

    Notifier<GeometryManager>	geometryRead;

    mDeprecated("Use Pos::GeomID::udf()")
    static Pos::GeomID		cUndefGeomID()	{ return Pos::GeomID::udf(); }

protected:

    const Geometry*		getGeometry(OD::GeomSystem) const  = delete;
    void			addGeometry(Geometry&);

    int				indexOf(const Pos::GeomID&) const;
    bool			hasDuplicateLineNames();

    mutable Threads::Lock	lock_;
    RefObjectSet<Geometry>	geometries_;

    bool			hasduplnms_	= false;

public:

    /*! Admin functions:
      Use the following functions only when you know what you are doing. */
    bool			fillGeometries(TaskRunner*);
    void			ensureSIPresent();

    Geometry*			getGeometry(const Pos::GeomID&);
    Geometry2D&			get2D(const Pos::GeomID&);
    bool			write(Geometry&,uiString&);
    Pos::GeomID			addNewEntry(Geometry*,uiString&);
				/*! Returns new GeomID. */
    bool			removeGeometry(const Pos::GeomID&);

    Pos::GeomID			getGeomID(const char* lsm,
					  const char* linenm) const;
				/*!< Use only if you are converting
				    od4 geometries to od5 geometries */
    bool			fetchFrom2DGeom(uiString& errmsg);
				//converts od4 geometries to od5 geometries.
    bool			updateGeometries(TaskRunner*);

    Notifier<GeometryManager>	closing;

public:

    mDeprecated("Use TrcKey class")
    TrcKey			traceKey(Pos::GeomID,Pos::LineID,
					 Pos::TraceID) const;
				//!<For 3D
    mDeprecated("Use TrcKey class")
    TrcKey			traceKey(Pos::GeomID,Pos::TraceID) const;
				//!<For 2D

    mDeprecated("Use OD::Geom2D")
    static TrcKey::SurvID	get2DSurvID()	{ return OD::Geom2D; }
    mDeprecated("Use OD::Geom3D")
    static TrcKey::SurvID	get3DSurvID()	{ return OD::Geom3D; }

    mDeprecated("Use OD::Geom3D")
    TrcKey::SurvID		default3DSurvID() const
						{ return OD::Geom3D; }

private:

    std::unordered_map<std::string,int>		namemap_;
    std::unordered_map<int,int>			geomidmap_;
};


mGlobal(Basic) const GeometryManager& GM();
inline mGlobal(Basic) GeometryManager& GMAdmin()
{ return const_cast<GeometryManager&>( Survey::GM() ); }

mGlobal(Basic) bool is2DGeom(const Pos::GeomID&);
mGlobal(Basic) bool is3DGeom(const Pos::GeomID&);
mGlobal(Basic) bool isSynthetic(const Pos::GeomID&);
mGlobal(Basic) Pos::GeomID default3DGeomID();
mGlobal(Basic) Pos::GeomID getDefault2DGeomID();
mGlobal(Basic) bool isValidGeomID(const Pos::GeomID&);
mGlobal(Basic) void sortByLinename(TypeSet<Pos::GeomID>&,
				   BufferStringSet* lnms=nullptr);


mExpClass(Basic) GeometryReader
{
public:
    virtual		~GeometryReader();
			mOD_DisableCopy(GeometryReader)

			mDefineFactoryInClass(GeometryReader,factory)

    virtual bool	read(RefObjectSet<Geometry>&,TaskRunner*) const;
    virtual bool	read(RefObjectSet<Geometry>&,const ObjectSet<IOObj>&,
			     TaskRunner*) const;
    virtual bool	updateGeometries(RefObjectSet<Geometry>&,
					 TaskRunner*) const;

protected:
			GeometryReader();
};



mExpClass(Basic) GeometryWriter
{
public:
    virtual		~GeometryWriter();
			mOD_DisableCopy(GeometryWriter)

			mDefineFactoryInClass(GeometryWriter,factory)

    virtual bool	write(Geometry&,uiString&,
			      const char* crfromstr=nullptr) const;
    virtual IOObj*	createEntry(const char*) const;
    virtual Pos::GeomID createNewGeomID(const char*) const;
    virtual bool	removeEntry(const char*) const;

protected:
			GeometryWriter();
};

} // namespace Survey
