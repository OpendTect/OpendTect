
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : May 2007
-*/

static const char* rcsID = "$Id: tutpi.cc,v 1.2 2007-05-11 12:55:07 cvsbert Exp $";

#include "tutseistools.h"
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
    return 0;
}
