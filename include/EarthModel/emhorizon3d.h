#ifndef emhorizon_h
#define emhorizon_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emhorizon3d.h,v 1.33 2004-08-09 14:09:31 kristofer Exp $
________________________________________________________________________


-*/

#include "emsurface.h"
#include "emsurfacegeometry.h"

/*!
Rules for horizons.

A horizon can have many sections that can be used for reversed faults.


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
is an own section, while the white area is another section. In the border between
the sections, the nodes are defined on both sections, with the same coordinate.
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
				  sections for every Grid imported.
				*/

protected:
	    			Horizon(EMManager&,const ObjectID&);
    const IOObjContext&		getIOObjContext() const;

    friend class		EMManager;
    friend class		EMObject;
};


class HorizonGeometry : public SurfaceGeometry
{
public:
    				HorizonGeometry( EM::Surface& );

    static BinID		getBinID(const EM::SubID&);
    static BinID		getBinID(const RowCol&);
    static EM::SubID		getSubID(const BinID&);
    static RowCol		getRowCol(const BinID&);

    bool			createFromStick(const TypeSet<Coord3>&,float);

protected:
    Geometry::MeshSurface*	createSectionSurface(const SectionID&) const;
};


}; // Namespace


#endif
