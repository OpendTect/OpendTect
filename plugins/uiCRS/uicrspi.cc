/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : April 2017
-*/


#include "uicrssystem.h"
#include "uimsg.h"
#include "odplugin.h"


mDefODInitPlugin(uiCRS)
{
    Coords::uiProjectionBasedSystem::initClass();
    return 0;
}


mDefODPluginInfo(uiCRS)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"CRS: Coordinate Reference Systems - powered by PROJ.4",
	mODCRSPluginPackage,
	mODPluginCreator, mODPluginVersion,
	"Provides support for Coordinate Reference Systems "
		    "using the PROJ.4 services" ) );
    retpi.useronoffselectable_ = true;
    retpi.url_ = "proj4.org";
    mSetPackageDisplayName( retpi,
			    Coords::uiProjectionBasedSystem::pkgDispNm() );
    return &retpi;
}

mDefODPluginSurvRelToolsLoadFn(uiCRS)
{
    Coords::uiProjectionBasedSystem::initClass();
}
