/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Karthika
 * DATE     : Sep 2009
-*/

static const char* rcsID mUnusedVar = "$Id: bouncypi.cc,v 1.8 2012-05-02 15:11:06 cvskris Exp $";

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
