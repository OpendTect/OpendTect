#ifndef SoManLevelOfDetail_h
#define SoManLevelOfDetail_h
/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoManLevelOfDetail.h,v 1.6 2009-01-08 09:48:12 cvsnanne Exp $
________________________________________________________________________

-*/

#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/fields/SoMFFloat.h>
#include <Inventor/fields/SoSFInt32.h>

#define SO_MANLEVELOFDETAIL_AUTO (-1)

class SoBoundingBoxCache;

/*!\ brief
is a SoLevelOfDetail where the child can be selected manually by setting
whichChild. If automatic child selcection should be enabled, whichChild is set
to SO_MANLEVELOFDETAIL_AUTO

*/


class COIN_DLL_API SoManLevelOfDetail : public SoGroup {
    typedef SoGroup inherited;

    SO_NODE_HEADER(SoManLevelOfDetail);

public:
    static void		initClass(void);

			SoManLevelOfDetail(void);
			SoManLevelOfDetail(int numchildren);

    SoMFFloat		screenArea;
    SoSFInt32		whichChild;

    virtual void	doAction(SoAction* action);
    virtual void	callback(SoCallbackAction* action);
    virtual void	GLRender(SoGLRenderAction* action);
    virtual void	rayPick(SoRayPickAction* action);
    virtual void	pick(SoPickAction* action);
    virtual void 	getBoundingBox(SoGetBoundingBoxAction * action);

protected:
    virtual		~SoManLevelOfDetail();

private:
    void		commonConstructor(void);
    SoBoundingBoxCache*	bboxcache;
};

#endif
