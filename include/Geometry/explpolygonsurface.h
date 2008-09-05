#ifndef explpolygonsurface_h
#define explpolygonsurface_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Yuancheng Liu
 Date:          July 2008
 RCS:           $Id: explpolygonsurface.h,v 1.2 2008-09-05 21:23:05 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "indexedshape.h"
#include "position.h"

class DAGTetrahedraTree;

namespace Geometry
{

class PolygonSurface;

/*!A triangulated representation of a polygonsurface */


class ExplPolygonSurface: public Geometry::IndexedShape, public CallBacker
{
public:
			ExplPolygonSurface(const PolygonSurface*);
    			~ExplPolygonSurface();

    void		display(bool polygons,bool body);
    void		setSurface(const PolygonSurface*);
    const PolygonSurface* getSurface() const	     { return surface_; }

    bool		arePolygonsDisplayed() const { return displaypolygons_;}
    bool		isBodyDisplayed() const	     { return displaybody_; }

    bool		needsUpdate() const	     { return needsupdate_; }
    DAGTetrahedraTree*	getTetrahedraTree() const    { return tetrahedratree_; }
    TypeSet<int>	getExcludeTetragedras()	     { return notetrahedras_; }	
    bool		createsNormals() const       { return true; }
    
    bool		update(bool forceall,TaskRunner*);
    void		setRightHandedNormals(bool yn);

protected:

    void		updateGeometries();
    void		updatePolygonDisplay();
    bool		updateBodyDisplay(const TypeSet<Coord3>& allpoints);
    void		removeAll();
    void		addToGeometries(IndexedGeometry*);
    void		removeFromGeometries(const IndexedGeometry*);

    bool		displaypolygons_;
    bool		displaybody_;
    bool		needsupdate_;

    DAGTetrahedraTree*		tetrahedratree_; 
    TypeSet<int>		notetrahedras_;   
    const PolygonSurface*	surface_;
    IndexedGeometry*		bodytriangle_;
    IndexedGeometry*		polygondisplay_;
};

};

#endif
