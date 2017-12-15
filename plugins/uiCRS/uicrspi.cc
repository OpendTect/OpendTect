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
	"CRS: Coordinate Reference Systems - powered by PROJ.4",
	mODCRSPluginPackage,
	mODPluginCreator, mODPluginVersion,
	"Provides support for Coordinate Reference Systems "
		    "using the PROJ.4 services" ) );
    retpi.useronoffselectable_ = true;
    return &retpi;
}


mDefODInitPlugin(uiCRS)
{
    Coords::uiProjectionBasedSystem::initClass();
    return 0;
}

mDefODPluginSurvRelToolsLoadFn(uiCRS)
{
    Coords::uiProjectionBasedSystem::initClass();
}
