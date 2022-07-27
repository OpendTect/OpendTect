/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : April 2017
-*/


#include "moddepmgr.h"

#include "crssystem.h"
#include "file.h"
#include "filepath.h"
#include "genc.h"
#include "legal.h"
#include "od_istream.h"
#include "oddirs.h"
#include "survinfo.h"


static uiString* legalText()
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




mDefModInitFn(CRS)
{
    mIfNotFirstTime(return);
    legalInformation().addCreator( legalText, "PROJ" );
    if ( !NeedDataBase() )
	return;

    Coords::initCRSDatabase();
    Coords::ProjectionBasedSystem::initClass();
    SI().readSavedCoordSystem();
}
