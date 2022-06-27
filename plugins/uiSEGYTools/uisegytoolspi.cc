/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Wayne Mogg
 * DATE     : Jan 2021
-*/


#include "uisegytoolsmod.h"
#include "uisegycommon.h"
#include "odplugin.h"


mDefODPluginInfo(uiSEGYTools)
{
    mDefineStaticLocalObject( PluginInfo, retpi, (
	"SEG-Y support tools (GUI)",
	"OpendTect",
	"dGB Earth Sciences (Wayne Mogg)",
	"=od",
	"Implementation of the SEG-Y format handlers" ))
    return &retpi;
}


mDefODInitPlugin(uiSEGYTools)
{
    uiSEGY::initClasses();
    return nullptr;
}
