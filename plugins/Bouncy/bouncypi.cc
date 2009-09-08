/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Karthika
 * DATE     : Sep 2009
-*/

static const char* rcsID = "$Id: bouncypi.cc,v 1.3 2009-09-08 08:45:48 cvskarthika Exp $";

#include "plugins.h"


mExternC mGlobal int GetBouncyPluginType()
{
    return PI_AUTO_INIT_LATE;
}


mExternC mGlobal PluginInfo* GetBouncyPluginInfo()
{
    static PluginInfo retpi = {
	"Bouncy thingy (non-UI)",
	"dGB (Karthika)",
	"4.0",
    	"Having some fun in OpendTect." };
    return &retpi;
}


mExternC mGlobal const char* InitBouncyPlugin( int, char** )
{
    return 0;
}
