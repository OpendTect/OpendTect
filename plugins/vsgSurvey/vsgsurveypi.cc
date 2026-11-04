/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsgsurveymod.h"

#include "odplugin.h"
#include "visvsgsurvscene.h"


mDefODPluginInfo(vsgSurvey)
{
    static PluginInfo retpi(
	"VSG Viewer (Survey)",
	"The survey scene of the Vulkan (VSG) 3D viewer." );
    return &retpi;
}


mDefODInitPlugin(vsgSurvey)
{
    ODVsg::initSurveyClasses();
    return nullptr;
}
