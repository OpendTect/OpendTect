#ifndef SoIndexedPointSet_h
#define SoIndexedPointSet_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		March 2009
 RCS:		$Id: SoIndexedPointSet.h,v 1.1 2009-03-04 13:15:46 cvskris Exp $
________________________________________________________________________


-*/

#include "Inventor/nodes/SoIndexedShape.h"

#include "soodbasic.h"


/*!\brief
A shape class that is similar to PointSet, but takes indexes.
*/

mClass SoIndexedPointSet : public SoIndexedShape
{
    SO_NODE_HEADER(SoIndexedPointSet);
public:

    static void			initClass();
    				SoIndexedPointSet();

protected:
    void			generatePrimitives(SoAction*);
    SbBool			generateDefaultNormals(SoState* state,
						       SoNormalCache* nc);
    				/*! Does not work properly (TODO), use 
				    own normals instead */

    void			GLRender(SoGLRenderAction*);

private:
    enum			Binding { OVERALL = 0,
					  PER_VERTEX,
					  PER_VERTEX_INDEXED };


    Binding			findMaterialBinding(SoState*) const;
    Binding			findNormalBinding(SoState*) const;
};

#endif

