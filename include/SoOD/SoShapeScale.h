#ifndef SoShapeScale_h
#define SoShapeScale_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include <Inventor/nodes/SoNode.h>
#include <Inventor/fields/SoSFVec3f.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFBool.h>
#include <Inventor/nodes/SoSubNode.h>

#include "soodbasic.h"


/*!\brief
The SoShapeScale class is used for scaling and rotating and translating a shape
based on projected size.

The marker shape should be behind this node in the scene, and
should be approximately of unit size, and with a center 
position in (0, 0, 0).
	      
*/

mClass SoShapeScale : public SoNode
{
    SO_NODE_HEADER(SoShapeScale);
public:
			SoShapeScale(void);
    static void		initClass(void);

    SoSFBool		restoreProportions;
    			/*!< If true, the scale will be reset to (1,1,1),
			     wich will make the shape's proportions
			     equal in all dimensions */

    SoSFBool		dorotate;

    SoSFFloat		screenSize;
			/*!< The desired width on screen. Default is 5 pixels.
			     If screenSize is equal to zero, the equal-
			     screen-size-scaling is turned off.
			*/
    SoSFFloat		minScale;
    SoSFFloat		maxScale;

protected:

    virtual void	callback(SoCallbackAction* action);
    virtual void	GLRender(SoGLRenderAction* action);
    virtual void	getBoundingBox(SoGetBoundingBoxAction* action);
    virtual void	pick(SoPickAction* );
    virtual void	rayPick(SoRayPickAction* );
    virtual void	getPrimitiveCount(SoGetPrimitiveCountAction * );
    virtual void	doAction(SoAction*);
    virtual		~SoShapeScale();

    SbVec3f		scaleby;
    char		changescale;
    			//-1 = don't know, 0 = no, 1 = yes		
};

#endif
