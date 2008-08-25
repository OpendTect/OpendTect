/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Raman Singh
 * DATE     : July 2008
-*/

static const char* rcsID = "$Id: gmtpi.cc,v 1.3 2008-08-25 09:59:52 cvsraman Exp $";

#include "initgmt.h"
#include "plugins.h"

extern "C" int GetGMTPluginType()
{
    return PI_AUTO_INIT_EARLY;
}


extern "C" PluginInfo* GetGMTPluginInfo()
{
    static PluginInfo retpi = {
	"GMT Base",
	"dGB (Raman)",
	"3.2",
    	"Plots Surface data using GMT mapping tool" };
    return &retpi;
}


extern "C" const char* InitGMTPlugin( int, char** )
{
    GMT::initStdClasses();

    return 0;
}
