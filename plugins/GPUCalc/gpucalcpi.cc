/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : June 2008
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "gpucalc.h"
#include "odplugin.h"


mDefODPluginEarlyLoad(GPUCalc)
mDefODPluginInfo(GPUCalc)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"Graphics card based calculations", mODPluginODPackage,
	mODPluginCreator, mODPluginVersion, mODPluginSeeMainModDesc ) );
    return &retpii;
}


mDefODInitPlugin(GPUCalc)
{
    const char* nm = GPU::manager().getDevice(0)->name();
    nm = GPU::manager().getDevice( 1 )->name();

    return 0;
}

