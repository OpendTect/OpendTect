/*+
 * AUTHOR   : A.H. Bril
 * DATE     : Sept 1993
 * FUNCTION : file utilities
-*/

static const char* rcsID = "$Id: filegen.c,v 1.42 2003-10-28 12:15:22 arend Exp $";

#include "filegen.h"
#include "genc.h"
#include "string2.h"
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>

#ifndef __win__

# include <sys/stat.h>
# include <unistd.h>
# include <dirent.h>

# ifdef lux

#  include <sys/statfs.h>
#   define mStatFS statfs

# else

#  include <sys/statvfs.h>
#  define mStatFS statvfs

# endif

 static struct mStatFS fsstatbuf;
 static struct stat statbuf;

#else

# include <windows.h>
# include <shlwapi.h>
# include <time.h>
# include <largeint.h>

#endif

static const char* dirsep = sDirSep;


int File_exists( const char* fname )
{
#ifdef __win__
    return fname && *fname && PathFileExists( fname );
#else
    return fname && *fname && stat(fname,&statbuf) >= 0 ? YES : NO;
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
    if ( !dirname || !*dirname || !File_exists(dirname) ) return 0;
    return S_ISDIR(statbuf.st_mode);
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


int File_isRemote( const char* fname )
{
#ifdef __win__
    return NO;
#else
    if ( !File_exists(fname)
      || mStatFS(fname,&fsstatbuf) )
	return NO;

# ifdef lux
    /* return fsstatbuf.f_type == NFS_SUPER_MAGIC
	|| fsstatbuf.f_type == SMB_SUPER_MAGIC; */
    return fsstatbuf.f_type == 0x6969 || fsstatbuf.f_type == 0x517B;
# else
    return fsstatbuf.f_basetype[0] == 'n' && fsstatbuf.f_basetype[1] == 'f';
# endif
#endif
}

#define mToKbFac (1.0 / 1024.0)

int File_getFreeMBytes( const char* dirnm )
{
    double res = mToKbFac;

#ifdef __win__

    ULARGE_INTEGER freeBytesAvail2User; 
    ULARGE_INTEGER totalNrBytes;
    ULARGE_INTEGER totalNrFreeBytes;

    GetDiskFreeSpaceEx( dirnm, &freeBytesAvail2User,
			&totalNrBytes, &totalNrFreeBytes);

    double fac = MAXLONG;
    fac += 1;

    double aap = freeBytesAvail2User.HighPart * fac + 
		 freeBytesAvail2User.LowPart;

    res *= res * aap;

#else


    if ( !File_exists(dirnm)
      || mStatFS(dirnm,&fsstatbuf) )
	return 0;

    res *= res			/* to MB */
	* fsstatbuf.f_bavail	/* available blocks */
#ifdef lux
	* fsstatbuf.f_bsize;	/* block size */
#else
	* fsstatbuf.f_frsize;	/* 'real' block size */
#endif
    return (int)(res + .5);

#endif
}


int File_getKbSize( const char* fnm )
{
#ifdef __win__
    return 0;
#else

    double res = mToKbFac;

    if ( !File_exists(fnm) )
	return 0;

    res *= statbuf.st_size;
    return (int)(res + .5);

#endif
}


int File_isWritable( const char* fnm )
{
    FileNameString cmd;
    if ( !File_exists(fnm) ) return 0;

    sprintf( cmd, "test -w '%s'", fnm );
    return !system( cmd );
}


const char* File_getFullPath( const char* path, const char* filename )
{
    static FileNameString pathbuf;
    char* chptr;
#ifndef __win__
    int lastpos;
    FileNameString newpath;
#endif

    if ( path != pathbuf )
	strcpy( pathbuf, path && *path ? path : "." );

    if ( !filename || !*filename ) return pathbuf;

    /* Remove trailing dirseps from pathbuf */
    chptr = pathbuf; while ( *chptr ) chptr++; chptr--;
    while ( chptr != pathbuf-1 && *chptr == *dirsep ) *chptr-- = '\0';

#ifdef __win__

    PathAppend( (char*)pathbuf, filename );

    return pathbuf;

#else

    while ( matchString("..",(const char*)pathbuf) )
    {
	strcpy( pathbuf, File_getPathOnly(pathbuf) );
	chptr = pathbuf;
	while ( *chptr == *dirsep ) chptr++;
	strcpy( newpath, chptr );
	strcpy( pathbuf, newpath );
    }

    chptr = (char*)pathbuf;
    while ( *chptr == '.' && *(chptr+1) == '/' ) chptr += 2;

    lastpos = strlen( chptr ) - 1;
    if ( lastpos >= 0 && chptr[lastpos] != *dirsep )
	strcat( chptr, dirsep );

    strcat( chptr, filename );

    return chptr;

#endif
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

    chptr = pathbuf; while ( *chptr ) chptr++; chptr--;
    /* Remove trailing dir separators */
    while ( chptr != pathbuf-1 && *chptr == *dirsep ) *chptr-- = '\0';
    if ( !strcmp(pathbuf,"..") )
	strcpy( pathbuf, "../.." );
    else if ( !strcmp(pathbuf,".") )
	strcpy( pathbuf, ".." );
    else if ( !*chptr )
	strcpy( pathbuf, "/" );
    else
    {
	/* Remove trailing file or dir name */
	while ( chptr != pathbuf-1 && *chptr != *dirsep ) *chptr-- = '\0';
	/* Remove all dir separators */
	while ( chptr != pathbuf-1 && *chptr == *dirsep ) *chptr-- = '\0';
	if ( !pathbuf[0] ) strcpy( pathbuf, "." );
    }

#endif

    return pathbuf;

}


const char* File_getFileName( const char* fullpath )
{
    static FileNameString pathbuf;
    char* chptr = pathbuf;
    *chptr = '\0';
    if ( !fullpath || !*fullpath ) return chptr;
    if ( !(*fullpath+1) && (*fullpath == '/' || *fullpath == '.') )
	return chptr;

    strcpy( chptr, fullpath );

#ifdef __win__

    PathStripPath( chptr );

#else

    /* Remove trailing dir separators */
    while ( *chptr ) chptr++; chptr--;
    while ( chptr != pathbuf-1 && *chptr == *dirsep ) *chptr-- = '\0';

    while ( chptr != pathbuf-1 && *chptr != *dirsep ) chptr--;
    if ( chptr == pathbuf-1 || *chptr == *dirsep ) chptr++;

#endif

    return chptr;
}


const char* File_removeExtension( const char* inppath )
{
    static FileNameString retbuf;
    char* ptr;
    if ( !inppath ) inppath = "";

    strcpy( retbuf, inppath );

#ifdef __win__

    PathRemoveExtension( retbuf );

#else

    if ( strrchr(File_getFileName(retbuf),'.') )
	*strrchr( retbuf, '.' ) = '\0';

#endif

    return retbuf;
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

    static FileNameString tmppath;

    if( full )
	    GetTempPath( PATH_LENGTH, tmppath );
    else
	    sprintf( tmppath, "" );

    GetTempFileName( tmppath, "dgb", 0, pathbuf );

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


const char* File_getSimpleTempFileName( const char* ext )
{
    static FileNameString pathbuf;
    FileNameString uniquestr;
    static size_t counter = 0;
    size_t time_stamp = time((time_t*) 0 ) + counter++;
    sprintf(uniquestr, "%X%X", getPID(), (int) time_stamp);

#ifdef __win__

    sprintf( pathbuf, "dgb%s", uniquestr );

#else

    sprintf( pathbuf, "/tmp/dgb%s", uniquestr );

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


#define mRet(v) { mFREE(cmd); return v; }

int File_rename( const char* from, const char* to )
{
    int rv, len;
    char* cmd = 0;

    if ( !from || !*from || !to || !*to ) return NO;
    if ( !File_exists(from) ) return YES;

#ifdef __win__

    int ret = MoveFile( from, to );

# ifdef __debug__
    if( ! ret )
    {

	LPVOID lpMsgBuf;

	FormatMessage( 
	    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
	    NULL,
	    GetLastError(),
	    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
	    (LPTSTR) &lpMsgBuf,
	    0,
	    NULL 
	);

	// Display the string.
	MessageBox( NULL, lpMsgBuf, "GetLastError", MB_OK|MB_ICONINFORMATION );

	// Free the buffer.
	LocalFree( lpMsgBuf );
	 
    }
# endif
    return ret;

#else

    if ( !rename(from,to) )
	return YES;

    // Probably to other disk
    len = strlen( from ) + strlen( to ) + 25;
    cmd = mMALLOC(len,char);
    
    strcpy( cmd, "/bin/mv -f '" );
    strcat( cmd, from );
    strcat( cmd, "' '" );
    strcat( cmd, to );
    strcat( cmd, "'" );

    rv = system( cmd ) != -1 ? YES : NO;
    if ( rv )
	rv = File_exists( to );
    mRet( rv )

#endif
}


int File_copy( const char* from, const char* to, int recursive )
{
#ifdef __win__
    char* cmd = 0;
    int len, retval;

    if ( !from || !*from || !to || !*to ) return NO;
    if ( !File_exists(from) ) return YES;

    if ( recursive )
    { 
	if ( !from || !*from || !to || !*to ) return NO;
	if ( !File_exists(from) ) return YES;
	if ( File_exists(to) ) return NO;

	len = strlen( from ) + strlen( to ) + 25;
	cmd = mMALLOC(len,char);
	
	strcpy( cmd, "xcopy /E /I /Q /H /K " );
	strcat( cmd, " \"" );
	strcat( cmd, from );
	strcat( cmd, "\" \"" );
	strcat( cmd, to );
	strcat( cmd, "\"" );

	retval = system( cmd ) != -1 ? YES : NO;
	if ( retval ) retval = File_exists( to );
	mFREE(cmd);
	return retval;
    }

    return CopyFile( from, to, FALSE );

#else

    char* cmd = 0;
    int len, retval;
    if ( !from || !*from || !to || !*to ) return NO;
    if ( !File_exists(from) ) return YES;

    len = strlen( from ) + strlen( to ) + 25;
    cmd = mMALLOC(len,char);
    
    strcpy( cmd, "/bin/cp " );
    if ( recursive ) strcat( cmd, "-r " );
    strcat( cmd, " '" );
    strcat( cmd, from );
    strcat( cmd, "' '" );
    strcat( cmd, to );
    strcat( cmd, "'" );

    retval = system( cmd ) != -1 ? YES : NO;
    if ( retval ) retval = File_exists( to );
    mRet( retval )

#endif
}


int File_remove( const char* fname, int recursive )
{

#ifdef __win__
	
    if ( !File_exists(fname) )
	return YES;

    if ( recursive )
    { 
	char* cmd;
	int len, retval;

	len = strlen( fname ) + 30;
	cmd = mMALLOC(len,char);

	strcpy( cmd, "rd /S /Q " );
	strcat( cmd, fname );

	retval = system( cmd ) ? NO : YES;

	if ( retval && File_exists( fname ) ) retval = NO;
	mFREE(cmd);
	return retval;
    }

    return DeleteFile( fname );

#else

    char* cmd = 0;
    char* ptr;
    int len, retval, islink;
    FileNameString targ_fname;

    if ( !File_exists(fname) )
	return YES;

    if ( !recursive )
	return unlink((char*)fname) ? NO : YES;

    len = strlen( fname ) + 30;
    cmd = mMALLOC(len,char);
    strcpy( cmd, "/bin/rm -rf '" );

    islink = File_isLink(fname) ? YES : NO;
    if ( File_isLink(fname) ) 
    {
	strcpy( targ_fname, File_linkTarget(fname) );
        if ( File_exists( targ_fname ) ) 
	{
	    ptr = cmd + strlen(cmd);
	    strcat( cmd, targ_fname );
	    strcat( cmd, "'" );
	    if ( !system(cmd) || File_exists(targ_fname) )
		mRet(NO)
	    *ptr = '\0';
	}
    }

    strcat( cmd, fname );
    strcat( cmd, "'" );
    retval = system( cmd ) ? NO : YES;
    if ( retval && File_exists( fname ) ) retval = NO;
    mRet( retval )

#endif
}


int File_makeWritable( const char* fname, int recursive, int yn )
{

#ifdef __win__

    FileNameString cmd;
    strcpy( cmd, "attrib " );
    strcat( cmd, yn ? " -R " : " +R " );
    strcat( cmd, fname );
    if ( recursive && File_isDirectory(fname) ) strcat( cmd, "\\*.* /S ");
    return system( cmd ) != -1 ? YES : NO;

#else

    FileNameString cmd;
    strcpy( cmd, "chmod " );
    if ( recursive ) strcat( cmd, "-R ");
    strcat( cmd, yn ? "ug+w '" : "a-w '" );
    strcat( cmd, fname );
    strcat( cmd, "'" );
    return system( cmd ) != -1 ? YES : NO;

#endif
}


int File_isLink( const char* fname )
{
#ifdef __win__
    return NO;
#else
    return fname && lstat(fname,&statbuf) >= 0 && S_ISLNK(statbuf.st_mode)
	 ? YES : NO;
#endif
}


int File_createLink( const char* from, const char* to )
{
#ifdef __win__
    return NO;
#else
    char cmd[512];
    if ( !from || !to || !*from || !*to ) return NO;

    strcpy( cmd, "ln -s '" );
    strcat( cmd, from );
    strcat( cmd, "' '" );
    strcat( cmd, to );
    strcat( cmd, "'" );
    if ( system( cmd ) == -1 ) return -1;
    return File_isLink( to );
#endif
}


const char* File_linkTarget( const char* fname )
{
#ifdef __win__
    return 0;
#else
    static FileNameString pathbuf;
    return File_isLink(fname) && readlink(fname,pathbuf,256) != -1
	 ? pathbuf : fname;
#endif
}


const char* File_getCurrentDir()
{
    static FileNameString pathbuf;
    getcwd( pathbuf, PATH_LENGTH );
    return pathbuf;
}
