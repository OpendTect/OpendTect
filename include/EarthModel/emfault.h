#ifndef emfault_h
#define emfault_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		9-04-2002
 RCS:		$Id: emfault.h,v 1.14 2003-10-03 06:32:09 nanne Exp $
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
	    		       int attridx=-1);
    Executor*		saver(const EM::SurfaceIODataSelection* s=0,
	    		      bool auxdataonly=false,const MultiID* key=0);

protected:
    friend			class ::dgbEMFaultReader;
    Geometry::MeshSurface*	createPatchSurface(const PatchID&) const;
};

}; // Namespace


#endif
