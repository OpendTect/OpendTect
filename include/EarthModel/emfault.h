#ifndef emfault_h
#define emfault_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		9-04-2002
 RCS:		$Id: emfault.h,v 1.7 2003-04-22 11:01:38 kristofer Exp $
________________________________________________________________________


-*/
#include "emobject.h"
#include "position.h"

class RowCol;

namespace Geometry
{
    class GridSurfaceImpl;
};

namespace EarthModel
{

/*!\brief

*/
class Fault : public EMObject
{
public:
			Fault( EarthModel::EMManager&, const MultiID &);
			~Fault();

    PosID		setPos( const RowCol&, const Coord3&,bool addtohistory);
    Coord3 		getPos( const EarthModel::PosID&) const;
    bool		setPos( const EarthModel::PosID&, const Coord3&,
				bool addtohistory);

    Executor*		loader();
    bool		isLoaded() const { return surface; }
    Executor*		saver();

    Geometry::GridSurfaceImpl*		getSurface() { return surface; }
    const Geometry::GridSurfaceImpl*	getSurface() const { return surface; }
    
protected:
    Geometry::GridSurfaceImpl*		surface;
};

}; // Namespace


#endif
