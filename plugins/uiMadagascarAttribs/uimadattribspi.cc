/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Helene Huck
 * DATE     : Sep 2009
-*/

static const char* rcsID = "$Id: uimadattribspi.cc,v 1.1 2009-10-27 15:55:06 cvshelene Exp $";

#include "uimadagcattrib.h"
#include "plugins.h"

mExternC int GetuiMadagascarAttribsPluginType()
{
    return PI_AUTO_INIT_LATE;
}


mExternC PluginInfo* GetuiMadagascarAttribsPluginInfo()
{
    static PluginInfo retpii = {
	"Madagascar attributes (UI)",
	"dGB - Helene Huck",
	"=od",
	"Transforming Madagascar routines into OpendTect attributes." };
    return &retpii;
}


mExternC const char* InituiMadagascarAttribsPlugin( int, char** )
{
    uiMadAGCAttrib::initClass();

    return 0; // All OK - no error messages
}
