/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Helene Huck
 * DATE     : Sep 2009
-*/


#include "uimadagcattrib.h"
#include "madagcattrib.h"
#include "odplugin.h"


mDefODPluginInfo(uiMadagascarAttribs)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"Madagascar Attributes",
	mMadagascarAttribLinkPackage,
	mODPluginCreator, mODPluginVersion,
	"Allows using Madagascar routines as OpendTect attributes." ) );
    return &retpi;
}


mDefODInitPlugin(uiMadagascarAttribs)
{
    uiMadAGCAttrib::initClass();

    return 0; // All OK - no error messages
}
