/*+
 * AUTHOR   : A.H. Bril
 * DATE     : Sept 1993
 * FUNCTION : file utilities
-*/

static const char* rcsID = "$Id: filegen.c,v 1.5 2001-02-13 17:20:58 bert Exp $";

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
static const char* dirsep = sDirSep;


int File_exists( const char* fname )
{
    return !fname || stat(fname,&statbuf) < 0 ? NO : YES;
}


int File_isEmpty( const char* fname )
{
    return !fname || stat(fname,&statbuf) < 0 || statbuf.st_size < 1 ? YES : NO;
}


#include <dirent.h>
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


int File_createDir( const char* dirname, int mode )
{
    if ( !dirname || !*dirname ) return NO;

    if ( mode == 0 ) mode = 0755;
    return mkdir( dirname, (mode_t)mode ) < 0 ? NO : YES;
}


int File_rename( const char* oldname, const char* newname )
{
    if ( !oldname || !*oldname || !newname || !*newname ) return NO;

    if ( rename(oldname,newname) != 0 ) return NO;
    return YES;
}


int File_isAbsPath( const char* fname )
{
    if ( !fname ) return YES;
    skipLeadingBlanks( fname );
    return *fname == *dirsep;
}


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


const char* File_getFullPath( const char* path, const char* filename )
{
    static FileNameString pathbuf;
    int lastpos;

    if ( path != pathbuf ) strcpy( pathbuf, path && *path ? path : "." );
    if ( !filename || !*filename ) return pathbuf;

    if ( !strcmp(filename,"..") ) return File_getPathOnly(pathbuf);

    lastpos = strlen( pathbuf ) - 1;
    if ( lastpos >= 0 && pathbuf[lastpos] != *dirsep )
	strcat( pathbuf, dirsep );

    strcat( pathbuf, filename );
    return pathbuf;
}


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

    /* search for last occurrence of directory separator in path */
    chptr = strrchr( pathbuf, *dirsep );
    if ( chptr ) *chptr = '\0';
    else	 strcpy( pathbuf, "." );

    return pathbuf;
}


const char* File_getFileName( const char* fullpath )
{
    static FileNameString pathbuf;
    const char* chptr;
    pathbuf[0] = '\0';
    if ( !fullpath ) return pathbuf;

    /* search for last occurrence of directory separator in path */
    chptr = strrchr( fullpath, *dirsep );
    if ( chptr ) chptr++;
    else	 chptr = fullpath;

    strcpy( pathbuf, chptr );
    return pathbuf;
}


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


int File_makeWritable( const char* fname, int recursive, int yn )
{
    FileNameString cmd;
    strcpy( cmd, "chmod " );
    if ( recursive ) strcat( cmd, "-R ");
    strcat( cmd, yn ? "ug+w " : "a-w " );
    strcat( cmd, fname );
    return system( cmd ) != -1 ? YES : NO;
}
