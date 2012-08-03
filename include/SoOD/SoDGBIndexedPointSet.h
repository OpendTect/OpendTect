#ifndef SoDGBIndexedPointSet_h
#define SoDGBIndexedPointSet_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		March 2009
 RCS:		$Id: SoDGBIndexedPointSet.h,v 1.4 2012-08-03 13:00:40 cvskris Exp $
________________________________________________________________________


-*/

#include "soodmod.h"
#include "Inventor/nodes/SoIndexedShape.h"

#include "soodbasic.h"


/*!\brief
A shape class that is similar to PointSet, but takes indexes.

UPDATE: Coin has a SoIndexedPointSet class, which should be used when
it comes to the stable repository.
*/

mClass(SoOD) SoDGBIndexedPointSet : public SoIndexedShape
{
    SO_NODE_HEADER(SoDGBIndexedPointSet);
public:

    static void		initClass();
    			SoDGBIndexedPointSet();

protected:

    void		generatePrimitives(SoAction*);
    SbBool		generateDefaultNormals(SoState*,SoNormalCache*);
    				/*! Does not work properly (TODO), use 
				    own normals instead */
    virtual SbBool	generateDefaultNormals( SoState* st, SoNormalBundle* b )
			{ return SoVertexShape::generateDefaultNormals(st,b); }

    void		GLRender(SoGLRenderAction*);

private:

    enum		Binding { OVERALL = 0, PER_VERTEX, PER_VERTEX_INDEXED };

    Binding		findMaterialBinding(SoState*) const;
    Binding		findNormalBinding(SoState*) const;

};

#endif


