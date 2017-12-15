#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Apr 2011
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

/* The following function MIGHT be defined: */
#define mDefODPluginSurvRelToolsLoadFn(pinm) \
    mExternC(pinm) void Load##pinm##PluginSurvRelTools(); \
    mExternC(pinm) void Load##pinm##PluginSurvRelTools()

/* If you have a SIP or uiSurveyManager::Util, this calls the load function: */
#define mCallODPluginSurvRelToolsLoadFn(pinm) Load##pinm##PluginSurvRelTools()


/* Define ONLY if your plugin needs early loading (i.e. before any UI)
   This is common for 'Batch'-type plugins.
 */
#define mDefODPluginEarlyLoad(pinm) \
    mExternC(pinm) int Get##pinm##PluginType(); \
    mExternC(pinm) int Get##pinm##PluginType() { return PI_AUTO_INIT_EARLY; }


#define mODPluginCreator		"opendtect.org"
#define mODPluginVersion		"=od"
#define mODPluginSeeMainModDesc		"See main module's description"

#define mODPluginODPackage		"OpendTect"
#define mODPluginExtraAttribsPackage	"Additional Attributes"
#define mODPluginGamesPackage		"Games"
#define mODPluginTutorialsPackage	"Tutorials"
