/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uimadagcattrib.h"
#include "odplugin.h"


mDefODPluginInfo(uiMadagascarAttribs)
{
    mDefineStaticLocalObject( PluginInfo, retpi, (
	"Madagascar Attributes (GUI)",
	"OpendTect",
	"dGB Earth Sciences (Helene)",
	"=od",
	"User Interface for Transforming Madagascar routines "
	"into OpendTect attributes." ))
    return &retpi;
}


mDefODInitPlugin(uiMadagascarAttribs)
{
    uiMadAGCAttrib::initClass();

    return nullptr; // All OK - no error messages
}
