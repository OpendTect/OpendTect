/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 3-8-1994
-*/

static const char* rcsID = "$Id: dirlist.cc,v 1.3 2001-05-31 12:55:12 windev Exp $";

#include "dirlist.h"
#include "filegen.h"

#ifdef win
#include <windows.h>
#else
#include <unistd.h>
#include <dirent.h>
#endif



DirList::DirList( const char* dirname, int dirindic )
	: UserIDObjectSet<UserIDObject>(dirname)
	, dir(dirname?dirname:".")
	, indic(dirindic)
{
    update();
}


void DirList::update()
{
    deepErase();
#ifdef __win__
    WIN32_FIND_DATA	dat;
    HANDLE		mhndl;

    mhndl = FindFirstFile( dir, &dat );

    do
    {
        if ( (dat.cFileName)[0] == '.' && (dat.cFileName)[1] == '\0' ) continue;
        if ( (dat.cFileName)[0] == '.' && (dat.cFileName)[1] == '.'
	  && (dat.cFileName)[2] == '\0' ) continue;

	if ( indic )
	{
	    FileNameString fullnm( File_getFullPath(dir,dat.cFileName) );
	    int isdir = File_isDirectory( fullnm );
	    if ( (indic>0 && !isdir) || (indic<0 && isdir) )
		continue;
	}

	*this += new UserIDObject( dat.cFileName );

    } while ( FindNextFile(mhndl,&dat) );

#else
    DIR* dirp = opendir( dir );
    if ( !dirp ) return;

    struct dirent* dp;
    for ( dp = readdir(dirp); dp; dp = readdir(dirp) )
    {
        if ( (dp->d_name)[0] == '.' && (dp->d_name)[1] == '\0' ) continue;
        if ( (dp->d_name)[0] == '.' && (dp->d_name)[1] == '.'
	  && (dp->d_name)[2] == '\0' ) continue;

	if ( indic )
	{
	    FileNameString fullnm( File_getFullPath(dir,dp->d_name) );
	    int isdir = File_isDirectory( fullnm );
	    if ( (indic>0 && !isdir) || (indic<0 && isdir) )
		continue;
	}

	*this += new UserIDObject( dp->d_name );
    }
    closedir(dirp);
#endif
}


DirList::~DirList()
{
    deepErase();
}
