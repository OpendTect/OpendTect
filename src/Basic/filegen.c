/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sept 1993
 * FUNCTION : file utilities
-*/

static const char* rcsID = "$Id: filegen.c,v 1.73 2007-10-05 09:20:32 cvsnanne Exp $";

#include "filegen.h"
#include "string2.h"
#include "genc.h"
#include "mallocdefs.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#ifndef __msvc__
# include <dirent.h>
#endif

#ifdef __win__
# include "winutils.h"

# include <windows.h>
# include <shlwapi.h>
# include <time.h>
# include <io.h>

# ifndef __msvc__
#  include <largeint.h>
# endif

# define stat _stati64
# define fstat _fstat
# define access _access

#else

# include <unistd.h>

# ifdef lux

#  include <sys/statfs.h>
#  define mStatFS statfs

# else

#  ifdef __mac__
#   include <sys/mount.h>
#   define mStatFS statfs
#  else
#   include <sys/statvfs.h>
#   define mStatFS statvfs
#  endif
# endif

 static struct mStatFS fsstatbuf;

#endif

static struct stat statbuf;


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
    if( !File_exists( fname ) ) return YES;
#ifdef __msvc__
    pErrMsg("File_isEmpty not fully implemented!");
    return NO;
#else
    return stat(fname,&statbuf) < 0 || statbuf.st_size < 1 ? YES : NO;
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
#  ifdef mac
#ifdef __debug__
	fprintf(stderr,"File_IsRemote untested. Please verify");
#endif
    return fsstatbuf.f_fstypename[0] == 'n' && fsstatbuf.f_fstypename[1] == 'f';
#  else
    return fsstatbuf.f_basetype[0] == 'n' && fsstatbuf.f_basetype[1] == 'f';
#  endif
# endif
#endif
}

#define mToKbFac (1.0 / 1024.0)

int File_getFreeMBytes( const char* dirnm )
{
    double fac = mToKbFac;
    double res;

    if ( !File_exists(dirnm) )
	return 0;

#ifdef __win__

    ULARGE_INTEGER freeBytesAvail2User; 
    ULARGE_INTEGER totalNrBytes;
    ULARGE_INTEGER totalNrFreeBytes;

    GetDiskFreeSpaceEx( dirnm, &freeBytesAvail2User,
			&totalNrBytes, &totalNrFreeBytes);

    res = freeBytesAvail2User.LowPart * fac * fac;
    res += ((double)freeBytesAvail2User.HighPart) * 2048;

#else

    if ( mStatFS(dirnm,&fsstatbuf) )
	return 0;

    res = fac * fac		/* to MB */
	* fsstatbuf.f_bavail	/* available blocks */
#ifdef lux
	* fsstatbuf.f_bsize;	/* block size */
#else
# ifdef mac
	* fsstatbuf.f_bsize;	/* fundamental file system block size */
# else
	* fsstatbuf.f_frsize;	/* 'real' block size */
# endif
#endif

#endif

    return (int)(res + .5);
}


int File_getKbSize( const char* fnm )
{
#ifdef __msvc__
    return 0;
#else

    double res = mToKbFac;

    if ( !File_exists(fnm) )
	return 0;
#ifdef __win__
    stat((char*)fnm,&statbuf);
#endif
    res *= statbuf.st_size;
    if ( res < 0 )
	res = - res * 2;
    return (int)(res + .5);

#endif
}


const char* File_getTime( const char* fnm )
{
    static char buf[64];
#ifdef __msvc__
    return 0;
#else

    if ( !File_exists(fnm) )
	return 0;
#ifdef __win__
    stat((char*)fnm,&statbuf);
    // TODO: make this work on win32 (now undefined reference to ctime_r)
    return 0;
#endif

    (void)ctime_r( &statbuf.st_mtime, buf );
    return buf;

#endif
}


int File_isWritable( const char* fnm )
{
    FileNameString cmd;
    if ( !File_exists(fnm) ) return 0;

    int acc =  access( fnm, W_OK );

    return !acc;
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

    if ( ret )
	ret = File_exists( to );

    return ret;

#else

    if ( !rename(from,to) )
    {
	if ( File_exists(to) )
	    return YES;
	else if ( !File_exists(from) )
	    return NO;
    }

    // Failed. May be to other disk. Let's try again via shell.
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

	len = strlen( from ) + strlen( to ) + 128;
	cmd = mMALLOC(len,char);
	
	strcpy( cmd, "xcopy /E /I /Q /H " );
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

	strcpy( cmd, "rd /S /Q \"" );
	strcat( cmd, fname );
	strcat( cmd, "\"" );

	retval = system( cmd ) ? NO : YES;

	if ( retval && File_exists( fname ) ) retval = NO;
	mFREE(cmd);
	return retval;
    }

    return DeleteFile( fname );

#else

    char* cmd = 0;
    char* ptr;
    int len, retval;
    FileNameString targ_fname;

    if ( !File_exists(fname) )
	return YES;

    if ( !recursive )
	return unlink((char*)fname) ? NO : YES;

    len = strlen( fname ) + 30;
    cmd = mMALLOC(len,char);
    strcpy( cmd, "/bin/rm -rf '" );

    if ( File_isLink(fname) ) 
    {
	strcpy( targ_fname, File_linkTarget(fname) );
        if ( File_exists( targ_fname ) ) 
	{
	    ptr = cmd + strlen(cmd);
	    strcat( cmd, targ_fname );
	    strcat( cmd, "'" );
	    if ( system(cmd)<0 || File_exists(targ_fname) )
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
    strcat( cmd, "\"" );
    strcat( cmd, fname );
    strcat( cmd, "\"" );
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

    FileNameString fnm;
    if ( !fname || !*fname ) return 0;
    if ( strstr(fname,".lnk") || strstr(fname,".LNK") )
	return YES;

    strcpy( fnm, fname ); strcat( fnm, ".lnk" );
    if ( File_exists(fnm) ) return YES;
    strcpy( fnm, fname ); strcat( fnm, ".LNK" );
    return File_exists(fnm);

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
    if ( !File_isLink(fname) )
	return fname;

    return getWinLinkTarget(fname);

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
