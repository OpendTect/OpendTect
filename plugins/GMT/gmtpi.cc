/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : July 2008
-*/

static const char* rcsID = "$Id: gmtpi.cc,v 1.5 2009-07-22 16:01:27 cvsbert Exp $";

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
