#ifndef odplugin_h
#define odplugin_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Apr 2011
 RCS:		$Id: odplugin.h,v 1.2 2011/04/21 09:44:17 cvsbert Exp $
________________________________________________________________________

-*/

extern "C" {
#include "pluginbase.h"
}
#include "gendefs.h"


/* The following function MUST be defined: */
#define mDefODInitPlugin(pinm) \
    mExternC const char* Init##pinm##Plugin(int,char**); \
    mExternC const char* Init##pinm##Plugin( int argc, char** argv )

/* The following function SHOULD be defined: */
#define mDefODPluginInfo(pinm) \
    mExternC PluginInfo* Get##pinm##PluginInfo(); \
    mExternC PluginInfo* Get##pinm##PluginInfo()

/* Define ONLY if your plugin needs early loading (i.e. before any UI)
   This is common for 'Batch'-type plugins.
 */
#define mDefODPluginEarlyLoad(pinm) \
    mExternC int Get##pinm##PluginType(); \
    mExternC int Get##pinm##PluginType() { return PI_AUTO_INIT_EARLY; }


#endif
