
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : NOv 2003
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "uitextureattrib.h"
#include "odplugin.h"


mDefODPluginInfo(uiTexture)
{
    static PluginInfo retpi = {
	"Texture Attribute",
	"dGB (Paul)",
	"0.9",
    	"User interface for Texture attributes" };
    return &retpi;
}

mDefODInitPlugin(uiTexture)
{
    uiTextureAttrib::initClass();
    return 0;
}
