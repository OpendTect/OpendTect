#ifndef SoIndexedTriangleFanSet_h
#define SoIndexedTriangleFanSet_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoIndexedTriangleFanSet.h,v 1.4 2009-01-08 09:48:12 cvsnanne Exp $
________________________________________________________________________


-*/

#include "Inventor/nodes/SoIndexedShape.h"


/*!\brief
A shape class that is similar to IndexedTriangleStripSet. The organized like
a fan, where the first index is in the center of the fan, and the following
indexes was the surrounding coordinates.

1   2   3   4   5

       6

I.e. a coordIndex sequence of : 6, 1, 2, 3, 4, 5, -1 creates the following
triangles: 6-1-2, 6-2-3, 6-3-4, 6-4-5.
This saves quite much mem compared with stripsets.

*/

class COIN_DLL_API SoIndexedTriangleFanSet : public SoIndexedShape
{
    SO_NODE_HEADER(SoIndexedTriangleFanSet);
public:

    static void			initClass();
    				SoIndexedTriangleFanSet();

protected:
    void			generatePrimitives(SoAction*);
    SbBool			generateDefaultNormals(SoState* state,
						       SoNormalCache* nc);
    				/*! Does not work properly (TODO), use 
				    own normals instead */

    void			GLRender(SoGLRenderAction*);

private:
    enum			Binding { OVERALL = 0,
					  PER_FAN,
					  PER_FAN_INDEXED,
					  PER_TRIANGLE,
					  PER_TRIANGLE_INDEXED,
					  PER_VERTEX,
					  PER_VERTEX_INDEXED };


    Binding			findMaterialBinding(SoState*) const;
    Binding			findNormalBinding(SoState*) const;
};

#endif

