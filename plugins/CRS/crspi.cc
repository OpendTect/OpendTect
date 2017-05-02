/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : April 2017
-*/


#include "odplugin.h"
#include "crsproj.h"
#include "oddirs.h"

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
    Coords::ProjectionRepos* repos = new Coords::ProjectionRepos( "EPSG",
	    			toUiString("Standard EPSG Projectons") );
    BufferString epsgfnm = mGetSetupFileName( "epsg" );
    repos->readFromFile( epsgfnm );
    return 0;
}

