/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 3-8-1994
-*/

static const char* rcsID = "$Id: dirlist.cc,v 1.7 2003-10-30 12:15:31 bert Exp $";

#include "dirlist.h"
#include "globexpr.h"
#include "filegen.h"

#ifdef __win__
#include <windows.h>
#else
#include <unistd.h>
#include <dirent.h>
#endif



DirList::DirList( const char* dirname, DirList::Type t, const char* msk )
	: BufferStringSet(true)
    	, dir_(dirname?dirname:".")
	, type_(t)
    	, mask_(msk)
{
    update();
}


void DirList::update()
{
    deepErase();
    const bool havemask = mask_ != "";
    GlobExpr ge( mask_.buf() );

#ifdef __win__
    WIN32_FIND_DATA	dat;
    HANDLE		mhndl;

    BufferString dirnm = dir_;
    dirnm += "\\*";

    mhndl = FindFirstFile( (const char*)dirnm, &dat );

    do
    {
        if ( (dat.cFileName)[0] == '.' && (dat.cFileName)[1] == '\0' ) continue;
        if ( (dat.cFileName)[0] == '.' && (dat.cFileName)[1] == '.'
	  && (dat.cFileName)[2] == '\0' ) continue;

	if ( type_ != AllEntries )
	{
	    FileNameString fullnm( File_getFullPath(dir_.buf(),dat.cFileName) );
	    if ( (type_ == FilesOnly) == (bool)File_isDirectory(fullnm) )
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

	if ( type_ != AllEntries )
	{
	    FileNameString fullnm( File_getFullPath(dir_.buf(),dp->d_name) );
	    if ( (type_ == FilesOnly) == (bool)File_isDirectory(fullnm) )
		continue;
	}

	if ( havemask && !ge.matches(dp->d_name) )
	    continue;

	add( dp->d_name );
    }
    closedir(dirp);
#endif
}
