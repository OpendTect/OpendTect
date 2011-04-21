/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : July 2007
-*/

static const char* rcsID = "$Id: madpi.cc,v 1.5 2011-04-21 13:09:13 cvsbert Exp $";

#include "maddefs.h"
#include "odplugin.h"

mDefODPluginEarlyLoad(Madagascar)
mDefODPluginInfo(Madagascar)
{
    static PluginInfo retpii = {
	"Madagascar base",
	"dGB - Bert Bril",
	"=od",
	"The Madagascar batch-level tools." };
    return &retpii;
}


mDefODInitPlugin(Madagascar)
{
    static BufferString prescanmsg = ODMad::PI().errMsg();
    return prescanmsg.isEmpty() ? 0 : prescanmsg.buf();
}
