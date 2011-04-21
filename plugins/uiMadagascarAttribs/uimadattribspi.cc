/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Helene Huck
 * DATE     : Sep 2009
-*/

static const char* rcsID = "$Id: uimadattribspi.cc,v 1.2 2011-04-21 13:09:13 cvsbert Exp $";

#include "uimadagcattrib.h"
#include "odplugin.h"


mDefODPluginInfo(uiMadagascarAttribs)
{
    static PluginInfo retpii = {
	"Madagascar attributes (UI)",
	"dGB - Helene Huck",
	"=od",
	"Transforming Madagascar routines into OpendTect attributes." };
    return &retpii;
}


mDefODInitPlugin(uiMadagascarAttribs)
{
    uiMadAGCAttrib::initClass();

    return 0; // All OK - no error messages
}
