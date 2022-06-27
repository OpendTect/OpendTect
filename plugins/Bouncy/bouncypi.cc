/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Karthika
 * DATE     : Sep 2009
-*/


#include "odplugin.h"

#include "bouncymod.h"


mDefODPluginEarlyLoad(Bouncy)
mDefODPluginInfo(Bouncy)
{
    mDefineStaticLocalObject(PluginInfo, retpi, (
	"Bouncy thingy (Base)",
	"OpendTect",
	"dGB Earth Sciences (Karthika)",
	"=od",
	"Having some fun in OpendTect." ))
    return &retpi;
}


mDefODInitPlugin(Bouncy)
{
    return nullptr;
}
