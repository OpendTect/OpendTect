/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 3-8-1994
-*/

static const char* rcsID = "$Id: dirlist.cc,v 1.17 2010-03-18 05:32:31 cvsnanne Exp $";

#include "dirlist.h"

#include "file.h"
#include "filepath.h"
#include "globexpr.h"

#ifdef __win__
# include <windows.h>
#else
# include <unistd.h>
# include <dirent.h>
#endif



DirList::DirList( const char* dirname, DirList::Type t, const char* msk )
	: dir_(dirname?dirname:".")
	, type_(t)
    	, mask_(msk)
{
    update();
}


void DirList::update()
{
    erase();
    const bool havemask = !mask_.isEmpty();
    GlobExpr ge( mask_.buf(), !__iswin__ && !__ismac__  );
    FilePath fp( dir_ ); fp.add( "X" );

#ifdef __win__
    WIN32_FIND_DATA	dat;
    HANDLE		mhndl;

    BufferString dirnm = dir_;
    dirnm += "\\*";

    mhndl = FindFirstFile( (const char*)dirnm, &dat );
    if ( mhndl == INVALID_HANDLE_VALUE )
	return;

    do
    {
        if ( (dat.cFileName)[0] == '.' && (dat.cFileName)[1] == '\0' ) continue;
        if ( (dat.cFileName)[0] == '.' && (dat.cFileName)[1] == '.'
	  && (dat.cFileName)[2] == '\0' ) continue;

	if ( type_ != AllEntries )
	{
	    fp.setFileName( dat.cFileName );
	    if ( (type_==FilesOnly) == (bool)File::isDirectory(fp.fullPath()) )
		continue;
	}

	if ( havemask && !ge.matches(dat.cFileName) )
	    continue;

	add( dat.cFileName );

    } while ( FindNextFile(mhndl,&dat) );

#else
    DIR* dirp = opendir( dir_.buf() );
    if ( !dirp ) return;

    struct dirent* dp;
    for ( dp = readdir(dirp); dp; dp = readdir(dirp) )
    {
        if ( (dp->d_name)[0] == '.' && (dp->d_name)[1] == '\0' ) continue;
        if ( (dp->d_name)[0] == '.' && (dp->d_name)[1] == '.'
	  && (dp->d_name)[2] == '\0' ) continue;

	fp.setFileName( dp->d_name );
	if ( !File::exists(fp.fullPath()) )
	    continue;

	if ( type_ != AllEntries )
	{
	    if ( (type_==FilesOnly) == (bool)File::isDirectory(fp.fullPath()) )
		continue;
	}

	if ( havemask && !ge.matches(dp->d_name) )
	    continue;

	add( dp->d_name );
    }
    closedir(dirp);
#endif

    sort();
}


const char* DirList::fullPath( int idx ) const
{
    static BufferString ret;
    ret = "";

    if ( idx < size() )
    {
	FilePath fp( dirName() );
	fp.add( (*this).get(idx) );
	ret = fp.fullPath();
    }

    return ret.buf();
}
