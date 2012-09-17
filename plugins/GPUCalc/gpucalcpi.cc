/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : June 2008
-*/

static const char* rcsID = "$Id: gpucalcpi.cc,v 1.5 2011/04/21 13:09:13 cvsbert Exp $";

#include "gpucalc.h"

#include "odplugin.h"
#include "errh.h"

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

    return 0;
}
