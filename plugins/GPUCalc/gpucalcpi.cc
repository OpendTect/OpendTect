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
    static PluginInfo retpii = {
	"Graphics card based calculations", "dGB (Kristofer Tingdahl)", "=dgb",
	"" };
    return &retpii;
}


mDefODInitPlugin(GPUCalc)
{
    const char* nm = GPU::manager().getDevice(0)->name();
    nm = GPU::manager().getDevice( 1 )->name();

    return nullptr;
}

