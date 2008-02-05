#ifndef visgeomindexedshape_h
#define visgeomindexedshape_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		September 2007
 RCS:		$Id: visgeomindexedshape.h,v 1.4 2008-02-05 21:55:46 cvskris Exp $
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

protected:
						~GeomIndexedShape();

    Coordinates*				coords_;
    Normals*					normals_;

    ObjectSet<SoIndexedShape>			strips_;
    ObjectSet<const Geometry::IndexedGeometry>	stripgeoms_;

    ObjectSet<SoIndexedShape>			lines_;
    ObjectSet<const Geometry::IndexedGeometry>	linegeoms_;

    ObjectSet<SoIndexedShape>			fans_;
    ObjectSet<const Geometry::IndexedGeometry>	fangeoms_;

    Geometry::IndexedShape*			shape_;
};

};
	
#endif
