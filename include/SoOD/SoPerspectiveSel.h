#ifndef SoPerspectiveSel_h
#define SoPerspectiveSel_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________

-*/

#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/fields/SoMFVec3f.h>
#include <Inventor/fields/SoSFVec3f.h>

#include "soodbasic.h"

mClass SoPerspectiveSel : public SoGroup
{
    typedef SoGroup inherited;

    SO_NODE_HEADER(SoPerspectiveSel);

public:
			SoPerspectiveSel(void);
			SoPerspectiveSel(int numchildren);

    static void initClass(void);

    SoMFVec3f		perspectives;
    SoSFVec3f		center;

    virtual void	doAction(SoAction * action);
    virtual void	callback(SoCallbackAction * action);
    virtual void	GLRender(SoGLRenderAction * action);
    virtual void	GLRenderBelowPath(SoGLRenderAction * action);
    virtual void	GLRenderInPath(SoGLRenderAction * action);
    virtual void	GLRenderOffPath(SoGLRenderAction * action);
    virtual void	rayPick(SoRayPickAction * action);
    virtual void	getBoundingBox(SoGetBoundingBoxAction * action);
    virtual void	getPrimitiveCount(SoGetPrimitiveCountAction * action);

protected:
    virtual		~SoPerspectiveSel();

    virtual int		whichToTraverse(SoAction *);

private:
    void		commonConstructor(void);
};

#endif
