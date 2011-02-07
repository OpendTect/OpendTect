/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : June 2008
-*/

static const char* rcsID = "$Id: gpucalcpi.cc,v 1.4 2011-02-07 12:54:26 cvskris Exp $";

#include "gpucalc.h"

#include "plugins.h"
#include "errh.h"

extern "C" int GetGPUCalcPluginType()
{
    return PI_AUTO_INIT_EARLY;
}


extern "C" PluginInfo* GetGPUCalcPluginInfo()
{
    static PluginInfo retpii = {
	"Graphics card based calculations", "dGB (Kristofer Tingdahl)", "=dgb",
	"" };
    return &retpii;
}


extern "C" const char* InitGPUCalcPlugin( int, char** )
{
    const char* nm = GPU::manager().getDevice(0)->name();
    nm = GPU::manager().getDevice( 1 )->name();

    return 0;
}
