#ifndef SoIndexedTriangleFanSet_h
#define SoIndexedTriangleFanSet_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "Inventor/nodes/SoIndexedShape.h"

#include "soodbasic.h"


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

mClass SoIndexedTriangleFanSet : public SoIndexedShape
{
    SO_NODE_HEADER(SoIndexedTriangleFanSet);
public:

    static void		initClass();
    			SoIndexedTriangleFanSet();

protected:
    void		generatePrimitives(SoAction*);
    SbBool		generateDefaultNormals(SoState*,SoNormalCache*);
    				/*! Does not work properly (TODO), use 
				    own normals instead */
    virtual SbBool	generateDefaultNormals( SoState* st, SoNormalBundle* b )
			{ return SoVertexShape::generateDefaultNormals(st,b); }

    void		GLRender(SoGLRenderAction*);

private:
    enum		Binding { OVERALL = 0,
				  PER_FAN, PER_FAN_INDEXED,
				  PER_TRIANGLE, PER_TRIANGLE_INDEXED,
				  PER_VERTEX, PER_VERTEX_INDEXED };

    Binding		findMaterialBinding(SoState*) const;
    Binding		findNormalBinding(SoState*) const;

};

#endif

