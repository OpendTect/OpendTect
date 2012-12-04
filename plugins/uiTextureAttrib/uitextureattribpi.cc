
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : NOv 2003
-*/

static const char* rcsID mUnusedVar = "$Id: uitextureattribpi.cc 27530 2012-11-19 09:49:13Z kristofer.tingdahl@dgbes.com $";

#include "uitextureattrib.h"
#include "odplugin.h"


mDefODPluginInfo(uiTextureAttrib)
{
    static PluginInfo retpi = 
    {
	"Texture Attribute",
	"dGB (Paul)",
	"0.9",
    	"User interface for Texture attributes" 
    };
    return &retpi;
}

mDefODInitPlugin(uiTextureAttrib)
{
    uiTextureAttrib::initClass();
    return 0;
}
