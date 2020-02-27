/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : July 2008
-*/


#include "gmtmod.h"
#include "gmtdef.h"
#include "initgmtplugin.h"
#include "odplugin.h"


mDefODPluginEarlyLoad(GMT)
mDefODPluginInfo(GMT)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"GMT Link (Base)",
	"OpendTect",
	"dGB (Raman Singh)",
	"=od",
	"GMT mapping tool - base") );
    return &retpi;
}


mDefODInitPlugin(GMT)
{
    GMT::initStdClasses();

    return 0;
}
