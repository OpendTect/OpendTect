/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Raman Singh
 * DATE     : July 2008
-*/

static const char* rcsID = "$Id: gmtpi.cc,v 1.4 2009-04-06 07:19:31 cvsranojay Exp $";

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
    	"Plots Surface data using GMT mapping tool" };
    return &retpi;
}


mExternC const char* InitGMTPlugin( int, char** )
{
    GMT::initStdClasses();

    return 0;
}
