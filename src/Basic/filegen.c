/*+
 * AUTHOR   : A.H. Bril
 * DATE     : Sept 1993
 * FUNCTION : file utilities
-*/

static const char* rcsID = "$Id: filegen.c,v 1.9 2001-05-02 13:50:16 windev Exp $";

#include "filegen.h"
#include "genc.h"
#include "string2.h"
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>

#ifndef __win__
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
static struct stat statbuf;
#else
#include <windows.h>
#include <shlwapi.h>
#endif

static const char* dirsep = sDirSep;


int File_exists( const char* fname )
{
#ifdef __win__
    return fname && *fname && PathFileExists( fname );
#else
    return !fname || stat(fname,&statbuf) < 0 ? NO : YES;
#endif
}


int File_isEmpty( const char* fname )
{
#ifdef __win__
    return !File_exists( fname );
#else
    return !fname || stat(fname,&statbuf) < 0 || statbuf.st_size < 1 ? YES : NO;
#endif
}


int File_isDirectory( const char* dirname )
{
#ifdef __win__
    return PathIsDirectory( dirname );
#else
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
#endif
}


int File_isAbsPath( const char* fname )
{
    if ( !fname ) return YES;
    skipLeadingBlanks( fname );
#ifdef __win__
    return !PathIsRelative( fname );
#else
    return *fname == *dirsep;
#endif
}


const char* File_getFullPath( const char* path, const char* filename )
{
    static FileNameString pathbuf;
#ifndef __win__
    int lastpos;
    FileNameString newpath;
    char* ptr;
#endif

    if ( path != pathbuf )
	strcpy( pathbuf, path && *path ? path : "." );

    if ( !filename || !*filename ) return pathbuf;

#ifdef __win__

    PathAppend( (char*)pathbuf, filename );

#else

    while ( matchString("..",(const char*)pathbuf) )
    {
	strcpy( pathbuf, File_getPathOnly(pathbuf) );
	ptr = pathbuf;
	while ( *ptr == *dirsep ) ptr++;
	strcpy( newpath, ptr );
	strcpy( pathbuf, newpath );
    }

    lastpos = strlen( pathbuf ) - 1;
    if ( lastpos >= 0 && pathbuf[lastpos] != *dirsep )
	strcat( pathbuf, dirsep );

    strcat( pathbuf, filename );

#endif

    return pathbuf;
}


const char* File_getPathOnly( const char* fullpath )
{
    static FileNameString pathbuf;
#ifndef __win__
    char* chptr;
#endif

    if ( fullpath != pathbuf )
	strcpy( pathbuf, fullpath ? fullpath : "" );

    if ( !pathbuf[0] || !strcmp(pathbuf,".") )
    {
	strcpy( pathbuf, ".." );
	return pathbuf;
    }

#ifdef __win__
    
    PathRemoveFileSpec( (char*)pathbuf );

#else

    /* search for last occurrence of directory separator in path */
    chptr = strrchr( pathbuf, *dirsep );
    if ( chptr ) *chptr = '\0';
    else	 strcpy( pathbuf, "." );

#endif

    return pathbuf;

}


const char* File_getFileName( const char* fullpath )
{
    static FileNameString pathbuf;

#ifndef __win__
    const char* chptr;
#endif

    pathbuf[0] = '\0';
    if ( !fullpath || !*fullpath ) return pathbuf;

#ifdef __win__

    strcpy( (char*)pathbuf, fullpath );
    PathStripPath( (char*)pathbuf );

#else

    /* search for last occurrence of directory separator in path */
    chptr = strrchr( fullpath, *dirsep );
    if ( chptr ) chptr++;
    else	 chptr = fullpath;

    strcpy( pathbuf, chptr );

#endif

    return pathbuf;
}


const char* File_getBaseName( const char* fullpath )
{
    static FileNameString pathbuf;
    FileNameString fname;

#ifndef __win__
    char* chptr;
#endif

    pathbuf[0] = '\0';
    if ( !fullpath || !*fullpath ) return pathbuf;

    strcpy( fname, File_getFileName(fullpath) );

#ifdef __win__

	PathRemoveExtension( fname );

#else

    /* search for first occurrence of . in filename */
    chptr = strchr( fname, '.' );
    if ( chptr ) *chptr = '\0';

#endif

    strcpy( pathbuf, fname );
    return pathbuf;
}


const char* File_getTempFileName( const char* id, const char* ext, int full )
{
    static FileNameString pathbuf;


#ifdef __win__

    sprintf( pathbuf, "dgb%d%d%s", id, (int)(Stat_getRandom() * 10000), getPID() );

#else

    if ( id && *id )
	sprintf( pathbuf, "%sdgb%s%d", full ? "/tmp/" : "", id, getPID() );
    else
	sprintf( pathbuf, "%sdgb%d", full ? "/tmp/" : "", getPID() );

#endif

    if ( ext )
    {
	strcat( pathbuf, "." );
	strcat( pathbuf, ext );
    }

    return pathbuf;
}


int File_createDir( const char* dirname, int mode )
{
    if ( !dirname || !*dirname ) return NO;
    if ( mode == 0 ) mode = 0755;

#ifdef __win__

    return CreateDirectory( dirname, 0 );

#else

    return mkdir( dirname, (mode_t)mode ) < 0 ? NO : YES;

#endif
}


int File_rename( const char* oldname, const char* newname )
{
    if ( !oldname || !*oldname || !newname || !*newname ) return NO;

#ifdef __win__

    return MoveFile( oldname, newname );

#else

    return rename(oldname,newname) ? NO : YES;

#endif
}


int File_copy( const char* from, const char* to, int recursive )
{
#ifdef __win__

    if ( !File_exists(from) ) return YES;
    if ( recursive )
	{ fprintf(stderr,"File_copy recursive not impl\n"); return NO; }

    return CopyFile( from, to, FALSE );

#else

    char* cmd;
    int len, retval;
    if ( !from || !*from || !to || !*to ) return NO;
    if ( !File_exists(from) ) return YES;

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

#endif
}


int File_remove( const char* fname, int force, int recursive )
{

#ifdef __win__

    if ( !File_exists(fname) ) return YES;

    if ( recursive )
	{ fprintf(stderr,"File_remove recursive not impl\n"); return NO; }

    return DeleteFile( fname );

#else

    char* cmd;
    int len, retval;
    if ( !fname ) return NO;
    if ( !File_exists(fname) ) return YES;

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

#endif
}


int File_makeWritable( const char* fname, int recursive, int yn )
{

#ifdef __win__

    return YES;

#else

    FileNameString cmd;
    strcpy( cmd, "chmod " );
    if ( recursive ) strcat( cmd, "-R ");
    strcat( cmd, yn ? "ug+w " : "a-w " );
    strcat( cmd, fname );
    return system( cmd ) != -1 ? YES : NO;

#endif
}
