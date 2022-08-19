/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "madagcattrib.h"
#include "odplugin.h"

mDefODPluginEarlyLoad(MadagascarAttribs)
mDefODPluginInfo(MadagascarAttribs)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"Madagascar Attributes (Base)",
	"OpendTect",
	"dGB Earth Sciences - Helene Huck",
	"=od",
	"Transforming Madagascar routines into OpendTect attributes." ))
    return &retpi;
}


mDefODInitPlugin(MadagascarAttribs)
{
    Attrib::MadAGC::initClass();

    return nullptr; // All OK - no error messages
}
