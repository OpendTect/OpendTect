/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "moddepmgr.h"
#include "uivelocityfunctionvolume.h"
#include "uivelocityfunctionstored.h"
#include "uilinearveltrans.h"


mDefModInitFn(uiVelocity)
{
    mIfNotFirstTime( return );

    Vel::uiVolumeFunction::initClass();
    Vel::uiStoredFunction::initClass();

    Vel::uiLinearVelTransform::initClass();
}
