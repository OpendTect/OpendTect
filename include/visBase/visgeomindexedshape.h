#ifndef visgeomindexedshape_h
#define visgeomindexedshape_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		September 2007
 RCS:		$Id: visgeomindexedshape.h,v 1.1 2007-09-14 13:11:30 cvskris Exp $
________________________________________________________________________


-*/

#include "visobject.h"

namespace Geometry { class IndexedShape; class IndexedGeometry; }

class SoIndexedShape;

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

    void			setSurface(Geometry::IndexedShape*);
    				//!<Does not become mine, should remain
				//!<in memory
    void			setRightHandSystem(bool);

    void			touch();

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
