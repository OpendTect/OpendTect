#ifndef emfault_h
#define emfault_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		9-04-2002
 RCS:		$Id: emfault.h,v 1.8 2003-05-05 12:02:15 kristofer Exp $
________________________________________________________________________


-*/
#include "emsurface.h"


class dgbEarthModelFaultReader;

namespace EarthModel
{

/*!\brief

*/
class Fault : public EarthModel::Surface
{
public:
			Fault( EarthModel::EMManager&, const MultiID &);
			~Fault();

    Executor*		loader();
    bool		isLoaded() const { return surfaces.size(); }
    Executor*		saver();

protected:
    friend			class ::dgbEarthModelFaultReader;
    Geometry::GridSurface*	createPatchSurface() const;
};

}; // Namespace


#endif
