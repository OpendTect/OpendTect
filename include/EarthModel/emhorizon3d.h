#ifndef emhorizon_h
#define emhorizon_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emhorizon3d.h,v 1.12 2002-10-14 13:45:21 niclas Exp $
________________________________________________________________________


-*/

#include "emposid.h"
#include "emobject.h"
#include "sets.h"
#include "position.h"

class BinID;
class RowCol;

namespace Geometry
{
    class CompositeGridSurface;
    class TriangleStripSet;
    class Snapped2DSurface;
};


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

The posid is is interpreted such as bit 48-33 gives the gridid while bit
0-32 gives the id on the grid.
*/

class Horizon : public EMObject
{
public:
    enum FillType	{ Empty, LowLow, LowHigh, HighLow, HighHigh, Full };
    int			findPos( const RowCol&, TypeSet<PosID>& res ) const;
    void		addSquare( const RowCol&,
	    			   float inl0crl0, float inl0crl1,
				   float inl1crl0, float inl1crl1 );

    static unsigned short	getSurfID(PosID);
    static unsigned long	getSurfPID(PosID);
    PosID			getPosID(unsigned short surfid,
	    				 unsigned long  surfpid ) const;
    Coord3		getPos(PosID);

    Executor*		loader();
    bool		isLoaded() const;
    Executor*		saver();

    bool		import( const Grid& );
    			/*!< Removes all data and sets it to a single-
			     sub-horizon.
			*/

    Coord		getCoord( const RowCol& ) const;
    RowCol		getClosestNode( const Coord& ) const;

    const Geometry::CompositeGridSurface&	getSurfaces() const
    						{ return surfaces; }
    Geometry::CompositeGridSurface&		getSurfaces(){return surfaces;}

protected:
    friend class	EMManager;
    friend class	EMObject;
    friend class	dgbEarthModelHorizonReader;
    friend class	dgbEarthModelHorizonWriter;

    		Horizon(EMManager&, const MultiID&);
    		~Horizon();

    void	setTransform( float x1, float y1, float row1, float col1,
	    		      float x2, float y2, float row2, float col2,
			      float x3, float y3, float row3, float col3 );

    float	a11,a12,a13,a21,a22,a23; //Transformation coords
    float	b11,b12,b13,b21,b22,b23; //Reverse transformation coords

    Geometry::CompositeGridSurface&	surfaces;
};

}; // Namespace


#endif
