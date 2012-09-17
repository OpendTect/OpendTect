/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Karthika
 * DATE     : Sep 2009
-*/

static const char* rcsID = "$Id: bouncypi.cc,v 1.6 2011/04/21 13:09:13 cvsbert Exp $";

#include "odplugin.h"


mDefODPluginEarlyLoad(Bouncy)
mDefODPluginInfo(Bouncy)
{
    static PluginInfo retpi = {
	"Bouncy thingy (Non-UI)",
	"dGB (Karthika)",
	"4.2",
    	"Having some fun in OpendTect." };
    return &retpi;
}


mDefODInitPlugin(Bouncy)
{
    return 0;
}
