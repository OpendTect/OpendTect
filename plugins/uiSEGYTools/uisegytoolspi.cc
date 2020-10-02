/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Wayne Mogg
 * DATE     : Oct 2020
-*/


#include "uisegytoolsmod.h"
#include "uisegycommon.h"
#include "odplugin.h"


mDefODPluginInfo(uiSEGYTools)
{
    mDefineStaticLocalObject (PluginInfo, retpi, (
	"SEG-Y support (Base)",
	"OpendTect",
	"dGB (Wayne Mogg)",
	"=od",
	"Supports the SEG-Y format" ));
    return &retpi;
}


mDefODInitPlugin(uiSEGYTools)
{
    uiSEGY::initClasses();
    return nullptr;
}
