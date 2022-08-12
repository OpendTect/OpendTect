#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		July 2008
________________________________________________________________________


-*/


#include "earthmodelmod.h"
#include "embody.h"
#include "emsurface.h"
#include "emsurfacegeometry.h"
#include "polygonsurface.h"
#include "tableascio.h"

namespace Table { class FormatDesc; }
template <class T> class SortedList;

namespace EM
{
class PolygonBody;

/*!
\brief PolygonBody SurfaceGeometry
*/

mExpClass(EarthModel) PolygonBodyGeometry : public SurfaceGeometry
{ mODTextTranslationClass(PolygonBodyGeometry)
public:
			PolygonBodyGeometry(PolygonBody&);
			~PolygonBodyGeometry();

    int			nrPolygons() const;
    int			nrKnots(int polygonnr) const;
    bool		insertPolygon(int polygonnr,
				      int firstknot,const Coord3& pos,
				      const Coord3& editnormal,
				      bool addtohistory);
    bool		removePolygon(int polygonnr,
				      bool addtohistory);
    bool		insertKnot(const SubID&,
				   const Coord3& pos,bool addtohistory);
    bool		removeKnot(const SubID&,
				   bool addtohistory);
    const Coord3&	getPolygonNormal(int polygon) const;

    EMObjectIterator*	createIterator(
				const TrcKeyZSampling* =0) const override;

    Geometry::PolygonSurface* geometryElement() override;
    const Geometry::PolygonSurface* geometryElement() const override;

    Executor*		loader(const SurfaceIODataSelection* s=0) override;
    Executor*		saver(const SurfaceIODataSelection* s=0,
			      const MultiID* key=0) override;

    void		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;

// Deprecated public functions
    mDeprecated("Use geometryElement()")
    Geometry::PolygonSurface*
			sectionGeometry(const SectionID&) override
			{ return geometryElement(); }
    mDeprecated("Use geometryElement() const")
    const Geometry::PolygonSurface*
			sectionGeometry(const SectionID&) const override
			{ return geometryElement(); }

    mDeprecated("Use without SectionID")
    int			nrPolygons(const SectionID&) const
			{ return nrPolygons(); }
    mDeprecated("Use without SectionID")
    int			nrKnots(const SectionID&,int polygonnr) const
			{ return nrKnots(polygonnr); }
    mDeprecated("Use without SectionID")
    bool		insertPolygon(const SectionID&,int polygonnr,
				      int firstknot,const Coord3& pos,
				      const Coord3& editnormal,
				      bool addtohistory)
			{ return insertPolygon(polygonnr,firstknot,pos,
					       editnormal,addtohistory); }
    mDeprecated("Use without SectionID")
    bool		removePolygon(const SectionID&,int polygonnr,
				      bool addtohistory)
			{ return removePolygon(polygonnr,addtohistory); }
    mDeprecated("Use without SectionID")
    bool		insertKnot(const SectionID&,const SubID& subid,
				   const Coord3& pos,bool addtohistory)
			{ return insertKnot(subid,pos,addtohistory); }
    mDeprecated("Use without SectionID")
    bool		removeKnot(const SectionID&,const SubID& subid,
				   bool addtohistory)
			{ return removeKnot(subid,addtohistory); }
    mDeprecated("Use without SectionID")
    const Coord3&	getPolygonNormal(const SectionID&,int polygon) const
			{ return getPolygonNormal(polygon); }

protected:
    Geometry::PolygonSurface*	createGeometryElement() const override;
};


/*!
\brief A Surface polygon Body.
*/

mExpClass(EarthModel) PolygonBody : public Surface, public Body
{ mDefineEMObjFuncs( PolygonBody );
public:
    PolygonBodyGeometry&	geometry() override;
    const PolygonBodyGeometry&	geometry() const override;

    const char*			type() const override	{ return typeStr(); }
    Executor*			loader() override;
    Executor*			saver() override;
    Executor*			saver(IOObj*) override;

    ImplicitBody*		createImplicitBody(TaskRunner*,
						   bool) const override;
    bool			getBodyRange(TrcKeyZSampling&) override;

    MultiID			storageID() const override;
    BufferString		storageName() const override;

    void			refBody() override;
    void			unRefBody() override;

    bool			useBodyPar(const IOPar&) override;
    void			fillBodyPar(IOPar&) const override;

    uiString			getUserTypeStr() const override
				{ return tr("Polygon Body"); }

protected:
    const IOObjContext&		getIOObjContext() const override;


    friend class		EMManager;
    friend class		EMObject;
    PolygonBodyGeometry		geometry_;
};

} // namespace EM

