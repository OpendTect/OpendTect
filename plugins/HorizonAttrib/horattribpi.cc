/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		September 2006
________________________________________________________________________

-*/


#include "horizonattrib.h"
#include "odplugin.h"
#include "moddepmgr.h"


mDefODPluginEarlyLoad(HorizonAttrib)
mDefODPluginInfo(HorizonAttrib)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"Horizon-Attribute (Base)",
	"OpendTect",
	mODPluginCreator, mODPluginVersion, mODPluginSeeMainModDesc ) );
    return &retpi;
}


mDefODInitPlugin(HorizonAttrib)
{
    OD::ModDeps().ensureLoaded( "EarthModel" );
    Attrib::Horizon::initClass();

    return 0; // All OK - no error messages
}
