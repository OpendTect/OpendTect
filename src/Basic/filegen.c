/*+
 * NAME     : %M%
 * AUTHOR   : A.H. Bril
 * DATE     : Sept 1993
 * FUNCTION : file utilities
-*/

static const char* rcsID = "$Id: filegen.c,v 1.2 1999-10-18 14:05:48 dgb Exp $";

/*@+
\section{File Functions}
%====================================
This module contains functions concerning files.
@-*/

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <malloc.h>
#include <stdlib.h>
#include "string2.h"
#include "filegen.h"
#include "genc.h"

static struct stat statbuf;


/*--------------------------------------------------@+File_exists
 checks whether a file exists.
\list{{\b Input}}
        \list{fname}the file name
\list{{\b Return}}YES or NO
-----------------------------------------------------------------@-*/
int File_exists( const char* fname )
{
    return !fname || stat(fname,&statbuf) < 0 ? NO : YES;
}


/*--------------------------------------------------@+File_isEmpty
 checks whether a file is empty.
 If it doesn't exists, it is considered empty.
\list{{\b Input}}
        \list{fname}the file name
\list{{\b Return}}YES or NO
-----------------------------------------------------------------@-*/
int File_isEmpty( const char* fname )
{
    return !fname || stat(fname,&statbuf) < 0 || statbuf.st_size < 1 ? YES : NO;
}


#include <dirent.h>
/*--------------------------------------------------@+File_isDirectory
 checks if the given name is a directory name.
\list{{\b Input}}
        \list{fname}the directory name
\list{{\b Return}}YES or NO
-----------------------------------------------------------------@-*/
int File_isDirectory( const char* dirname )
{
    DIR* dirptr;
    int rv = NO;

    if ( dirname && *dirname )
    {
	dirptr = opendir(dirname);
	if ( dirptr )
	{
	    closedir(dirptr);
	    return YES;
	}
    }

    return NO;
}


/*--------------------------------------------------@+File_createDir
 creates a directory
\list{{\b Input}}
        \list{dirname}the directory name (full path)
        \list{mode}the mode
\list{{\b Return}}YES or NO
-----------------------------------------------------------------@-*/
int File_createDir( const char* dirname, int mode )
{
    if ( !dirname || !*dirname ) return NO;

    if ( mode == 0 ) mode = 0755;
    return mkdir( dirname, (mode_t)mode ) < 0 ? NO : YES;
}


/*--------------------------------------------------@+File_rename
 renames a file (or directory).
\list{{\b Input}}
        \list{oldname}the name (full path) before rename
        \list{newname}the name (full path) after rename
\list{{\b Return}}YES or NO
-----------------------------------------------------------------@-*/
int File_rename( const char* oldname, const char* newname )
{
    if ( !oldname || !*oldname || !newname || !*newname ) return NO;

    if ( rename(oldname,newname) != 0 ) return NO;
    return YES;
}


/*--------------------------------------------------@+File_getFullPath
 constructs the full path name for path only and file name only.
\list{{\b Input}}
        \list{path}path only
        \list{filename}filename only
\list{{\b Return}}ptr to static buffer with full path name
-----------------------------------------------------------------@-*/
const char* File_getFullPath( const char* path, const char* filename )
{
    static FileNameString pathbuf;
    int lastpos;

    if ( path != pathbuf ) strcpy( pathbuf, path && *path ? path : "." );
    if ( !filename || !*filename ) return pathbuf;

    if ( !strcmp(filename,"..") ) return File_getPathOnly(pathbuf);

    lastpos = strlen( pathbuf ) - 1;
    if ( lastpos >= 0 && pathbuf[lastpos] != '/' )
	strcat( pathbuf, "/" );

    strcat( pathbuf, filename );
    return pathbuf;
}


/*--------------------------------------------------@+File_getPathOnly
 returns the path only from a full pathname.
\list{{\b Input}}
        \list{fullpath}full path: path + filename
\list{{\b Return}}ptr to static buffer with path only
\NOTE: function is not entirely safe when ".." is part of the path.
-----------------------------------------------------------------@-*/
const char* File_getPathOnly( const char* fullpath )
{
    static FileNameString pathbuf;
    char* chptr;

    if ( fullpath != pathbuf ) strcpy( pathbuf, fullpath ? fullpath : "" );

    if ( !pathbuf[0] || !strcmp(pathbuf,".") )
    {
	strcpy( pathbuf, ".." );
	return pathbuf;
    }

    /* search for last occurrence of / in path */
    chptr = strrchr( pathbuf, '/' );
    if ( chptr ) *chptr = '\0';
    else	 strcpy( pathbuf, "." );

    return pathbuf;
}


/*--------------------------------------------------@+File_getFileName
 returns the filename without any path prefixes.
\list{{\b Input}}
        \list{fullpath}full path: path + filename
\list{{\b Return}}ptr to static buffer with filename only
-----------------------------------------------------------------@-*/
const char* File_getFileName( const char* fullpath )
{
    static FileNameString pathbuf;
    const char* chptr;
    pathbuf[0] = '\0';
    if ( !fullpath ) return pathbuf;

    /* search for last occurrence of / in path */
    chptr = strrchr( fullpath, '/' );
    if ( chptr ) chptr++;
    else	 chptr = fullpath;

    strcpy( pathbuf, chptr );
    return pathbuf;
}


