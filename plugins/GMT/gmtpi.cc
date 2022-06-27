/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : July 2008
-*/


#include "initgmtplugin.h"
#include "odplugin.h"
#include "gmtmod.h"


mDefODPluginEarlyLoad(GMT)
mDefODPluginInfo(GMT)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"GMT Link (Base)",
	"OpendTect",
	"dGB Earth Sciences (Raman)",
	"=od",
	"A link to the GMT mapping tool"
	    "\nSee https://www.generic-mapping-tools.org for info on GMT" ))
    return &retpi;
}


mDefODInitPlugin(GMT)
{
    GMT::initStdClasses();

    return nullptr;
}
