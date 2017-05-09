/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : April 2017
-*/


#include "uicrssystem.h"
#include "uimsg.h"
#include "odplugin.h"


mDefODPluginInfo(uiCRS)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"Coordinate Reference System",
	"OpendTect",
	"dGB (Raman)",
	"6.2",
	"User interface for providing a library of Coordinate Reference Systems"
	    " that can be set at Survey level" ));
    return &retpi;
}


mDefODInitPlugin(uiCRS)
{
    Coords::uiProjectionBasedSystem::initClass();
    return 0;
}
