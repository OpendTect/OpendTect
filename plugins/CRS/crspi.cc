/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : April 2017
-*/


#include "odplugin.h"
#include "crssystem.h"
#include "oddirs.h"
#include "survinfo.h"

mDefODPluginEarlyLoad(CRS)
mDefODPluginInfo(CRS)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"Coordinate Reference System (base)",
	"OpendTect",
	"dGB (Raman)",
	"=od",
	"Coordinate Reference System - base" ));
    return &retpi;
}


mDefODInitPlugin(CRS)
{
    Coords::ProjectionBasedSystem::initClass();
    Coords::ProjectionRepos* repos = new Coords::ProjectionRepos( "EPSG",
				toUiString("Standard EPSG Projectons") );
    BufferString epsgfnm = mGetSetupFileName( "epsg" );
    repos->readFromFile( epsgfnm );
    Coords::ProjectionRepos::addRepos( repos );
    SI().readSavedCoordSystem();
    return 0;
}
