#ifndef emfault_h
#define emfault_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		9-04-2002
 RCS:		$Id: emfault.h,v 1.18 2004-07-23 12:54:54 kristofer Exp $
________________________________________________________________________


-*/
#include "emsurface.h"

namespace Geometry { class MeshSurface; };

namespace EM
{

/*!\brief

*/
class Fault : public EM::Surface
{
public:
    bool			createFromStick(const TypeSet<Coord3>&,float);

protected:
				Fault(EMManager&,const ObjectID&);

    Geometry::MeshSurface*	createSectionSurface(const SectionID&) const;
    const IOObjContext&		getIOObjContext() const;


    friend class		EMManager;
    friend class		EMObject;

};

}; // Namespace


#endif
