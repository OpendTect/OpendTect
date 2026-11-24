/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsgsurveymod.h"

#include "moddepmgr.h"
#include "visvsgsurvscene.h"


mDefModInitFn(vsgSurvey)
{
    mIfNotFirstTime( return );
    ODVsg::initSurveyClasses();
}
