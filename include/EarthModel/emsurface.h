#ifndef emsurface_h
#define emsurface_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emsurface.h,v 1.3 2003-05-12 08:19:54 kristofer Exp $
________________________________________________________________________


-*/

#include "emposid.h"
#include "emobject.h"
#include "sets.h"
#include "position.h"

/*!
Rules for surfaces.

A horizon can have many patches that can be used for reversed faults.


     ----------------------
     |                    |
     |       xxxxxx       |
     |     xxxxxxxxxx     |
     |   xx Patch 1 xxx   |
     |  XXXXXXXXXXXXXXX   |
     |                    |
     |                    |
     |     Patch 0        |
     |                    |
     |                    |
     |                    |
     ----------------------

The area marked with x is an area with an reversed fault, and the area with x
is an own patch, while the white area is another patch. In the border between
the patches, the nodes are defined on both patches, with the same coordinate.
In addition, they are also linked together. 
*/

class BinID;
class RowCol;
class CubeSampling;

namespace Geometry
{
    class GridSurface;
};


namespace EarthModel
{
class EMManager;

/*!\brief
The horizon is made up of one or more grids (so they can overlap at faults).
The grids are defined by knot-points in a matrix and the fillstyle inbetween
the knots.
*/

class Surface : public EMObject
{
public:
    int			nrPatches() const;
    PatchID		patchID(int idx) const;
    PatchID		addPatch(bool addtohistory);
    bool		addPatch(PatchID, bool addtohistory);
    			/*!< Return false if the patchid allready exists */
    			
    void		removePatch(EarthModel::PatchID, bool addtohistory);

    void		setPos( PatchID patch, const RowCol&, const Coord3&,
	    			bool autoconnect, bool addtohistory );
    Coord3		getPos(const EarthModel::PosID&) const;
    bool		setPos(const EarthModel::PosID&, const Coord3&,
	    		       bool addtohistory);
    
    int			findPos( const CubeSampling&,
	    			 TypeSet<EarthModel::PosID>* res ) const;

    int			getNeighbors( const EarthModel::PosID& posid, 
	    			      TypeSet<EarthModel::PosID>* res,
	   			      int size=1, bool circle=false ) const;
    			/*!<\param posid	The posid that we want the
						neigbors to
			    \param res		A pointer where the result is
			    			stored
			    \param size		Specifies how far from posid the
						search should continue
			    \param circle	Specifies wether size should
			    			be seen as a radius or the
						half sidelength of a square
			    \return		The number of neigbors found
			*/


    void		getLinkedPos( const EarthModel::PosID& posid,
	    			      TypeSet<EarthModel::PosID>& ) const;

   
    Coord		getCoord( const RowCol& ) const;
    RowCol		getClosestNode( const Coord& ) const;

    const Geometry::GridSurface*		getSurface(PatchID) const;

protected:
    virtual Geometry::GridSurface*		createPatchSurface() const = 0;

    int			findPos( const RowCol& rowcol,
	    			 TypeSet<PosID>& res ) const;
    friend class	EMManager;
    friend class	EMObject;

    			Surface(EMManager&, const MultiID& );
    			~Surface();
    void		cleanUp();

    ObjectSet<Geometry::GridSurface>	surfaces;
    TypeSet<PatchID>			patchids;
};


}; // Namespace


#endif
