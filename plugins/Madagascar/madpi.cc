/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert Bril
 * DATE     : July 2007
-*/

static const char* rcsID = "$Id: madpi.cc,v 1.2 2007-07-04 09:44:54 cvsbert Exp $";

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
    static BufferString prescanmsg = ODMad::PI().errMsg();
    return prescanmsg.isEmpty() ? 0 : prescanmsg.buf();
}
