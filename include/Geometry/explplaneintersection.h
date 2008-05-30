#ifndef explplaneintersection_h
#define explplaneintersection_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          May 2008
 RCS:           $Id: explplaneintersection.h,v 1.1 2008-05-30 03:49:10 cvskris Exp $
________________________________________________________________________

-*/

#include "indexedshape.h"
#include "position.h"

class RCol;
class Plane3;

namespace Geometry
{

/*! Class that represents the line in the intersection between an
    IndexedShape and one ore many planes. */


class ExplPlaneIntersection: public Geometry::IndexedShape,
			     public CallBacker
{
public:
				ExplPlaneIntersection();
				~ExplPlaneIntersection();

    int				nrPlanes() const;
    int				planeID(int idx) const;
    const Coord3&		planeNormal(int id) const;
    const TypeSet<Coord3>&	planePolygon(int id) const;

    int				addPlane(const Coord3& normal,
	    				 const TypeSet<Coord3>&);
    bool			setPlane(int id, const Coord3&,
	    				 const TypeSet<Coord3>&);
    void			removePlane(int id);

    void			setShape(const IndexedShape&);
    				/*!<Is assumed to remain in memory. */
    const IndexedShape*		getShape() const;
    				

    bool			needsUpdate() const;
    void			removeAll();

    void			setZScale(float nz)	{ zscale_ = nz; }
    float			getZScale() const	{ return zscale_; }

protected:
    bool			update(bool forceall,TaskRunner*);
    
    const IndexedShape*				shape_;
    int						shapeversion_;
    bool					needsupdate_;

    IndexedGeometry*				intersection_;

    TypeSet<int>				planeids_;
    ObjectSet<TypeSet<Coord3> >			planepts_;
    TypeSet<Coord3>				planenormals_;

    float					zscale_;
};

};

#endif
