#ifndef emfault_h
#define emfault_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		9-04-2002
 RCS:		$Id: emfault.h,v 1.19 2004-08-09 14:09:31 kristofer Exp $
________________________________________________________________________


-*/
#include "emsurface.h"
#include "emsurfacegeometry.h"

namespace Geometry { class MeshSurface; };

namespace EM
{

/*!\brief

*/
class Fault : public EM::Surface
{
protected:
				Fault(EMManager&,const ObjectID&);

    const IOObjContext&		getIOObjContext() const;


    friend class		EMManager;
    friend class		EMObject;

};


class FaultGeometry : public EM::SurfaceGeometry
{
public:
    			FaultGeometry( EM::Fault& );

protected:
    bool			createFromStick(const TypeSet<Coord3>&,float);
    Geometry::MeshSurface*	createSectionSurface(const SectionID&) const;
};


}; // Namespace


#endif
