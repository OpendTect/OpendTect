/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 3-8-1994
-*/

static const char* rcsID = "$Id: dirlist.cc,v 1.2 2001-05-02 13:50:14 windev Exp $";

#include "dirlist.h"
#include "filegen.h"

#ifdef win
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
}


DirList::~DirList()
{
    deepErase();
}
