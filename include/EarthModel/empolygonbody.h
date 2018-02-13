#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		July 2008
________________________________________________________________________


-*/


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

/*!\brief PolygonBody SurfaceGeometry */

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
    bool		insertKnot(const PosID&,
				   const Coord3& pos,bool addtohistory);
    bool		removeKnot(const PosID&,
				   bool addtohistory);
    Coord3		getPolygonNormal(int polygon) const;

    Geometry::PolygonSurface*
			geometryElement();
    const Geometry::PolygonSurface*
			geometryElement() const;

    ObjectIterator*	createIterator(const TrcKeyZSampling* =0) const;

    Executor*		loader(const SurfaceIODataSelection* s=0);
    Executor*		saver(const SurfaceIODataSelection* s=0,
			       const DBKey* key=0);

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:
    Geometry::PolygonSurface*	createGeometryElement() const;
};


/*!
\brief A Surface polygon Body.
*/

mExpClass(EarthModel) PolygonBody : public Surface, public Body
{   mDefineEMObjFuncs( PolygonBody );
    mODTextTranslationClass( PolygonBody );
public:
    PolygonBodyGeometry&	geometry();
    const PolygonBodyGeometry&	geometry() const;

    const char*			type() const	{ return typeStr(); }
    virtual Executor*		loader();
    virtual Executor*		saver();
    virtual Executor*		saver(IOObj*);

    virtual ImplicitBody*	createImplicitBody(const TaskRunnerProvider&,
						   bool) const;
    bool			getBodyRange(TrcKeyZSampling&);

    DBKey			storageID() const;
    BufferString		storageName() const;

    void			refBody();
    void			unRefBody();

    bool			useBodyPar(const IOPar&);
    void			fillBodyPar(IOPar&) const;

    uiString			getUserTypeStr() const
				{ return tr("Polygon Body"); }

protected:
    const IOObjContext&		getIOObjContext() const;


    friend class		Object;
    friend class		ObjectManager;
    PolygonBodyGeometry		geometry_;
};

} // namespace EM
