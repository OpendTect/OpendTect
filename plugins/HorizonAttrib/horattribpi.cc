/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		September 2006
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: horattribpi.cc,v 1.12 2012-05-02 15:11:10 cvskris Exp $";


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
