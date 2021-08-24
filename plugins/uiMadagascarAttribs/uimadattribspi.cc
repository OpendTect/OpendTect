/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Helene Huck
 * DATE     : Sep 2009
-*/


#include "uimadagcattrib.h"
#include "odplugin.h"


mDefODPluginInfo(uiMadagascarAttribs)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"Madagascar Attributes",
	"OpendTect",
	"dGB - Helene Huck",
	"=od",
	"Transforming Madagascar routines into OpendTect attributes." ) );
    return &retpi;
}


mDefODInitPlugin(uiMadagascarAttribs)
{
    uiMadAGCAttrib::initClass();

    return 0; // All OK - no error messages
}
