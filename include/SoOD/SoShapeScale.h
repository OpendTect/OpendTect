#ifndef SoShapeScale_h
#define SoShapeScale_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoShapeScale.h,v 1.6 2004-05-11 12:20:13 kristofer Exp $
________________________________________________________________________


-*/

#include <Inventor/nodes/SoNode.h>
#include <Inventor/fields/SoSFVec3f.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFBool.h>
#include <Inventor/nodes/SoSubNode.h>


/*!\brief
The SoShapeScale class is used for scaling and translating a marker based on
projected size.

The marker shape is stored in the "shape" part. Any kind of node
can be used, even group nodes with several shapes, but the marker
shape should be approximately of unit size, and with a center 
position in (0, 0, 0).
	      
*/

class SoShapeScale : public SoNode
{
    SO_NODE_HEADER(SoShapeScale);
public:
			SoShapeScale(void);
    static void		initClass(void);

    SoSFBool		doscale;
    SoSFBool		dorotate;
    SoSFFloat		screenSize;
			/*!< The desired width on screen. Default is 5 pixels */

protected:

    virtual void	callback(SoCallbackAction* action);
    virtual void	GLRender(SoGLRenderAction* action);
    virtual void	getBoundingBox(SoGetBoundingBoxAction* action);
    virtual void	pick(SoPickAction* );
    virtual void	getPrimitiveCount(SoGetPrimitiveCountAction * );
    virtual void	doAction(SoAction*);
    virtual		~SoShapeScale();
};

#endif
