/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisegytoolsmod.h"
#include "uisegycommon.h"

#include "odplugin.h"


mDefODPluginInfo(uiSEGYTools)
{
    static PluginInfo retpi(
	"SEG-Y support tools (GUI)",
	"Implementation of the SEG-Y format handlers" );
    return &retpi;
}


mDefODInitPlugin(uiSEGYTools)
{
    uiSEGY::initClasses();

    return nullptr;
}
