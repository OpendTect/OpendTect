/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : June 2008
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
