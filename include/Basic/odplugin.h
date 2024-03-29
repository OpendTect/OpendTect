#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"

extern "C" {
#include "pluginbase.h"
}
#include "gendefs.h"


/* The following function MUST be defined: */
#define mDefODInitPlugin(pinm) \
    mExternC(pinm) const char* Init##pinm##Plugin(int,char**); \
    mExternC(pinm) const char* Init##pinm##Plugin( int argc, char** argv )

/* The following function SHOULD be defined: */
#define mDefODPluginInfo(pinm) \
    mExternC(pinm) PluginInfo* Get##pinm##PluginInfo(); \
    mExternC(pinm) PluginInfo* Get##pinm##PluginInfo()

/* Define ONLY if your plugin needs early loading (i.e. before any UI)
   This is common for 'Batch'-type plugins.
 */
#define mDefODPluginEarlyLoad(pinm) \
    mExternC(pinm) int Get##pinm##PluginType(); \
    mExternC(pinm) int Get##pinm##PluginType() { return PI_AUTO_INIT_EARLY; }
