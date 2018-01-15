/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : March 1994
 * FUNCTION : general utilities
-*/

#include "oduicommon.h"

#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "settings.h"
#include "dirlist.h"

static const char* sStyleDir = "Styles";

BufferString OD::getActiveStyleName()
{
    BufferString stylenm = Settings::common().find( "dTect.StyleName" );
    if ( stylenm.isEmpty() )
    {
	stylenm = GetEnvVar( "OD_STYLENAME" );
	if ( stylenm.isEmpty() )
	    stylenm.set( "default" );
    }

    return stylenm;
}

namespace OD
{

mGlobal(Basic) bool setActiveStyleName(const char*);
bool setActiveStyleName( const char* nm )
{
    const BufferString curstylenm = getActiveStyleName();
    if ( curstylenm == nm )
	return false;
    Settings::common().update( "dTect.StyleName", nm );
    return true;
}

}


static BufferString getFileName( const File::Path& basedir,
				 const char* filebase,
				 const char* ext )
{
    File::Path fp( basedir, filebase );
    fp.setExtension( ext );
    return fp.fullPath();
}


static bool isFilePresent( const File::Path& basedir, const char* filebase,
			   const char* ext, BufferString& filenm )
{
    filenm = getFileName( basedir, filebase, ext );
    return File::exists( filenm );
}


#define mDefStyleDirs() \
    const File::Path userstyledir( GetSettingsFileName(sStyleDir) ); \
    const File::Path appstyledir( mGetApplSetupDataDir(), sStyleDir ); \
    const File::Path inststyledir( mGetSWDirDataDir(), sStyleDir )

BufferString OD::getStyleFile( const char* stylenm, const char* ext )
{
    mDefStyleDirs();

#define mRetIfExists(pathfp,filebase) \
    if ( isFilePresent(pathfp,filebase,ext,stylefnm) ) \
	return stylefnm

    BufferString stylefnm;
    mRetIfExists( userstyledir, stylenm );
    mRetIfExists( appstyledir, stylenm );
    mRetIfExists( inststyledir, stylenm );
    mRetIfExists( userstyledir, "default" );
    mRetIfExists( appstyledir, "default" );
    mRetIfExists( inststyledir, "default" );

    pFreeFnErrMsg( "Huh? The default style file should always exist" );
    return BufferString::empty();
}


static void getStyleNames( const File::Path& dirfp, BufferStringSet& nms )
{
    DirList dl( dirfp.fullPath(), File::FilesInDir, "*.qss" );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	File::Path fp( dl.get( idx ) );
	fp.setExtension( 0 );
	nms.addIfNew( fp.fileName() );
    }
}


void OD::getStyleNames( BufferStringSet& nms )
{
    nms.setEmpty();
    mDefStyleDirs();
    getStyleNames( userstyledir, nms );
    getStyleNames( appstyledir, nms );
    getStyleNames( inststyledir, nms );
}
