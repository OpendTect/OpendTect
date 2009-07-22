/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : July 2007
-*/

static const char* rcsID = "$Id: madpi.cc,v 1.4 2009-07-22 16:01:27 cvsbert Exp $";

#include "maddefs.h"
#include "plugins.h"

mExternC int GetMadagascarPluginType()
{
    return PI_AUTO_INIT_EARLY;
}


mExternC PluginInfo* GetMadagascarPluginInfo()
{
    static PluginInfo retpii = {
	"Madagascar base",
	"dGB - Bert Bril",
	"=od",
	"The Madagascar batch-level tools." };
    return &retpii;
}


mExternC const char* InitMadagascarPlugin( int, char** )
{
    static BufferString prescanmsg = ODMad::PI().errMsg();
    return prescanmsg.isEmpty() ? 0 : prescanmsg.buf();
}
