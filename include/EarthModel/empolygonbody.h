#ifndef empolygonbody_h
#define empolygonbody_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		July 2008
 RCS:		$Id$
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
{
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
			sectionGeometry(const SectionID&);
    const Geometry::PolygonSurface*
			sectionGeometry(const SectionID&) const;

    EMObjectIterator*	createIterator(const SectionID&,
	    			       const CubeSampling* =0) const;

    Executor*		loader(const SurfaceIODataSelection* s=0);
    Executor*		saver(const SurfaceIODataSelection* s=0,
	    		       const MultiID* key=0);

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:
    Geometry::PolygonSurface*	createSectionGeometry() const;
};


/*!
\brief A Surface polygon Body.
*/

mExpClass(EarthModel) PolygonBody : public Surface, public Body
{ mDefineEMObjFuncs( PolygonBody );
public:
    PolygonBodyGeometry&	geometry();
    const PolygonBodyGeometry&	geometry() const;

    Executor*			saver();
    Executor*			loader();
    Executor*                   saver(IOObj*);

    ImplicitBody*		createImplicitBody(TaskRunner*,bool) const;
    bool			getBodyRange(CubeSampling&);

    MultiID			storageID() const;
    BufferString		storageName() const;

    void			refBody();
    void			unRefBody();

    bool			useBodyPar(const IOPar&);
    void			fillBodyPar(IOPar&) const;

protected:
    const IOObjContext&		getIOObjContext() const;


    friend class		EMManager;
    friend class		EMObject;
    PolygonBodyGeometry		geometry_;
};


} // namespace EM


#endif

