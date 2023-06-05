/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "odplugin.h"

#include "crssystem.h"
#include "genc.h"
#include "legal.h"

mDefODPluginEarlyLoad(CRS)
mDefODPluginInfo(CRS)
{
    static PluginInfo retpi(
	"Coordinate Reference System (base)",
	"Coordinate Reference System - base" );
    return &retpi;
}


static mUnusedVar uiString* projLegalText()
{
    return legalText("proj");
}

static mUnusedVar uiString* sqliteLegalText()
{
    return legalText("sqlite");
}


namespace Coords
{ extern "C" { mGlobal(Basic) void SetWGS84(const char*,CoordSystem*); } }

bool initCRSPlugin()
{
#ifdef OD_NO_PROJ
    return false;
#else
    Coords::initCRSDatabase();
    Coords::ProjectionBasedSystem::initClass();

    SetWGS84( Coords::Projection::sWGS84ProjDispString(),
	      Coords::ProjectionBasedSystem::getWGS84LLSystem() );
    return true;
#endif
}


mDefODInitPlugin(CRS)
{
    if ( !NeedDataBase() )
	return nullptr;

#ifndef OD_NO_PROJ
    legalInformation().addCreator( projLegalText, "PROJ" );
    legalInformation().addCreator( sqliteLegalText, "SQLite" );
#endif
    initCRSPlugin();

    return nullptr;
}
