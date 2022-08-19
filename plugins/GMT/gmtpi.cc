/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

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
