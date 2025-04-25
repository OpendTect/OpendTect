#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "geometrymod.h"
#include "indexedshape.h"
#include "coord.h"

class DAGTetrahedraTree;

namespace Geometry
{

class PolygonSurface;

/*!A triangulated representation of a polygonsurface */


mExpClass(Geometry) ExplPolygonSurface : public Geometry::IndexedShape
				       , public CallBacker
{
public:
			ExplPolygonSurface(const PolygonSurface*,
					   float zscale=1);
			~ExplPolygonSurface();

    void		display(bool polygons,bool body);
    void		setPolygonSurface(const PolygonSurface*);
    const PolygonSurface* getPolygonSurface() const	{ return surface_; }

    void		setZScale(float);

    bool		arePolygonsDisplayed() const { return displaypolygons_;}
    bool		isBodyDisplayed() const      { return displaybody_; }

    bool		needsUpdate() const override { return needsupdate_; }
    DAGTetrahedraTree*	getTetrahedraTree() const    { return tetrahedratree_; }

    TypeSet<Coord3>	getSurfaceSamples() const    { return samples_; }
    TypeSet<int>	getSampleIndices() const     { return sampleindices_; }
			/*<The indices are corresponding to the samples_. */
    
    bool		prepareBodyDAGTree();
			/*<Create body tetrahedras based on surface polygons.*/
    char		positionToBody(const Coord3 point);
			/*<Check point is inside, on, or outside a triangulated
			   body surface, ret 1, 0, -1 respectively. */

    bool		createsNormals() const override       { return true; }
    
    bool		update(bool forceall,TaskRunner*) override;

protected:

    void		updateGeometries();
    bool		updateBodyDisplay();
    void		removeAll(bool) override;
    void		addToGeometries(IndexedGeometry*);
    void		removeFromGeometries(const IndexedGeometry*);
    void		addToTrianglePrimitiveSet(Geometry::PrimitiveSet*,
						  int,int,int);
    void		calcNormals(int nrtriangles,int idx1,int idx2,int idx3);

    int			getPolygonIdx(int);

    bool		displaypolygons_;
    bool		displaybody_;
    bool		needsupdate_;
    Coord3		scalefacs_;

    DAGTetrahedraTree*		tetrahedratree_; 
    TypeSet<Coord3>		samples_;
    TypeSet<int>		sampleindices_;
    TypeSet<int>		plgsamplesidxs_;
    const PolygonSurface*	surface_;
    IndexedGeometry*		bodytriangle_;
    IndexedGeometry*		polygondisplay_;
};

} // namespace Geometry
