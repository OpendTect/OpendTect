#ifndef odplugin_h
#define odplugin_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Apr 2011
 RCS:		$Id: odplugin.h,v 1.3 2012-08-03 13:00:13 cvskris Exp $
________________________________________________________________________

-*/

extern "C" {
#include "basicmod.h"
#include "pluginbase.h"
}
#include "gendefs.h"


/* The following function MUST be defined: */
#define mDefODInitPlugin(pinm) \
    mExternC(Basic) const char* Init##pinm##Plugin(int,char**); \
    mExternC(Basic) const char* Init##pinm##Plugin( int argc, char** argv )

/* The following function SHOULD be defined: */
#define mDefODPluginInfo(pinm) \
    mExternC(Basic) PluginInfo* Get##pinm##PluginInfo(); \
    mExternC(Basic) PluginInfo* Get##pinm##PluginInfo()

/* Define ONLY if your plugin needs early loading (i.e. before any UI)
   This is common for 'Batch'-type plugins.
 */
#define mDefODPluginEarlyLoad(pinm) \
    mExternC(Basic) int Get##pinm##PluginType(); \
    mExternC(Basic) int Get##pinm##PluginType() { return PI_AUTO_INIT_EARLY; }


#endif

