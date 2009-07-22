/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : Jul 2007
-*/

static const char* rcsID = "$Id";

#include "uieventfreqattrib.h"
#include "plugins.h"

extern "C" int GetuiEventFreqPluginType()
{
    return PI_AUTO_INIT_LATE;
}


extern "C" PluginInfo* GetuiEventFreqPluginInfo()
{
    static PluginInfo retpi = {
	"Event Frequency",
	"dGB",
	"=od",
	"User interface for Event Frequency attribute." };
    return &retpi;
}


extern "C" const char* InituiEventFreqPlugin( int, char** )
{
    uiEventFreqAttrib::initClass();
    return 0;
}
