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
    static BufferString infostr;
    infostr.set( "Plugin to add Coordinate Reference System support" )
	   .addNewLine(2)
	   .add("Using PROJ version: ").add( Coords::getProjVersion() )
	   .addNewLine().add( Coords::getEPSGDBStr() );

    static PluginInfo retpi(
	"Coordinate Reference System (base)",
	infostr.buf() );
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

const char* initCRSPlugin()
{
#ifdef OD_NO_PROJ
    return "proj dependency is disabled";
#else
    const char* msg = Coords::initCRSDatabase();
    if ( msg && *msg )
	return msg;

    Coords::ProjectionBasedSystem::initClass();

    SetWGS84( Coords::Projection::sWGS84ProjDispString(),
	      Coords::ProjectionBasedSystem::getWGS84LLSystem() );
    return nullptr;
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

    return initCRSPlugin();
}
