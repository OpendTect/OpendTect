/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : April 2017
-*/


#include "odplugin.h"
#include "crssystem.h"
#include "filepath.h"
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
    FilePath fp( mGetSetupFileName("CRS") );
    Coords::ProjectionRepos* repos = new Coords::ProjectionRepos( "EPSG",
				toUiString("Standard EPSG Projectons") );
    fp.add( "epsg" );
    repos->readFromFile( fp.fullPath() );
    Coords::ProjectionRepos::addRepos( repos );

    repos = new Coords::ProjectionRepos( "ESRI", toUiString("ESRI Projectons"));
    fp.setFileName( "esri" );
    repos->readFromFile( fp.fullPath() );
    Coords::ProjectionRepos::addRepos( repos );

    SI().readSavedCoordSystem();
    return 0;
}
