#ifndef emhorizon_h
#define emhorizon_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emhorizon3d.h,v 1.21 2003-07-29 13:14:19 nanne Exp $
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

class BinID;
class RowCol;


class dgbEMHorizonReader;
class dgbEMHorizonWriter;
class Grid;

namespace EM
{
class EMManager;
class SurfaceIODataSelection;

/*!\brief
The horizon is made up of one or more grids (so they can overlap at faults).
The grids are defined by knot-points in a matrix and the fillstyle inbetween
the knots.
*/

class Horizon : public EM::Surface
{
public:
    Executor*		loader(const EM::SurfaceIODataSelection* s=0,
	    		       bool auxdataonly=false);
    Executor*		saver(const EM::SurfaceIODataSelection* s=0,
	    		      bool auxdataonly=false);

    Executor*		import( const Grid& );
    			/*!< Removes all data and sets it to a single-
			     sub-horizon.
			*/

    static BinID	getBinID( const EM::SubID& );
    static BinID	getBinID( const RowCol& );

protected:
    friend class	EMManager;
    friend class	EMObject;
    friend class	::dgbEMHorizonReader;
    friend class	::dgbEMHorizonWriter;

    Geometry::GridSurface*	createPatchSurface() const;

	    			Horizon(EMManager&, const MultiID&);
    				~Horizon();


    float	a11,a12,a13,a21,a22,a23; //Transformation coords
    float	b11,b12,b13,b21,b22,b23; //Reverse transformation coords
};


}; // Namespace


#endif
