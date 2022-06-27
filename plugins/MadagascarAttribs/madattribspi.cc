/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Helene Huck
 * DATE     : Sep 2009
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
