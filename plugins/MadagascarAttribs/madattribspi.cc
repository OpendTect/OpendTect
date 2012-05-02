/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Helene Huck
 * DATE     : Sep 2009
-*/

static const char* mUnusedVar rcsID = "$Id: madattribspi.cc,v 1.4 2012-05-02 11:52:47 cvskris Exp $";

#include "madagcattrib.h"
#include "odplugin.h"

mDefODPluginEarlyLoad(MadagascarAttribs)
mDefODPluginInfo(MadagascarAttribs)
{
    static PluginInfo retpii = {
	"Madagascar Attributes (Base)",
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
