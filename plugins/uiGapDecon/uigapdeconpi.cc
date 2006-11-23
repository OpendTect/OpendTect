/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : H. Huck
 * DATE     : Aug 2006
-*/

static const char* rcsID = "$Id";

#include "uigapdeconattrib.h"
#include "plugins.h"

extern "C" int GetuiGapDeconPluginType()
{
    return PI_AUTO_INIT_LATE;
}


extern "C" PluginInfo* GetuiGapDeconPluginInfo()
{
    static PluginInfo retpi = {
	"Gap Decon",
	"dGB (Helene)",
	"=od",
	"User Interface for Gap Decon attribute plugin." };
    return &retpi;
}


extern "C" const char* InituiGapDeconPlugin( int, char** )
{
    uiGapDeconAttrib::initClass();
    return 0; // All OK - no error messages
}
