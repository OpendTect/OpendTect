/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Karthika
 * DATE     : Sep 2009
-*/

static const char* rcsID = "$Id: bouncypi.cc,v 1.5 2009-09-24 10:42:40 cvsnanne Exp $";

#include "plugins.h"


mExternC int GetBouncyPluginType()
{
    return PI_AUTO_INIT_EARLY;
}


mExternC PluginInfo* GetBouncyPluginInfo()
{
    static PluginInfo retpi = {
	"Bouncy thingy (Non-UI)",
	"dGB (Karthika)",
	"4.0",
    	"Having some fun in OpendTect." };
    return &retpi;
}


mExternC const char* InitBouncyPlugin( int, char** )
{
    return 0;
}
