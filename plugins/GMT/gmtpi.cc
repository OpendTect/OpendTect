/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : July 2008
-*/

static const char* rcsID = "$Id: gmtpi.cc,v 1.6 2010-11-08 11:48:22 cvsbert Exp $";

#include "initgmt.h"
#include "plugins.h"

mExternC int GetGMTPluginType()
{
    return PI_AUTO_INIT_EARLY;
}


mExternC PluginInfo* GetGMTPluginInfo()
{
    static PluginInfo retpi = {
	"GMT Base",
	"dGB (Raman)",
	"3.2",
    	"GMT mapping tool - base" };
    return &retpi;
}


mExternC const char* InitGMTPlugin( int, char** )
{
    GMT::initStdClasses();

    return 0;
}
