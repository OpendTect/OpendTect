#ifndef emhorizon_h
#define emhorizon_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emhorizon3d.h,v 1.31 2004-07-14 15:33:59 nanne Exp $
________________________________________________________________________


-*/

#include "emsurface.h"

/*!
Rules for horizons.

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

class Grid;

namespace Geometry { class MeshSurface; };

namespace EM
{

/*!\brief
The horizon is made up of one or more grids (so they can overlap at faults).
The grids are defined by knot-points in a matrix and the fillstyle inbetween
the knots.
*/

class Horizon : public EM::Surface
{
public:
    Executor*			import(const Grid&,int idx,bool fixholes);
    				/*!< Removes all data when idx=0 and creates 
				  patches for every Grid imported.
				*/

    BinID			getBinID(const EM::SubID&);
    BinID			getBinID(const RowCol&);
    EM::SubID			getSubID(const BinID&);
    RowCol			getRowCol(const BinID&);

    bool			createFromStick(const TypeSet<Coord3>&,float);

protected:
	    			Horizon(EMManager&,const ObjectID&);

    Geometry::MeshSurface*	createPatchSurface(const PatchID&) const;
    const IOObjContext&		getIOObjContext() const;

    friend class		EMManager;
    friend class		EMObject;
};


}; // Namespace


#endif
