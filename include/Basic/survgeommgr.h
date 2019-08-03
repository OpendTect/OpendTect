#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		9-4-1996
________________________________________________________________________

-*/

#include "coord.h"
#include "factory.h"
#include "survgeom.h"
#include "threadlock.h"

class TaskRunnerProvider;


namespace Survey
{

class GeometryManager;
class Geometry2DWriter;

mGlobal(Basic) const SurvGM& GM();
inline mGlobal(Basic) SurvGM& GMAdmin()
{ return const_cast<SurvGM&>( Survey::GM() ); }


/*!\brief Provides Geometry objects from a geometry ID, name, or DBKey

  This class is thread-'safe' only, not really thread-correct or comfortable.
  As long as you do not remove Geometries in MT situations, you should be OK
  though.

 */

mExpClass(Basic) GeometryManager
{ mODTextTranslationClass(GeometryManager)
public:

    typedef ObjectSet<Geometry2D>	Geom2DSet;
    mUseType( Geom2DSet,		idx_type );
    mUseType( Geom2DSet,		size_type );

			GeometryManager();
			~GeometryManager();

    const Geometry*	getGeometry( GeomID gid ) const
			{ return gtGeometry( gid ); }
    const Geometry3D&	get3DGeometry() const;
    const Geometry*	getGeometry(const char* nm) const;
    const Geometry2D*	get2DGeometry(GeomID) const;
    const Geometry2D*	get2DGeometry(const char* linenm) const;

    size_type		nr2DGeometries() const;
    idx_type		indexOf(GeomID) const;
    const Geometry2D*	get2DGeometryByIdx(idx_type) const;

    GeomID		getGeomID(const char* linenm) const;
    const char*		getName(GeomID) const;
    GeomID		default2DGeomID() const;

    bool		fillGeometries(const TaskRunnerProvider&);
    void		list2D(GeomIDSet&,BufferStringSet* nms=0) const;

protected:

    idx_type		gtIndexOf(GeomID) const;
    Geometry*		gtGeometry(GeomID) const;
    void		doAddGeometry(Geometry2D&);

    mutable Threads::Lock geometrieslock_;
    Geom2DSet		geometries_;
    static BufferString	factorykey_;

public:

    GeomID	findRelated(const Geometry&,Geometry::RelationType&,
			    bool usezrg) const;

    /*! Admin functions:
      Use the following functions only when you know what you are doing. */

    bool	save(const Geometry2D&,uiString&,Geometry2DWriter* wrr=0) const;
    bool	addEntry(Geometry2D*,GeomID&,uiString&);
    bool	removeGeometry(GeomID);

    bool	updateGeometries(const TaskRunnerProvider&) const;

};


mExpClass(Basic) Geometry2DReader
{
public:

    virtual		~Geometry2DReader()		{}
			mDefineFactoryInClass(Geometry2DReader,factory)

    virtual bool	read(ObjectSet<Geometry2D>&,
				       const TaskRunnerProvider&) const	  = 0;
    virtual bool	updateGeometries(ObjectSet<Geometry2D>&,
					 const TaskRunnerProvider&) const = 0;

};



mExpClass(Basic) Geometry2DWriter
{
public:

    virtual		~Geometry2DWriter()		{}
			mDefineFactoryInClass(Geometry2DWriter,factory)

    virtual bool	write(const Geometry2D&,uiString&,
			      const char* crfromstr=0) const	= 0;
    virtual GeomID	getGeomIDFor(const char*) const		= 0;

};

} //namespace Survey
