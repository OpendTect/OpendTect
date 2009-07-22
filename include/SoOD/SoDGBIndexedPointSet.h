#ifndef SoDGBIndexedPointSet_h
#define SoDGBIndexedPointSet_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		March 2009
 RCS:		$Id: SoDGBIndexedPointSet.h,v 1.2 2009-07-22 16:01:19 cvsbert Exp $
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

