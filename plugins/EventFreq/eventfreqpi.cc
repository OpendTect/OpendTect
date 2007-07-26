/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert Bril
 * DATE     : Aug 2006
-*/

static const char* rcsID = "$Id: eventfreqpi.cc,v 1.1 2007-07-26 16:35:22 cvsbert Exp $";

#include "eventfreqattrib.h"
#include "plugins.h"

extern "C" int GetEventFreqPluginType()
{
    return PI_AUTO_INIT_EARLY;
}


extern "C" PluginInfo* GetEventFreqPluginInfo()
{
    static PluginInfo retpii = {
	"Event frequency Attribute (Base)",
	"dGB - Bert Bril",
	"=od",
	"Defining the 'EventFreq' attribute." };
    return &retpii;
}


extern "C" const char* InitEventFreqPlugin( int, char** )
{
    Attrib::EventFreq::initClass();
    return 0; // All OK - no error messages
}
