#ifndef emhorizon_h
#define emhorizon_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emhorizon3d.h,v 1.14 2003-04-22 11:01:38 kristofer Exp $
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
    class GridSurface;
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
*/

class Horizon : public EMObject
{
public:
    int			nrParts() const;
    PartID		partID(int idx) const;
    PartID		addPart(bool addtohistory);
    bool		addPart(PartID, bool addtohistory);
    			/*!< Return false if the partid allready exists */
    			
    void		removePart(EarthModel::PartID, bool addtohistory);

    void		setPos( PartID part, const RowCol&, const Coord3&,
	    			bool autoconnect, bool addtohistory );
    Coord3		getPos(const EarthModel::PosID&) const;
    bool		setPos(const EarthModel::PosID&, const Coord3&,
	    		       bool addtohistory);
    
    int			findPos( const RowCol&,
	    			 TypeSet<EarthModel::PosID>& res ) const;

    Executor*		loader();
    bool		isLoaded() const;
    Executor*		saver();

    bool		import( const Grid& );
    			/*!< Removes all data and sets it to a single-
			     sub-horizon.
			*/

    Coord		getCoord( const RowCol& ) const;
    RowCol		getClosestNode( const Coord& ) const;

    const Geometry::GridSurface*		getSurface(PartID) const;

protected:
    friend class	EMManager;
    friend class	EMObject;
    friend class	::dgbEarthModelHorizonReader;
    friend class	::dgbEarthModelHorizonWriter;

    		Horizon(EMManager&, const MultiID&);
    		~Horizon();
    void	cleanUp();

    void	setTransform( float x1, float y1, float row1, float col1,
	    		      float x2, float y2, float row2, float col2,
			      float x3, float y3, float row3, float col3 );

    float	a11,a12,a13,a21,a22,a23; //Transformation coords
    float	b11,b12,b13,b21,b22,b23; //Reverse transformation coords

    EarthModel::ObjectID			objid;

    ObjectSet<Geometry::Snapped2DSurface>	surfaces;
    TypeSet<PartID>				partids;
};


}; // Namespace


#endif
