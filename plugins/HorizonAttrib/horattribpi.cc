/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		September 2006
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";


#include "horizonattrib.h"
#include "odplugin.h"
#include "moddepmgr.h"


mDefODPluginEarlyLoad(HorizonAttrib)
mDefODPluginInfo(HorizonAttrib)
{
    static PluginInfo retpii = {
	"Horizon-Attribute (Base)",
	"dGB (Nanne)",
	"=od",
	"The 'Horizon' attribute plugin." };
    return &retpii;
}


mDefODInitPlugin(HorizonAttrib)
{
    OD::ModDeps().ensureLoaded( "EarthModel" );
    Attrib::Horizon::initClass();

    return 0; // All OK - no error messages
}
