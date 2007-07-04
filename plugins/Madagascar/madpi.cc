/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert Bril
 * DATE     : July 2007
-*/

static const char* rcsID = "$Id: madpi.cc,v 1.1 2007-07-04 08:01:47 cvsbert Exp $";

#include "maddefs.h"
#include "plugins.h"

extern "C" int GetMadagascarPluginType()
{
    return PI_AUTO_INIT_EARLY;
}


extern "C" PluginInfo* GetMadagascarPluginInfo()
{
    static PluginInfo retpii = {
	"Madagascar base",
	"dGB - Bert Bril",
	"=od",
	"The Madagascar batch-level tools." };
    return &retpii;
}


extern "C" const char* InitMadagascarPlugin( int, char** )
{

    return 0; // All OK - no error messages
}
