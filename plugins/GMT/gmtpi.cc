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
	"dGB (Raman)",
	"=od",
    	"GMT mapping tool - base" ));
    return &retpi;
}


mDefODInitPlugin(GMT)
{
    GMT::initStdClasses();

    return nullptr;
}
