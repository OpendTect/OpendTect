#ifndef SoCameraFacingZAxisRotation_h
#define SoCameraFacingZAxisRotation_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________

-*/

#include "soodmod.h"
#include <Inventor/nodes/SoTransformation.h>
#include <Inventor/SbLinear.h>

#include <Inventor/fields/SoSFBool.h>
#include <Inventor/fields/SoSFFloat.h>

#include "soodbasic.h"

/*!\brief

*/

mSoODClass SoCameraFacingZAxisRotation : public SoTransformation
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


