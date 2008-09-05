#ifndef explpolygonsurface_h
#define explpolygonsurface_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Yuancheng Liu
 Date:          July 2008
 RCS:           $Id: explpolygonsurface.h,v 1.1 2008-09-05 16:48:42 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "indexedshape.h"
#include "position.h"

namespace Geometry
{

class PolygonSurface;

/*!A triangulated representation of a polygonsurface */


class ExplPolygonSurface: public Geometry::IndexedShape, public CallBacker
{
public:
			ExplPolygonSurface(PolygonSurface*);
    			~ExplPolygonSurface();

    void		setSurface(PolygonSurface*);
    PolygonSurface*	getSurface()			{ return surface_; }
    const PolygonSurface* getSurface() const		{ return surface_; }

    void		display(bool polygons,bool body);
    bool		arePolygonsDisplayed() const { return displaypolygons_;}
    bool		isBodyDisplayed() const	     { return displaybody_; }

    bool		needsUpdate() const		{ return needsupdate_; }

    bool		update(bool forceall,TaskRunner*);
    void		setRightHandedNormals(bool yn);
    bool		createsNormals() const		{ return true; }

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
    
    PolygonSurface*	surface_;
    IndexedGeometry*	bodytriangle_;
    IndexedGeometry*	polygondisplay_;
};

};

#endif
