#ifndef SoForegroundTranslation_h
#define SoForegroundTranslation_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoForegroundTranslation.h,v 1.9 2009/07/22 16:01:19 cvsbert Exp $
________________________________________________________________________


-*/

#include "Inventor/nodes/SoNode.h"
#include "Inventor/nodes/SoSubNode.h"
#include "Inventor/fields/SoSFFloat.h"

#include "soodbasic.h"

/*!\brief
A class that moves the objects further down the traversal towards the camera,
so they can be clearly viewed. An example is wireframes that should be visible
in front of the camera, and not mixed with the triangles of the surface that
it represents.

*/

mClass SoForegroundTranslation : public SoNode
{
    typedef SoNode		inherited;
    SO_NODE_HEADER(SoForegroundTranslation);
public:
    				SoForegroundTranslation();
    static void			initClass();

    SoSFFloat			lift;
    				//!<Distance that the object should be moved
    				

protected:
    void	GLRender(SoGLRenderAction*);
    void	getBoundingBox(SoGetBoundingBoxAction*);
    void	callback(SoCallbackAction*);
    void	getMatrix(SoGetMatrixAction*);
    void	pick(SoPickAction*);
    void	getPrimitiveCount(SoGetPrimitiveCountAction*);

    void	doAction(SoAction*);
};

#endif

