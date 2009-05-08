#ifndef SoDGBIndexedPointSet_h
#define SoDGBIndexedPointSet_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		March 2009
 RCS:		$Id: SoDGBIndexedPointSet.h,v 1.1 2009-05-08 21:45:09 cvskris Exp $
________________________________________________________________________


-*/

#include "Inventor/nodes/SoIndexedShape.h"

#include "soodbasic.h"


/*!\brief
A shape class that is similar to PointSet, but takes indexes.

UPDATE: Coin has a SoIndexedPointSet class, which should be used when
it comes to the stable repository.
*/

mClass SoDGBIndexedPointSet : public SoIndexedShape
{
    SO_NODE_HEADER(SoDGBIndexedPointSet);
public:

    static void			initClass();
    				SoDGBIndexedPointSet();

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

