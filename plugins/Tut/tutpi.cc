
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : May 2007
-*/

static const char* rcsID = "$Id: tutpi.cc,v 1.3 2007-06-01 06:29:09 cvsraman Exp $";

#include "tutseistools.h"
#include "tutorialattrib.h"
#include "plugins.h"

extern "C" int GetTutPluginType()
{
    return PI_AUTO_INIT_EARLY;
}


extern "C" PluginInfo* GetTutPluginInfo()
{
    static PluginInfo retpi = {
	"Tutorial plugin development (Non-UI)",
	"dGB (Raman/Bert)",
	"3.0",
    	"Shows some simple plugin basics." };
    return &retpi;
}


extern "C" const char* InitTutPlugin( int, char** )
{
    Attrib::Tutorial::initClass();

    return 0;
}
