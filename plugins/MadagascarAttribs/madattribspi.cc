/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Helene Huck
 * DATE     : Sep 2009
-*/

static const char* rcsID = "$Id: madattribspi.cc,v 1.2 2011-04-21 13:09:13 cvsbert Exp $";

#include "madagcattrib.h"
#include "odplugin.h"

mDefODPluginEarlyLoad(MadagascarAttribs)
mDefODPluginInfo(MadagascarAttribs)
{
    static PluginInfo retpii = {
	"Trace Match (Base)",
	"dGB - Helene Huck",
	"=od",
	"Transforming Madagascar routines into OpendTect attributes." };
    return &retpii;
}


mDefODInitPlugin(MadagascarAttribs)
{
    Attrib::MadAGC::initClass();

    return 0; // All OK - no error messages
}
