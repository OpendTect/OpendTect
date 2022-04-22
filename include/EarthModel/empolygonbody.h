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

    int			nrPolygons(const SectionID&) const;
    int			nrKnots(const SectionID&,int polygonnr) const;
    bool		insertPolygon(const SectionID&,int polygonnr,
				      int firstknot,const Coord3& pos,
				      const Coord3& editnormal,
				      bool addtohistory);
    bool		removePolygon(const SectionID&,int polygonnr,
				      bool addtohistory);
    bool		insertKnot(const SectionID&,const SubID&,
				   const Coord3& pos,bool addtohistory);
    bool		removeKnot(const SectionID&,const SubID&,
				   bool addtohistory);
    const Coord3&	getPolygonNormal(const SectionID&,int polygon) const;

    Geometry::PolygonSurface*
			sectionGeometry(const SectionID&) override;
    const Geometry::PolygonSurface*
			sectionGeometry(const SectionID&) const override;

    EMObjectIterator*	createIterator(const SectionID&,
				   const TrcKeyZSampling* =0) const override;

    Executor*		loader(const SurfaceIODataSelection* s=0) override;
    Executor*		saver(const SurfaceIODataSelection* s=0,
			       const MultiID* key=0) override;

    void		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;

protected:
    Geometry::PolygonSurface*	createSectionGeometry() const override;
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

