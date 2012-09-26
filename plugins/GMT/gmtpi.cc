/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : July 2008
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "initgmtplugin.h"
#include "odplugin.h"
#include "gmtmod.h"

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
