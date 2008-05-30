#ifndef visgeomindexedshape_h
#define visgeomindexedshape_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		September 2007
 RCS:		$Id: visgeomindexedshape.h,v 1.5 2008-05-30 04:31:40 cvskris Exp $
________________________________________________________________________

-*/

#include "visobject.h"

namespace Geometry { class IndexedShape; class IndexedGeometry; }

class SoIndexedShape;
class TaskRunner;

namespace visBase
{

class Coordinates;
class Normals;

/*!Visualisation for Geometry::IndexedShape. */

class GeomIndexedShape : public VisualObjectImpl
{
public:
    static GeomIndexedShape*	create()
				mCreateDataObj(GeomIndexedShape);

    void			setDisplayTransformation(mVisTrans*);
    mVisTrans*			getDisplayTransformation();

    void			setSurface(Geometry::IndexedShape*,
	    				   TaskRunner* = 0);
    				//!<Does not become mine, should remain
				//!<in memory
    void			setRightHandSystem(bool);

    void			touch(bool forall,TaskRunner* =0);

    void			set3DLineRadius(float radius,
	    					bool constantonscreen=true,
						float maxworldsize=-1);
    				/*!<If radius is less than 0, a normal
				    line will be drawn. */

protected:
						~GeomIndexedShape();

    Coordinates*				coords_;
    Normals*					normals_;

    ObjectSet<SoIndexedShape>			strips_;
    ObjectSet<const Geometry::IndexedGeometry>	stripgeoms_;

    float					lineradius_;
    bool					lineconstantonscreen_;
    float					linemaxsize_;
    ObjectSet<SoIndexedShape>			lines_;
    ObjectSet<const Geometry::IndexedGeometry>	linegeoms_;

    ObjectSet<SoIndexedShape>			fans_;
    ObjectSet<const Geometry::IndexedGeometry>	fangeoms_;

    Geometry::IndexedShape*			shape_;
};

};
	
#endif
