/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : April 2017
-*/


#include "odplugin.h"

#include "crssystem.h"
#include "file.h"
#include "filepath.h"
#include "genc.h"
#include "legal.h"
#include "od_istream.h"
#include "oddirs.h"
#include "survinfo.h"

mDefODPluginEarlyLoad(CRS)
mDefODPluginInfo(CRS)
{
    mDefineStaticLocalObject( PluginInfo, retpi, (
	"Coordinate Reference System (base)",
	"OpendTect",
	"dGB Earth Sciences (Raman)",
	"=od",
	"Coordinate Reference System - base" ));
    return &retpi;
}


static mUnusedVar uiString* legalText()
{
    uiString* ret = new uiString;
    FilePath fp( mGetSetupFileName("CRS"), "COPYING" );
    if ( File::exists(fp.fullPath()) )
    {
	BufferString legaltxt;
	od_istream strm( fp.fullPath() );
	if ( strm.getAll(legaltxt) )
	    *ret = toUiString( legaltxt );
    }

    return ret;
}

namespace Coords
{ extern "C" { mGlobal(Basic) void SetWGS84(const char*,CoordSystem*); } }

mDefODInitPlugin(CRS)
{
    if ( !NeedDataBase() )
	return nullptr;

#ifndef OD_NO_PROJ
    legalInformation().addCreator( legalText, "PROJ" );

    Coords::initCRSDatabase();
    Coords::ProjectionBasedSystem::initClass();
    SI().readSavedCoordSystem();
    SetWGS84( Coords::Projection::sWGS84ProjDispString(),
	      Coords::ProjectionBasedSystem::getWGS84LLSystem() );
#endif

    return nullptr;
}
