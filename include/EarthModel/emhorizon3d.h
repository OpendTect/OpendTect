#ifndef emhorizon_h
#define emhorizon_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emhorizon3d.h,v 1.3 2002-05-21 13:44:44 kristofer Exp $
________________________________________________________________________


-*/

#include "emposid.h"
#include "emobject.h"
#include "sets.h"
#include "geompos.h"

namespace Geometry
{
    class CompositeGridSurface;
    class TriangleStripSet;
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
    int			findPos( int inl, int crl, TypeSet<PosID>& res ) const;
    void		addSquare( int inl, int crl,
	    			   float inl0crl0, float inl0crl1,
				   float inl1crl0, float inl1crl1 );

    static unsigned short	getSurfID(PosID);
    static unsigned long	getSurfPID(PosID);
    PosID			getPosID(unsigned short surfid,
	    				 unsigned long  surfpid ) const;
    Geometry::Pos		getPos(PosID);


    Executor*		loader();
    bool		isLoaded() const;
    Executor*		saver();

    bool		import( const Grid& );
    			/*!< Removes all data and sets it to a single-
			     sub-horizon.
			*/

    void		getTriStrips( Geometry::TriangleStripSet* ) const;
    const Geometry::CompositeGridSurface&	getSurfaces() const
    						{ return surfaces; }
    Geometry::CompositeGridSurface&		getSurfaces(){return surfaces;}

protected:
    			friend EMManager;
			friend EMObject;

    			Horizon(EMManager&, int id);
    			~Horizon();


    Geometry::CompositeGridSurface&	surfaces;	
};

}; // Namespace


#endif
