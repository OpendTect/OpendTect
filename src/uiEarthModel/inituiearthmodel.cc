/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "moddepmgr.h"
#include "uibodyposprovgroup.h"
#include "uihorinterpol.h"
#include "uisurfaceposprov.h"

mDefModInitFn(uiEarthModel)
{
    mIfNotFirstTime( return );
    
    uiInvDistHor3DInterpol::initClass();
    uiTriangulationHor3DInterpol::initClass();
    uiExtensionHor3DInterpol::initClass();
    uiContinuousCurvatureHor3DInterpol::initClass();

    uiBodyPosProvGroup::initClass();
    uiSurfacePosProvGroup::initClass();
}
