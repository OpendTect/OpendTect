#ifndef SoMarkerScale_h
#define SoMarkerScale_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoMarkerScale.h,v 1.2 2003-11-07 12:21:54 bert Exp $
________________________________________________________________________


-*/

#include <Inventor/nodes/SoNode.h>
#include <Inventor/fields/SoSFVec3f.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/nodes/SoSubNode.h>


/*!\brief
The SoMarkerScale class is used for scaling and translating a marker based on
projected size.

The marker shape is stored in the "shape" part. Any kind of node
can be used, even group nodes with several shapes, but the marker
shape should be approximately of unit size, and with a center 
position in (0, 0, 0).
	      
*/

class SoMarkerScale : public SoNode
{
    SO_NODE_HEADER(SoMarkerScale);
public:
			SoMarkerScale(void);
    static void		initClass(void);

    SoSFVec3f		translation;
    			/*!< Sets the position of the shape */
    SoSFVec3f		scaleFactor;
    			/*!< Makes it possible to set the proportions of the
			     shape */
    SoSFFloat		screenSize;
			/*!< The desired width on screen. Default is 5 pixels */

protected:

    virtual void	callback(SoCallbackAction* action);
    virtual void	GLRender(SoGLRenderAction* action);
    virtual void	getBoundingBox(SoGetBoundingBoxAction* action);
    virtual void	pick(SoPickAction* );
    virtual void	getPrimitiveCount(SoGetPrimitiveCountAction * );
    virtual void	doAction(SoAction*);
    virtual		~SoMarkerScale();
};

#endif
