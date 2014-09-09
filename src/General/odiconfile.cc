/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 2014
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "odiconfile.h"

#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "separstr.h"
#include "settings.h"


static bool getPngFileName( BufferString& fnm )
{
    if ( File::exists( fnm ) )
	return true;

    const BufferString pngfnm( fnm, ".png" );
    if ( File::exists(pngfnm) )
    {
	fnm = pngfnm;
	return true;
    }

    return false;
}


static bool getFullFilename( const char* inp, BufferString& fname )
{
    fname = inp;
    if ( fname.isEmpty() )
	return false;

    FilePath fp( fname );
    if ( !fp.isAbsolute() )
    {
	BufferString icsetnm;
	Settings::common().get( "Icon set name", icsetnm );
	if ( icsetnm.isEmpty() )
	    icsetnm = "Default";
	const BufferString dirnm( "icons.", icsetnm );

	fp.setPath( GetSettingsFileName(dirnm) );
	fname = fp.fullPath();
	if ( getPngFileName(fname) )
	    return true;

	fp.setPath( mGetSetupFileName(dirnm) );
	fname = fp.fullPath();
	if ( getPngFileName(fname) )
	    return true;

	// Not in selected icon set? Then we take the one in icons.Default
	fp.setPath( mGetSetupFileName("icons.Default") );
	fname = fp.fullPath();
    }

    return getPngFileName( fname );
}


OD::IconFile::IconFile( const char* identifier )
{
    if ( !getFullFilename(identifier,fullpath_) )
    {
	// final fallback (icon simply missing even from release)
	pErrMsg(BufferString("Icon not found: '",identifier,"'"));
	fullpath_ = FilePath(mGetSetupFileName("icons.Default"),
			"iconnotfound.png").fullPath();
    }
}


bool OD::IconFile::isPresent( const char* identifier )
{
    BufferString fname;
    return getFullFilename( identifier, fname );
}
