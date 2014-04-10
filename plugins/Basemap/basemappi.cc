/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Nanne Hemstra
 * DATE     : January 2014
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "odplugin.h"
#include "basemapmod.h"

mDefODPluginEarlyLoad(Basemap)
mDefODPluginInfo(Basemap)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"Basemap (base)",
	"dGB (Nanne)",
	"=od",
    	"Basemap - base") );
    return &retpi;
}


mDefODInitPlugin(Basemap)
{
    return 0;
}
