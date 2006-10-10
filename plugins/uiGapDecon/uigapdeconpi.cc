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
	"GapDecon User Interface",
	"dGB (Helene)",
	"=od",
	"User interface for Gap Decon plugin." };
    return &retpi;
}


extern "C" const char* InituiGapDeconPlugin( int, char** )
{
    uiGapDeconAttrib::initClass();
    return 0; // All OK - no error messages
}
