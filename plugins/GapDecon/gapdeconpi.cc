/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : H. Huck
 * DATE     : Aug 2006
-*/

static const char* rcsID = "$Id";

#include "gapdeconattrib.h"
#include "plugins.h"

extern "C" int GetGapDeconPluginType()
{
    return PI_AUTO_INIT_EARLY;
}


extern "C" PluginInfo* GetGapDeconPluginInfo()
{
    static PluginInfo retpii = {
	"Gap Decon Base",
	"dGB - Helene",
	"=od",
	"Gap Decon (Prediction Error Filter) attribute plugin.\n\n" };
    return &retpii;
}


extern "C" const char* InitGapDeconPlugin( int, char** )
{
    Attrib::GapDecon::initClass();

    return 0; // All OK - no error messages
}
