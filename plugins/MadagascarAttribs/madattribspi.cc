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
    static PluginInfo retpi(
	"Madagascar Attributes (Base)",
	"Transforming Madagascar routines into OpendTect attributes." );
    return &retpi;
}


mDefODInitPlugin(MadagascarAttribs)
{
    Attrib::MadAGC::initClass();

    return nullptr; // All OK - no error messages
}
