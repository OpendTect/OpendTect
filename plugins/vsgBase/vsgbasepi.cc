/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsgbasemod.h"

#include "odplugin.h"
#include "visvsgdataman.h"


mDefODPluginInfo(vsgBase)
{
    static PluginInfo retpi(
	"VSG Viewer (Base)",
	"The base classes of the Vulkan (VSG) 3D viewer." );
    return &retpi;
}


mDefODInitPlugin(vsgBase)
{
    ODVsg::initStdClasses();
    return nullptr;
}
