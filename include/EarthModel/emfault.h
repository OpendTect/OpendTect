#ifndef emfault_h
#define emfault_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		9-04-2002
 RCS:		$Id: emfault.h,v 1.10 2003-07-30 13:47:33 nanne Exp $
________________________________________________________________________


-*/
#include "emsurface.h"


class dgbEMFaultReader;

namespace EM
{

/*!\brief

*/
class Fault : public EM::Surface
{
public:
			Fault( EM::EMManager&, const MultiID &);
			~Fault();

    Executor*		loader();
    bool		isLoaded() const { return surfaces.size(); }
    Executor*		saver();

protected:
    friend			class ::dgbEMFaultReader;
    Geometry::GridSurface*	createPatchSurface(const PatchID&) const;
};

}; // Namespace


#endif
