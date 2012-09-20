
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Paul
 * DATE     : Sep 2012
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "textureattrib.h"
#include "odplugin.h"


mDefODPluginEarlyLoad(TextureAttrib)
mDefODPluginInfo(TextureAttrib)
{
    static PluginInfo retpi = {
	"Texture Attribute (base)",
	"dGB (Paul)",
	"0.9",
    	"Implements Texture attributes" };
    return &retpi;
}


mDefODInitPlugin(TextureAttrib)
{
    Attrib::Texture::initClass();

    return 0;
}