/*--------------------------------------------------@+File_getBaseName
 returns the basename, i.e. all extensions and path prefixes removed.
\list{{\b Input}}
        \list{fullpath}full path: path + basename + extensions
\list{{\b Return}}ptr to static buffer with basename only
-----------------------------------------------------------------@-*/
const char* File_getBaseName( const char* fullpath )
{
    static FileNameString pathbuf;
    char* chptr;
    File_getFileName( fullpath );

    /* search for first occurrence of . in filename */
    chptr = strchr( pathbuf, '.' );
    if ( chptr ) *chptr = '\0';

    return pathbuf;
}


/*--------------------------------------------------@+File_getTempFileName
 returns a temporary file name. Both input strings may be null, when ext is empty, there will still be a dot ('.'). When null, not. 
\list{{\b Input}}
        \list{id}An id which distinguished this file from other of same process
        \list{ext}Extension of file
        \list{full}If NO, local file name, otherwise full path
\list{{\b Return}}ptr to static buffer with temp file name
-----------------------------------------------------------------@-*/
const char* File_getTempFileName( const char* id, const char* ext, int full )
{
    static FileNameString pathbuf;

    if ( id && *id )
	sprintf( pathbuf, "%sdgb%s%d", full ? "/tmp/" : "", id, getpid() );
    else
	sprintf( pathbuf, "%sdgb%d", full ? "/tmp/" : "", getpid() );

    if ( ext )
    {
	strcat( pathbuf, "." );
	strcat( pathbuf, ext );
    }

    return pathbuf;
}


/*--------------------------------------------------@+File_isAbsPath
 returns YES if path is absolute (<-> relative). On UNIX, checks whether path
starts with '/'. If fname is null, stdin or stdout are assumed which signify
absolute paths, so YES is returned.
\list{{\b Input}}
        \list{fname}File name
\list{{\b Return}}YES or NO
-----------------------------------------------------------------@-*/
int File_isAbsPath( const char* fname )
{
    return !fname || *fname == '/';
}


/*--------------------------------------------------@+File_copy
 copies (a) file(s).
\list{{\b Input}}
        \list{from}the name to copy from
        \list{to}the target name
        \list{recursive}YES if recursive downward
\list{{\b Return}}YES or NO
-----------------------------------------------------------------@-*/
int File_copy( const char* from, const char* to, int recursive )
{
    char* cmd;
    int len, retval;
    if ( !from || !to ) return NO;

    len = strlen( from ) + strlen( to ) + 25;
    cmd = mMALLOC(len,char);
    
    strcpy( cmd, "/bin/cp " );
    if ( recursive ) strcat( cmd, "-r " );
    strcat( cmd, " " );
    strcat( cmd, from );
    strcat( cmd, " " );
    strcat( cmd, to );

    retval = system( cmd ) != -1 ? YES : NO;
    if ( retval ) retval = File_exists( to );
    mFREE(cmd);
    return retval;
}


/*--------------------------------------------------@+File_link
 links a file to another name.
\list{{\b Input}}
        \list{from}the name to link from
        \list{to}the target name
\list{{\b Return}}YES or NO
-----------------------------------------------------------------@-*/
int File_link( const char* from, const char* to )
{
    char cmd[PATH_LENGTH+PATH_LENGTH+10];
    if ( !from || !to ) return NO;
    
    strcpy( cmd, "ln -s " );
    strcat( cmd, from );
    strcat( cmd, " " );
    strcat( cmd, to );

    system( cmd );
    return File_exists( to ) ? YES : NO;
}


/*--------------------------------------------------@+File_remove
 removes a file (or directory).
\list{{\b Input}}
        \list{fname}the name (full path) of the file or directory to remove
        \list{force}YES when action should be 'force'-d
        \list{recursive}YES if recursive downward
\list{{\b Return}}YES or NO
\NOTE: if force or recursive a system rm is executed. Otherwise, unlink()
is used. In that case, the return value actually tells wether the removal
has failed or succeeded. In the other cases, File_exists() is used and part
of the removal may not have failed when NO is returned.
-----------------------------------------------------------------@-*/
int File_remove( const char* fname, int force, int recursive )
{
    char* cmd;
    int len, retval;
    if ( !fname ) return NO;

    len = strlen( fname ) + 30;
    cmd = mMALLOC(len,char);

    if ( !recursive )
	return unlink((char*)fname) ? NO : YES;

    strcpy( cmd,     "/bin/rm -" );
    if ( recursive ) strcat( cmd, "r" );
    if ( force )     strcat( cmd, "f" );
                     strcat( cmd, " " );
                     strcat( cmd, fname );

    retval = system( cmd ) ? NO : YES;
    if ( retval && File_exists( fname ) ) retval = NO;
    mFREE(cmd);
    return retval;
}
