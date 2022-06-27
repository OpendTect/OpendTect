/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Helene Huck
 * DATE     : Sep 2009
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
