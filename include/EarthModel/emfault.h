#ifndef emfault_h
#define emfault_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		9-04-2002
 RCS:		$Id: emfault.h,v 1.11 2003-08-13 10:13:55 nanne Exp $
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
			Fault( EM::EMManager&, const MultiID &);
			~Fault();

    bool		isLoaded() const { return surfaces.size(); }
    Executor*		loader(const EM::SurfaceIODataSelection* s=0,
	    		       bool auxdataonly=false);
    Executor*		saver(const EM::SurfaceIODataSelection* s=0,
	    		      bool auxdataonly=false);

protected:
    friend			class ::dgbEMFaultReader;
    Geometry::GridSurface*	createPatchSurface(const PatchID&) const;
};

}; // Namespace


#endif
