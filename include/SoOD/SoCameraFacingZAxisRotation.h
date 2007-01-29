#ifndef SoCameraFacingZAxisRotation_h
#define SoCameraFacingZAxisRotation_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoCameraFacingZAxisRotation.h,v 1.1 2007-01-29 13:42:57 cvskris Exp $
________________________________________________________________________

-*/

#include <Inventor/nodes/SoTransformation.h>
#include <Inventor/SbLinear.h>

#include <Inventor/fields/SoSFBool.h>
#include <Inventor/fields/SoSFFloat.h>


/*!\brief

*/

class SoCameraFacingZAxisRotation : public SoTransformation
{
    typedef SoTransformation inherited;
    SO_NODE_HEADER(SoCameraFacingZAxisRotation);

public:
    		SoCameraFacingZAxisRotation();
    static void	initClass();
    void	doAction(SoAction * action);
    void	GLRender(SoGLRenderAction * action);
    void	callback(SoCallbackAction * action);
    void	getBoundingBox(SoGetBoundingBoxAction * action);
    void	getMatrix(SoGetMatrixAction * action);
    void	pick(SoPickAction * action);
    void	getPrimitiveCount(SoGetPrimitiveCountAction * action);


    SoSFBool	lock;
protected:
    		~SoCameraFacingZAxisRotation();

    SbRotation	currot_;
};

#endif

