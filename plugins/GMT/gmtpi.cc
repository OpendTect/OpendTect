/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : July 2008
-*/

static const char* mUnusedVar rcsID = "$Id: gmtpi.cc,v 1.11 2012-05-02 11:52:45 cvskris Exp $";

#include "initgmtplugin.h"
#include "odplugin.h"

mDefODPluginEarlyLoad(GMT)
mDefODPluginInfo(GMT)
{
    static PluginInfo retpi = {
	"GMT (base)",
	"dGB (Raman)",
	"=od",
    	"GMT mapping tool - base" };
    return &retpi;
}


mDefODInitPlugin(GMT)
{
    GMT::initStdClasses();

    return 0;
}
