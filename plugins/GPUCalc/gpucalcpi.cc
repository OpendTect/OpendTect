/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "gpucalc.h"
#include "odplugin.h"


mDefODPluginEarlyLoad(GPUCalc)
mDefODPluginInfo(GPUCalc)
{
    mDefineStaticLocalObject( PluginInfo, retpi, (
	"Graphics card based calculations (Base)",
	"OpendTect",
	"dGB Earth Sciences (Kristofer Tingdahl)",
	"=od",
	"Graphics card based calculations" ))
    return &retpi;
}


mDefODInitPlugin(GPUCalc)
{
    const char* nm = GPU::manager().getDevice(0)->name();
    nm = GPU::manager().getDevice( 1 )->name();

    return nullptr;
}
