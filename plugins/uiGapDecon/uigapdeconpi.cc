/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : H. Huck
 * DATE     : Aug 2006
-*/

static const char* rcsID = "$Id";

#include "uigapdeconattrib.h"
#include "uiattrfact.h"

#include "plugins.h"

extern "C" int GetuiGapDeconPluginType()
{
    return PI_AUTO_INIT_LATE;
}


extern "C" PluginInfo* GetuiGapDeconPluginInfo()
{
    static PluginInfo retpi = {
	"GapDecon User Interface",
	"dGB",
	"=od",
	"User interface for Gap Decon plugin.\n\n"
        "Usage of Gap Decon may be subject to patent laws!" };
    return &retpi;
}


mDeclAttrDescEd(GapDecon)

extern "C" const char* InituiGapDeconPlugin( int, char** )
{
    uiAttribFactory::add( "GapDecon", "GapDecon",
	    		  new uiGapDeconAttrDescEdCreater);

    return 0; // All OK - no error messages
}
