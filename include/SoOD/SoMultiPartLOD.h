#ifndef vissomultirespart_h
#define vissomultirespart_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoMultiPartLOD.h,v 1.3 2003-11-07 12:21:54 bert Exp $
________________________________________________________________________


-*/

#include <Inventor/nodes/SoGroup.h>
#include <Inventor/fields/SoMFVec3f.h>
#include <Inventor/fields/SoSFInt32.h>
#include <Inventor/fields/SoMFFloat.h>

/*! \brief 
is a SoLOD that is used for shapes that has more than one part. The 'normal'
SoLOD does only have one center and the distance to the camera is calculated
from there. On a multi-part horizon this in not good enogh, since we might view
a detail on one of the parts, while the distance to the center still is large.
This is solved my allowing multiple centers. Each part adds its own center
to the centers field.

At render the distance between all centers and the camera is calculated, and the
shortest distance determines which child should be rendered in the same manner
as the normal SoLOD.

The user can set the whichChild field to disable the 'automatic' resolution selection.

*/


class SoMultiPartLOD : public SoGroup
{
    typedef SoGroup	inherited;
    SO_NODE_HEADER(SoMultiPartLOD);
public:
			SoMultiPartLOD(void);

    SoMFVec3f		centers;
    SoMFFloat 		range;
    SoSFInt32		whichChild;

    virtual void	doAction(SoAction * action);
    virtual void	callback(SoCallbackAction * action);
    virtual void	GLRender(SoGLRenderAction * action);
    virtual void	GLRenderBelowPath(SoGLRenderAction * action);
    virtual void	GLRenderInPath(SoGLRenderAction * action);
    virtual void	GLRenderOffPath(SoGLRenderAction * action);
    virtual void	rayPick(SoRayPickAction * action);
    virtual void	getBoundingBox(SoGetBoundingBoxAction * action);
    virtual void	getPrimitiveCount(SoGetPrimitiveCountAction * action);

    static void		initClass(void);

protected:
				~SoMultiPartLOD();
    virtual int			whichToTraverse(SoAction *);

private:
    void			commonConstructor(void);
};


#endif
