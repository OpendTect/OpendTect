#ifndef emfault_h
#define emfault_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		9-04-2002
 RCS:		$Id: emfault.h,v 1.13 2003-09-30 12:56:34 kristofer Exp $
________________________________________________________________________


-*/
#include "emsurface.h"


class dgbEMFaultReader;

namespace EM
{
class SurfaceIODataSelection;

/*!\brief

*/
class Fault : public EM::Surface
{
public:
			Fault( EM::EMManager&, const MultiID&);
			~Fault();

    bool		isLoaded() const { return surfaces.size(); }
    Executor*		loader(const EM::SurfaceIODataSelection* s=0,
	    		       bool auxdataonly=false);
    Executor*		saver(const EM::SurfaceIODataSelection* s=0,
	    		      bool auxdataonly=false,const MultiID* key=0);

protected:
    friend			class ::dgbEMFaultReader;
    Geometry::MeshSurface*	createPatchSurface(const PatchID&) const;
};

}; // Namespace


#endif
