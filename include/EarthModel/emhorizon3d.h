#ifndef emhorizon_h
#define emhorizon_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emhorizon3d.h,v 1.16 2003-05-22 08:24:32 kristofer Exp $
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


class dgbEarthModelHorizonReader;
class dgbEarthModelHorizonWriter;
class Grid;

namespace EarthModel
{
class EMManager;

/*!\brief
The horizon is made up of one or more grids (so they can overlap at faults).
The grids are defined by knot-points in a matrix and the fillstyle inbetween
the knots.
*/

class Horizon : public EarthModel::Surface
{
public:
    Executor*		loader();
    Executor*		saver();

    bool		import( const Grid& );
    			/*!< Removes all data and sets it to a single-
			     sub-horizon.
			*/

    Coord		getCoord( const RowCol& node ) const;
    RowCol		getClosestNode( const Coord& pos ) const;

protected:
    friend class	EMManager;
    friend class	EMObject;
    friend class	::dgbEarthModelHorizonReader;
    friend class	::dgbEarthModelHorizonWriter;

    Geometry::GridSurface*	createPatchSurface() const;

	    			Horizon(EMManager&, const MultiID&);
    				~Horizon();

    void	setTransform( float x1, float y1, float row1, float col1,
	    		      float x2, float y2, float row2, float col2,
			      float x3, float y3, float row3, float col3 );

    float	a11,a12,a13,a21,a22,a23; //Transformation coords
    float	b11,b12,b13,b21,b22,b23; //Reverse transformation coords
};


}; // Namespace


#endif
